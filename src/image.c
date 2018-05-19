#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ivm_list.h>
#include <ivm_vm.h>
#include <ivm_image.h>



static bool overlaps(struct ivm_image* image, uint64_t addr, size_t size)
{
    ivm_list_foreach(struct ivm_segment, seg, &image->segments) {
        uint64_t start = seg->vm_start;
        uint64_t end = seg->vm_start + seg->vm_size;

        if ((start <= addr && addr < end) || (start <= addr + size && addr + size < end)) {
            return true;
        }
    }

    return false;
}



static void remove_segment(struct ivm_segment* segment)
{
    ivm_list_foreach(struct ivm_section, sect, &segment->sections) {
        segment->image->num_sections--;
        ivm_list_remove(sect);
        free(sect);
    }

    segment->image->num_segments--;
    segment->image->file_size -= segment->file_size;

    struct ivm_segment* next = ivm_list_next(&segment->image->segments, segment);

    while (next != NULL) {
        next->file_start -= segment->file_start;
        next = ivm_list_next(&segment->image->segments, next);
    }

    ivm_list_remove(segment);
    free(segment);
}



static int create_vm_data(struct ivm_image* image, size_t num_states, size_t frame_size, size_t num_frames)
{
    size_t data_size = sizeof(struct ivm_data) 
        + sizeof(struct ivm_registers)
        + sizeof(struct ivm_state) * num_states
        + sizeof(struct ivm_frame) * num_frames;

    struct ivm_data* data = malloc(data_size);
    if (data == NULL) {
        return errno;
    }
    memset(data, 0, data_size);

    data->state_size = num_states;
    data->fshift = 9; // FIXME: calculate from frame size
    data->fsize = frame_size;
    data->fnum = num_frames;

    image->data = data;
    image->data_size = data_size;
    image->data_offset_to_regs = sizeof(struct ivm_data);
    image->data_offset_to_states = image->data_offset_to_regs + sizeof(struct ivm_registers);
    image->data_offset_to_ft = image->data_offset_to_states + sizeof(struct ivm_state) * num_states;
    image->data_offset_to_ct = image->data_offset_to_ft + sizeof(struct ivm_frame) * num_frames;

    return 0;
}



int ivm_image_create(struct ivm_image** handle, size_t stack_size, size_t frame_size, size_t num_frames)
{
    if (handle == NULL) {
        return EINVAL;
    }

    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize < 1) {
        return errno;
    }

    struct ivm_image* image = malloc(sizeof(struct ivm_image));
    if (image == NULL) {
        return errno;
    }

    int err = create_vm_data(image, stack_size, frame_size, num_frames);
    if (err != 0) {
        free(image);
        return err;
    }

    image->file_size = 0;
    image->vm_file_offset = 0;
    image->vm_code = NULL;
    image->vm_entry_point = 0;
    image->num_segments = 0;
    image->num_sections = 0;
    image->page_size = pagesize;
    ivm_list_init(&image->segments);

    *handle = image;
    return 0;
}



void ivm_image_remove(struct ivm_image* image)
{
    ivm_list_foreach(struct ivm_segment, seg, &image->segments) {
        remove_segment(seg);
    }

    free(image->data);
    free(image->vm_code);
    free(image);
}



int ivm_image_add_segment(struct ivm_segment** handle, 
                          struct ivm_image* image, 
                          enum ivm_segment_type type, 
                          size_t align, 
                          uint64_t addr,
                          size_t size,
                          size_t file_align)
{
    addr = IVM_ALIGN_ADDR(addr, align);
    size = IVM_ALIGN_ADDR(size, align);

    if (overlaps(image, addr, size)) {
        return EFAULT;
    }

    struct ivm_segment* segment = malloc(sizeof(struct ivm_segment));
    if (segment == NULL) {
        return errno;
    }

    segment->type = type;
    segment->image = image;
    ivm_list_insert(&image->segments, segment);
    segment->vm_align = align;
    segment->vm_start = addr;
    segment->vm_size = size;
    segment->file_start = image->file_size;
    segment->file_align = file_align;
    segment->file_size = 0;
    ivm_list_init(&segment->sections);
    
    image->num_segments++;

    if (handle != NULL) {
        *handle = segment;
    }
    return 0;
}



int ivm_image_add_section(struct ivm_section** handle,
                          struct ivm_segment* segment,
                          enum ivm_section_type type,
                          size_t align,
                          const void* data,
                          size_t size)
{
    struct ivm_section* section = malloc(sizeof(struct ivm_section));
    if (section == NULL) {
        return errno;
    }

    const struct ivm_section* prev = ivm_list_last(struct ivm_section, &segment->sections);

    section->type = type;
    section->segment = segment;
    ivm_list_insert(&segment->sections, section);
    section->data = data;
    section->size = size;
    section->vm_align = align;
    section->vm_size = IVM_ALIGN_ADDR(size, align);
    section->vm_offset_to_seg = IVM_ALIGN_ADDR(prev != NULL ? prev->vm_offset_to_seg + prev->vm_size : 0, align);
    uint64_t prev_offset = prev != NULL ? IVM_ALIGN_ADDR(prev->vm_offset_to_seg + prev->vm_size, prev->vm_align) : 0;
    section->vm_offset_to_prev = section->vm_offset_to_seg - prev_offset;
    section->size = size;
    section->data = data;

    if (segment->vm_start + section->vm_offset_to_seg + section->vm_size > segment->vm_start + segment->vm_size) {
        ivm_list_remove(section);
        free(section);
        return ENOSPC;
    }

    // Adjust file pointers
    section->file_offset_to_seg = prev != NULL ? prev->file_offset_to_seg + prev->size + prev->file_padding : 0;
    section->file_offset_to_prev = 0;
    section->file_padding = IVM_ALIGN_ADDR(size, segment->file_align) - size;
    segment->file_size += section->size + section->file_padding;

    struct ivm_image* image = segment->image;
    image->num_sections++;
    image->file_size += section->size + section->file_padding;

    if (handle != NULL) {
        *handle = section;
    }

    return 0;
}



int ivm_image_load_vm(struct ivm_image* image, const struct ivm_vm_functions* funcs, uint64_t addr)
{
    int err;

    size_t code_align = 8;

    size_t size = image->page_size // offset with one page
        + IVM_ALIGN_ADDR(funcs->loader.size, code_align)
        + IVM_ALIGN_ADDR(funcs->vm.size, code_align)
        + IVM_ALIGN_ADDR(funcs->interrupt.size, code_align);

    size = IVM_ALIGN_ADDR(size, image->page_size);

    image->vm_code = malloc(size);
    if (image->vm_code == NULL) {
        return errno;
    }

    // Load VM code (offset by one page)
    unsigned char* ldptr = (unsigned char*) IVM_ALIGN_ADDR(image->vm_code, code_align) + image->page_size;
    memcpy(ldptr, (void*) funcs->loader.addr, funcs->loader.size);
    unsigned char* vmptr = ldptr + IVM_ALIGN_ADDR(funcs->loader.size, code_align);
    memcpy(vmptr, (void*) funcs->vm.addr, funcs->vm.size);
    unsigned char* intrptr = vmptr + IVM_ALIGN_ADDR(funcs->vm.size, code_align);
    memcpy(intrptr, (void*) funcs->interrupt.addr, funcs->interrupt.size);

    // TODO: create syscall table
    
    // Create code segment
    struct ivm_segment* segment = NULL;
    err = ivm_image_add_segment(&segment, image, IVM_SEG_CODE, image->page_size, addr, size, image->page_size);
    if (err != 0) {
        free(image->vm_code);
        image->vm_code = NULL;
        return err;
    }

    // Create code section
    struct ivm_section* section = NULL;
    err = ivm_image_add_section(&section, segment, IVM_SECT_CODE, 1, ((unsigned char*) image->vm_code) + image->page_size, size - image->page_size);
    if (err != 0) {
        free(image->vm_code);
        image->vm_code = NULL;
        return err;
    }

    image->vm_entry_point = segment->vm_start;
    image->vm_file_offset = segment->file_start;
    image->data->vm_addr = segment->vm_start + image->page_size + (vmptr - ldptr);
    image->data->interrupt = (ivm_interrupt_t) (segment->vm_start + image->page_size + (intrptr - ldptr));

    strcpy(image->data->id, funcs->id);
    return 0;
}



static void initialize_frame_table(struct ivm_image* image, uint64_t addr, size_t size)
{
    struct ivm_frame* frames = (struct ivm_frame*) (((unsigned char*) image->data) + image->data_offset_to_ft);

    for (size_t i = 0; i < image->data->fnum; ++i) {
        frames[i].addr = 0;
        if (image->data->fsize * i <= size) {
            frames[i].addr = addr + image->data->fsize * i;
        }
        frames[i].attr = 0;
        frames[i].file = -1;
        frames[i].offs = 0;
    }
}



int ivm_image_reserve_vm_data(struct ivm_image* image, uint64_t data_addr, size_t bytecode_size)
{
    int err;

    if (bytecode_size > image->data->fsize * image->data->fnum) {
        return EINVAL;
    }

    struct ivm_segment* data_segment = NULL;
    err = ivm_image_add_segment(&data_segment, image, IVM_SEG_DATA, image->page_size, data_addr, image->data_size, image->page_size);
    if (err != 0) {
        return err;
    }

    if (data_segment->vm_start != data_addr) {
        return EFAULT;
    }

    image->data->registers = (void*) (data_segment->vm_start + image->data_offset_to_regs);
    image->data->states = (void*) (data_segment->vm_start + image->data_offset_to_states);
    image->data->ftable = (void*) (data_segment->vm_start + image->data_offset_to_ft);
    image->data->ctable = (void*) (data_segment->vm_start + image->data_offset_to_ct);

    struct ivm_section* data_section = NULL;
    err = ivm_image_add_section(&data_section, data_segment, IVM_SECT_DATA, image->page_size, image->data, image->data_size);
    if (err != 0) {
        return err;
    }

    
    struct ivm_segment* code_segment = NULL;
    err = ivm_image_add_segment(&code_segment, image, IVM_SEG_DATA, image->page_size, data_addr + data_segment->vm_size, bytecode_size, image->page_size);
    if (err != 0) {
        return err;
    }

    struct ivm_section* code_section = NULL;
    err = ivm_image_add_section(&code_section, code_segment, IVM_SECT_BYTECODE, image->page_size, NULL, bytecode_size);
    if (err != 0) {
        return err;
    }

    initialize_frame_table(image, code_segment->vm_start, bytecode_size);
    return 0;
}
