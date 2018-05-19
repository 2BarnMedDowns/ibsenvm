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



int ivm_image_create(struct ivm_image** handle ,size_t stack_size, size_t region_size)
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

    memset(&image->vm_data, 0, sizeof(struct ivm_data));
    image->vm_data_file_offset = 0;
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



int ivm_image_load_vm(struct ivm_image* image, const struct ivm_vm_functions* funcs, size_t vm_align, size_t code_align)
{
    return 0;
}

//int noravm_image_load_vm(struct noravm_image* image, const struct noravm_functions* funcs, size_t vm_align, size_t code_align)
//{
//    int err;
//
//    size_t pagesize = image->page_size;
//    size_t size = pagesize // offset with one page
//        + NORAVM_ALIGN_ADDR(funcs->loader.size, code_align) 
//        + NORAVM_ALIGN_ADDR(funcs->vm.size, code_align) 
//        + NORAVM_ALIGN_ADDR(funcs->intr.size, code_align);
//
//    size = NORAVM_ALIGN_ADDR(size, pagesize);
//
//    image->vm_code = malloc(size);
//    if (image->vm_code == NULL) {
//        return errno;
//    }
//
//    // Load VM code offset by one page
//    unsigned char* loaderptr = (unsigned char*) NORAVM_ALIGN_ADDR(image->vm_code, code_align) + pagesize;
//    memcpy(loaderptr, (void*) funcs->loader.addr, funcs->loader.size);
//    unsigned char* vmptr = loaderptr + NORAVM_ALIGN_ADDR(funcs->loader.size, code_align);
//    memcpy(vmptr, (void*) funcs->vm.addr, funcs->vm.size);
//    unsigned char* intrptr = vmptr + NORAVM_ALIGN_ADDR(funcs->vm.size, code_align);
//    memcpy(intrptr, (void*) funcs->intr.addr, funcs->intr.size);
//
//    // Create code segment
//    struct noravm_segment* segment = NULL;
//    err = noravm_image_add_segment(&segment, image, NORAVM_SEG_NORAVM, vm_align, size);
//    if (err != 0) {
//        free(image->vm_code);
//        image->vm_code = NULL;
//        return err;
//    }
//
//    // Create code section
//    struct noravm_section* section = NULL;
//    err = noravm_image_add_section(&section, segment, NORAVM_SECT_CODE, 1, pagesize, ((unsigned char*) image->vm_code) + pagesize, size - pagesize);
//    if (err != 0) {
//        free(image->vm_code);
//        image->vm_code = NULL;
//        return err;
//    }
//
//    image->vm_entry_point = segment->vm_start;
//    image->noravm_file_offset = segment->file_start;
//
//    image->noravm_entry.machine_addr = segment->vm_start + pagesize + (vmptr - loaderptr);
//    image->noravm_entry.intr_addr = segment->vm_start + pagesize + (intrptr - loaderptr);
//
//    strcpy(image->noravm_entry.id, funcs->id);
//    return 0;
//}



//int noravm_image_reserve_vm_data(struct noravm_image* image, uint64_t entry_addr, size_t data_size, size_t bytecode_size)
//{
//    int err;
//
//    size_t pagesize = image->page_size;
//    size_t stack_size = image->noravm_entry.stack_size * sizeof(struct noravm_state);
//    size_t state_size = sizeof(struct noravm_data) + stack_size;
//    size_t region_size = sizeof(struct noravm_region) * (data_size / image->noravm_entry.region_size);
//
//    if (bytecode_size > data_size) {
//        return EINVAL;
//    }
//
//    struct noravm_segment* private_data = NULL;
//    err = noravm_image_add_segment(&private_data, image, NORAVM_SEG_DATA, entry_addr, pagesize + state_size + region_size);
//    if (err != 0) {
//        return err;
//    }
//
//    if (private_data->vm_start != entry_addr) {
//        return EFAULT;
//    }
//
//    struct noravm_section* entry_point = NULL;
//    err = noravm_image_add_section(&entry_point, private_data, NORAVM_SECT_ENTRY_INFO, pagesize, pagesize, &image->noravm_entry, sizeof(struct noravm_entry_point));
//    if (err != 0) {
//        return err;
//    }
//
//    struct noravm_section* vm_data_struct = NULL;
//    err = noravm_image_add_section(&vm_data_struct, private_data, NORAVM_SECT_BSS, pagesize, 0, NULL, state_size + region_size);
//    if (err != 0) {
//        return err;
//    }
//    image->noravm_entry.data_addr = vm_data_struct->segment->vm_start + vm_data_struct->vm_offset_to_seg;
//    image->noravm_entry.data_size = state_size + region_size;
//
//
//    struct noravm_segment* vm_memory = NULL;
//    err = noravm_image_add_segment(&vm_memory, image, NORAVM_SEG_DATA, pagesize, data_size);
//    if (err != 0) {
//        return err;
//    }
//
//    struct noravm_section* bytecode = NULL;
//    err = noravm_image_add_section(&bytecode, vm_memory, NORAVM_SECT_BYTECODE, pagesize, pagesize, NULL, bytecode_size);
//    if (err != 0) {
//        return err;
//    }
//
//    image->noravm_entry.mem_addr = bytecode->segment->vm_start + bytecode->vm_offset_to_seg;
//    image->noravm_entry.mem_size = vm_memory->vm_size;
//
//    return 0;
//}

