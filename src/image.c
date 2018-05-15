#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <noravm_list.h>
#include <noravm_vm.h>
#include <noravm_image.h>



//static void grow_segment(struct noravm_segment* segment, size_t grow)
//{
//    grow = NORAVM_ALIGN_ADDR(grow, segment->alignment);
//    segment->vm_size += grow;
//
//    size_t prev_end = segment->vm_start + segment->vm_size;
//
//    struct noravm_segment* ptr = noravm_list_next(&segment->image->segments, segment);
//    while (ptr != NULL) {
//        ptr->vm_start = NORAVM_ALIGN_ADDR(prev_end, ptr->alignment);
//        prev_end = ptr->vm_start + ptr->vm_size;
//        // FIXME: set offset to previous to match gap
//        // FIXME: move file offsets
//        
//        ptr = noravm_list_next(&segment->image->segments, ptr);
//    }
//}



static void noravm_image_remove_segment(struct noravm_segment* segment)
{
    noravm_list_foreach(struct noravm_section, sect, &segment->sections) {
        segment->image->num_sections--;
        noravm_list_remove(sect);
        free(sect);
    }

    segment->image->num_segments--;
    free(segment);
}



int noravm_image_create(struct noravm_image** handle, uint64_t start_addr)
{
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize < 1) {
        return errno;
    }

    struct noravm_image* image = malloc(sizeof(struct noravm_image));
    if (image == NULL) {
        return errno;
    }

    memset(image->noravm_entry.id, 0, sizeof(image->noravm_entry));
    image->noravm_entry.data_addr = 0;
    image->noravm_entry.code_addr = 0;
    image->noravm_entry.code_size = 0;
    image->noravm_entry.mem_addr = 0;
    image->noravm_entry.mem_size = 0;
    image->noravm_entry.machine_addr = 0;
    image->noravm_entry.intr_addr = 0;

    noravm_list_init(&image->segments);
    image->noravm_file_offset = 0;
    image->vm_start = start_addr;
    image->file_size = 0;
    image->num_segments = 0;
    image->num_sections = 0;
    image->vm_code = NULL;
    image->page_size = pagesize;

    *handle = image;
    return 0;
}



void noravm_image_remove(struct noravm_image* image)
{
    noravm_list_foreach(struct noravm_segment, seg, &image->segments) {
        noravm_image_remove_segment(seg);
    }

    free(image->vm_code);
    free(image);
}



int noravm_image_add_segment(struct noravm_segment** handle, 
                             struct noravm_image* image, 
                             enum noravm_segment_type type, 
                             size_t align, 
                             size_t size)
{
    struct noravm_segment* segment = malloc(sizeof(struct noravm_segment));
    if (segment == NULL) {
        return errno;
    }

    const struct noravm_segment* prev = noravm_list_last(struct noravm_segment, &image->segments);
    segment->image = image;
    image->num_segments++;

    noravm_list_insert(&image->segments, segment);
    segment->type = type;
    segment->vm_align = align;
    segment->vm_start = NORAVM_ALIGN_ADDR(prev != NULL ? prev->vm_start + prev->vm_size : image->vm_start, align);
    segment->vm_size = NORAVM_ALIGN_ADDR(size, align);
    segment->file_start = image->file_size;
    uint64_t prev_addr = prev != NULL ? NORAVM_ALIGN_ADDR(prev->vm_start + prev->vm_size, prev->vm_align) : image->vm_start;
    segment->file_offset_to_prev = segment->vm_start - prev_addr;
    segment->file_size = 0;

    noravm_list_init(&segment->sections);

    if (handle != NULL) {
        *handle = segment;
    }
    return 0;
}



int noravm_image_add_section(struct noravm_section** handle,
                             struct noravm_segment* segment,
                             enum noravm_section_type type,
                             size_t vm_align,
                             size_t file_align,
                             const void* data,
                             size_t size)
{
    struct noravm_section* section = malloc(sizeof(struct noravm_section));
    if (section == NULL) {
        return errno;
    }

    const struct noravm_section* prev = noravm_list_last(struct noravm_section, &segment->sections);
    section->segment = segment;
    segment->image->num_sections++;
    noravm_list_insert(&segment->sections, section);
    section->type = type;
    section->vm_align = vm_align;
    section->file_align = file_align;
    section->vm_size = NORAVM_ALIGN_ADDR(size, vm_align);
    section->vm_offset_to_seg = NORAVM_ALIGN_ADDR(prev != NULL ? prev->vm_offset_to_seg + prev->vm_size : 0, vm_align);
    uint64_t prev_offset = prev != NULL ? NORAVM_ALIGN_ADDR(prev->vm_offset_to_seg + prev->vm_size, prev->vm_align) : 0;
    section->vm_offset_to_prev = section->vm_offset_to_seg - prev_offset;
    section->size = size;
    section->data = data;

    // Adjust memory pointers
    if (segment->vm_start + section->vm_offset_to_seg + section->vm_size > segment->vm_start + segment->vm_size) {
        //grow_segment(segment, (segment->vm_start + section->vm_offset_to_seg + section->vm_size) - (segment->vm_start + section->vm_size));
        free(section);
        return ENOSPC;
    }

    struct noravm_image* image = segment->image;
    section->file_offset_to_seg = prev != NULL ? prev->file_offset_to_seg + prev->size + prev->file_padding : 0;
    section->file_offset_to_prev = 0;

    // Adjust file pointers
    if (file_align > 0) {
        section->file_padding = NORAVM_ALIGN_ADDR(size, section->file_align) - size;

        image->file_size += section->size + section->file_padding;
        segment->file_size += section->size + section->file_padding;
    }
    else {
        section->file_padding = 0;
    }

    if (handle != NULL) {
        *handle = section;
    }

    return 0;
}



int noravm_image_load_vm(struct noravm_image* image, const struct noravm_functions* funcs, size_t vm_align, size_t code_align)
{
    int err;

    size_t pagesize = image->page_size;
    size_t size = NORAVM_ALIGN_ADDR(funcs->loader.size, code_align) 
        + NORAVM_ALIGN_ADDR(funcs->vm.size, code_align) 
        + NORAVM_ALIGN_ADDR(funcs->intr.size, code_align);

    size = NORAVM_ALIGN_ADDR(size, pagesize);

    image->vm_code = malloc(size);
    if (image->vm_code == NULL) {
        return errno;
    }

    // Load VM code
    unsigned char* loaderptr = (unsigned char*) NORAVM_ALIGN_ADDR(image->vm_code, code_align);
    memcpy(loaderptr, (void*) funcs->loader.addr, funcs->loader.size);
    unsigned char* vmptr = loaderptr + NORAVM_ALIGN_ADDR(funcs->loader.size, code_align);
    memcpy(vmptr, (void*) funcs->vm.addr, funcs->vm.size);
    unsigned char* intrptr = vmptr + NORAVM_ALIGN_ADDR(funcs->vm.size, code_align);
    memcpy(intrptr, (void*) funcs->intr.addr, funcs->intr.size);

    // Create code segment
    struct noravm_segment* segment = NULL;
    err = noravm_image_add_segment(&segment, image, NORAVM_SEG_NORAVM, vm_align, size);
    if (err != 0) {
        free(image->vm_code);
        image->vm_code = NULL;
        return err;
    }

    // Create code section
    struct noravm_section* section = NULL;
    err = noravm_image_add_section(&section, segment, NORAVM_SECT_CODE, 1, pagesize, image->vm_code, size);
    if (err != 0) {
        free(image->vm_code);
        image->vm_code = NULL;
        return err;
    }

    image->vm_entry_point = segment->vm_start;
    image->noravm_file_offset = segment->file_start;

    image->noravm_entry.machine_addr = segment->vm_start + (vmptr - loaderptr);
    image->noravm_entry.intr_addr = segment->vm_start + (intrptr - loaderptr);

    strcpy(image->noravm_entry.id, funcs->id);
    return 0;
}



int noravm_image_reserve_vm_data(struct noravm_image* image, uint64_t data_addr, size_t data_size, size_t stack_entries, size_t bytecode_size)
{
    int err;

    size_t pagesize = image->page_size;
    size_t stack_size = stack_entries * sizeof(struct noravm_state);
    size_t state_size = sizeof(struct noravm_data) + stack_size;

    if (bytecode_size > data_size) {
        return EINVAL;
    }

    struct noravm_segment* data = NULL;
    err = noravm_image_add_segment(&data, image, NORAVM_SEG_DATA, data_addr, data_size + pagesize + state_size);
    if (err != 0) {
        return err;
    }
    
    if (data->vm_start != data_addr) {
        return EFAULT;
    }

    struct noravm_section* entry_point = NULL;
    err = noravm_image_add_section(&entry_point, data, NORAVM_SECT_CONST, pagesize, pagesize, &image->noravm_entry, sizeof(struct noravm_entry_point));
    if (err != 0) {
        return err;
    }
    
    struct noravm_section* vm_memory = NULL;
    err = noravm_image_add_section(&vm_memory, data, NORAVM_SECT_BYTECODE, pagesize, pagesize, NULL, bytecode_size);
    if (err != 0) {
        return err;
    }

    struct noravm_section* vm_data_struct = NULL;
    err = noravm_image_add_section(&vm_data_struct, data, NORAVM_SECT_BSS, pagesize, 0, NULL, state_size);
    if (err != 0) {
        return err;
    }
    image->noravm_entry.stack_size = stack_size;
    image->noravm_entry.data_addr = vm_data_struct->segment->vm_start + vm_data_struct->vm_offset_to_seg;

    image->noravm_entry.mem_addr = vm_memory->segment->vm_start + vm_memory->vm_offset_to_seg;
    image->noravm_entry.mem_size = vm_memory->vm_size;
    image->noravm_entry.code_addr = vm_memory->segment->vm_start + vm_memory->vm_offset_to_seg;
    image->noravm_entry.code_size = vm_memory->size;
    

    return 0;
}

