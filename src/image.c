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



static void grow_segment(struct noravm_segment* segment, size_t grow)
{
    grow = NORAVM_ALIGN_ADDR(grow, segment->alignment);
    segment->vm_size += grow;

    size_t prev_end = segment->vm_start + segment->vm_size;

    struct noravm_segment* ptr = noravm_list_next(&segment->image->segments, segment);
    while (ptr != NULL) {
        ptr->vm_start = NORAVM_ALIGN_ADDR(prev_end, ptr->alignment);
        prev_end = ptr->vm_start + ptr->vm_size;
        // FIXME: set offset to previous to match gap
        // FIXME: move file offsets
        
        ptr = noravm_list_next(&segment->image->segments, ptr);
    }
}


int noravm_image_create(struct noravm_image** handle, uint64_t start_addr)
{
    struct noravm_image* image = malloc(sizeof(struct noravm_image));
    if (image == NULL) {
        return errno;
    }

    memset(image->noravm_entry.id, 0, sizeof(image->noravm_entry));
    image->noravm_entry.mem_addr = 0;
    image->noravm_entry.mem_size = 0;
    image->noravm_entry.machine_addr = 0;
    image->noravm_entry.intr_addr = 0;


    noravm_list_init(&image->segments);
    image->noravm_file_offset = 0;
    image->vm_start = start_addr;
    image->file_size = 0;

    *handle = image;
    return 0;
}



void noravm_image_remove(struct noravm_image* image)
{
    noravm_list_foreach(struct noravm_segment, seg, &image->segments) {
        noravm_image_remove_segment(seg);
    }

    free(image);
}



void noravm_image_remove_segment(struct noravm_segment* segment)
{
    noravm_list_foreach(struct noravm_section, sect, &segment->sections) {
        noravm_list_remove(sect);
        free(sect);
    }

    free(segment);
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
    noravm_list_insert(&image->segments, segment);
    segment->type = type;
    segment->alignment = align;
    segment->vm_start = prev != NULL ? prev->vm_start + prev->vm_size : image->vm_start;
    segment->vm_size = NORAVM_ALIGN_ADDR(size, align);
    segment->file_start = image->file_size;
    segment->file_offset_to_prev = 0; // FIXME: set offset to previous to match alignment
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
    noravm_list_insert(&segment->sections, section);
    section->type = type;
    section->vm_size = NORAVM_ALIGN_ADDR(size, vm_align);
    section->vm_offset_to_seg = prev != NULL ? prev->vm_offset_to_seg + prev->vm_size : 0;
    section->vm_offset_to_prev = 0;
    section->size = size;
    section->data = data;

    // Adjust memory pointers
    if (segment->vm_start + section->vm_offset_to_seg + section->vm_size > segment->vm_start + segment->vm_size) {
        //grow_segment(segment, (segment->vm_start + section->vm_offset_to_seg + section->vm_size) - (segment->vm_start + section->vm_size));
        free(section);
        return ENOSPC;
    }

    // Adjust file pointers
    if (file_align > 0) {
        struct noravm_image* image = segment->image;
        section->file_start = image->file_size;
        section->file_offset_to_seg = prev != NULL ? prev->file_offset_to_seg + prev->size + prev->file_padding : 0;
        section->file_offset_to_prev = 0;
        section->file_padding = NORAVM_ALIGN_ADDR(size, file_align) - size;

        image->file_size += section->size + section->file_padding;
        segment->file_size += section->size + section->file_padding;
    }

    if (handle != NULL) {
        *handle = section;
    }
    return 0;
}



int noravm_image_load_vm_entry(struct noravm_segment* segment, const struct noravm_functions* funcs, size_t bytecode_size)
{
    int err;

    struct noravm_image* image = segment->image;

    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize < 0) {
        return errno;
    }

    strcpy(image->noravm_entry.id, funcs->id);

    struct noravm_section* loader = NULL;
    err = noravm_image_add_section(&loader, segment, NORAVM_SECT_CODE, pagesize, pagesize, (void*) funcs->loader.addr, funcs->loader.size);
    if (err != 0) {
        return err;
    }
    image->noravm_file_offset = loader->file_start;

    struct noravm_section* entry_point = NULL;
    err = noravm_image_add_section(&entry_point, segment, NORAVM_SECT_TEXT, pagesize, pagesize, &image->noravm_entry, sizeof(struct noravm_entry_point));
    if (err != 0) {
        return err;
    }

    struct noravm_section* machine = NULL;
    err = noravm_image_add_section(&machine, segment, NORAVM_SECT_CODE, pagesize, pagesize, (void*) funcs->vm.addr, funcs->vm.size);
    if (err != 0) {
        return err;
    }
    image->noravm_entry.machine_addr = machine->segment->vm_start + machine->vm_offset_to_seg;

    struct noravm_section* interrupt = NULL;
    err = noravm_image_add_section(&interrupt, segment, NORAVM_SECT_CODE, pagesize, pagesize, (void*) funcs->intr.addr, funcs->intr.size);
    if (err != 0) {
        return err;
    }
    image->noravm_entry.intr_addr = interrupt->segment->vm_start + interrupt->vm_offset_to_seg;

    struct noravm_section* code = NULL;
    err = noravm_image_add_section(&code, segment, NORAVM_SECT_BYTECODE, pagesize, pagesize, NULL, bytecode_size);
    if (err != 0) {
        return err;
    }
    image->noravm_entry.code_addr = code->segment->vm_start + code->vm_offset_to_seg;
    image->noravm_entry.code_size = code->size;
   
    return 0;
}



int noravm_image_set_vm_data_size(struct noravm_image* image, size_t data_size)
{
    int err;

    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize < 0) {
        return errno;
    }

    struct noravm_segment* data = NULL;
    err = noravm_image_add_segment(&data, image, NORAVM_SEG_DATA, pagesize, data_size + pagesize);
    if (err != 0) {
        return err;
    }

    struct noravm_section* vm_data_struct = NULL;
    err = noravm_image_add_section(&vm_data_struct, data, NORAVM_SECT_BSS, pagesize, 0, NULL, pagesize);
    if (err != 0) {
        return err;
    }
    image->noravm_entry.stack_size = 32 * sizeof(struct noravm_state);
    image->noravm_entry.data_addr = vm_data_struct->segment->vm_start + vm_data_struct->vm_offset_to_seg;
    
    struct noravm_section* vm_memory = NULL;
    err = noravm_image_add_section(&vm_memory, data, NORAVM_SECT_DATA, pagesize, 0, NULL, data_size);
    if (err != 0) {
        return err;
    }
    image->noravm_entry.mem_addr = vm_memory->segment->vm_start + vm_memory->vm_offset_to_seg;
    image->noravm_entry.mem_size = vm_memory->vm_size;
    
    return 0;
}

