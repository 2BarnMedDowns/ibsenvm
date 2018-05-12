#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ibsen/list.h>
#include <ibsen/image.h>
#include <noravm/state.h>
#include <noravm/entry.h>

#ifdef __APPLE__
#include <ibsen/macho.h>
#endif


#define ALIGN(size, alignment) \
    (((size) + (alignment) - 1) & ~((alignment) - 1))



static int create_image(struct ibsen_image** handle, enum ibsen_image_type type)
{
    *handle = NULL;

    struct ibsen_image* image = malloc(sizeof(struct ibsen_image));
    if (image == NULL) {
        return errno;
    }

    image->type = type;
    ibsen_list_init(&image->segments);
    image->state.start = NULL;
    image->state.interrupt = NULL;
    image->state.handler = NULL;
    image->state_addr = 0;
    image->ep_vm_addr = 0;
    image->ep_file_offset = 0;
    image->file_size = 0;

    *handle = image;
    return 0;
}



static void remove_section(struct ibsen_section* section)
{
    ibsen_list_remove(&section->list);
    free(section);
}



static void remove_segment(struct ibsen_segment* segment)
{
    ibsen_list_for_each(sect_it, &segment->sections) {
        struct ibsen_section* sect = container_of(sect_it, struct ibsen_section, list);
        remove_section(sect);
    }

    ibsen_list_remove(&segment->list);
    free(segment);
}



static int add_segment(struct ibsen_segment** handle, struct ibsen_image* image, enum ibsen_segment_type type, size_t size)
{
    struct ibsen_segment* segment = malloc(sizeof(struct ibsen_segment));
    
    if (segment == NULL) {
        return errno;
    }

    const struct ibsen_segment* prev = ibsen_list_last(&image->segments, struct ibsen_segment, list);
    segment->image = image;
    ibsen_list_insert_back(&segment->list, &image->segments);
    segment->type = type;
    segment->page_size = sysconf(_SC_PAGESIZE);
    segment->vm_addr = prev != NULL ? prev->vm_addr + prev->vm_size : 0;
    segment->vm_size = size;
    ibsen_list_init(&segment->sections);
    segment->file_start = image->file_size;
    segment->file_size = 0;

    *handle = segment;
    return 0;
}



static int add_section(struct ibsen_section** handle, 
                       struct ibsen_segment* segment, 
                       enum ibsen_section_type type, 
                       const void* data,
                       size_t size)
{
    struct ibsen_section* section = malloc(sizeof(struct ibsen_section));

    if (section == NULL) {
        return errno;
    }

    const struct ibsen_section* prev = ibsen_list_last(&segment->sections, struct ibsen_section, list);
    section->segment = segment;
    ibsen_list_insert_back(&section->list, &segment->sections);
    section->type = type;
    section->vm_offset = prev != NULL ? prev->vm_offset + prev->vm_size : 0;
    section->vm_size = ALIGN(size, segment->page_size);
    section->data = data;

    // Increase file pointers
    struct ibsen_image* image = segment->image;
    section->file_size = size;
    section->file_padding = section->vm_size - section->file_size;
    section->file_offset = image->file_size;

    image->file_size += section->file_size + section->file_padding;
    segment->file_size += section->file_size + section->file_padding;

    *handle = section;
    return 0;
}



static int load_noravm_code(struct ibsen_segment* segment, const struct noravm_info* info)
{
    int err;

    struct ibsen_section* loader;
    err = add_section(&loader, segment, SECTION_RE, info->loader.addr, info->loader.size);
    if (err != 0) {
        return err;
    }

    struct ibsen_section* vm;
    err = add_section(&vm, segment, SECTION_RE, info->start.addr, info->start.size);
    if (err != 0) {
        return err;
    }

    struct ibsen_section* intr;
    err = add_section(&intr, segment, SECTION_RE, info->interrupt.addr, info->interrupt.size);
    if (err != 0) {
        return err;
    }

    struct ibsen_section* fault;
    err = add_section(&fault, segment, SECTION_RE, info->fault.addr, info->fault.size);
    if (err != 0) {
        return err;
    }

    struct ibsen_image* image = segment->image;
    image->state.start = (noravm_start_t) (segment->vm_addr + vm->vm_offset);
    image->state.interrupt = (noravm_intr_t) (segment->vm_addr + intr->vm_offset);
    image->state.handler = (noravm_exc_t) (segment->vm_addr + fault->vm_offset);

    image->ep_vm_addr = segment->vm_addr + loader->vm_offset;
    image->ep_file_offset = loader->file_offset;

    return 0;
}



static int load_noravm_data(struct ibsen_segment* segment, size_t bytecode_size)
{
    int err;

    struct ibsen_section* vm_state;
    err = add_section(&vm_state, segment, SECTION_RW, &segment->image->state, sizeof(struct noravm));
    if (err != 0) {
        return err;
    }

    struct ibsen_section* vm_bytecode;
    err = add_section(&vm_bytecode, segment, SECTION_BC, NULL, bytecode_size);
    if (err != 0) {
        return err;
    }

    segment->image->state_addr = segment->vm_addr + vm_state->vm_offset;

    uint64_t addr = segment->vm_addr + vm_bytecode->vm_offset;
    size_t total_size = segment->vm_size - segment->page_size;
    size_t size_per_seg = total_size / NORAVM_MAX_SEGS;
    struct noravm* state = &segment->image->state;

    for (size_t i = 0; i < NORAVM_MAX_SEGS; ++i) {
        state->segments[i].flags = NORAVM_SEG_READ | NORAVM_SEG_WRITE | NORAVM_SEG_EXECUTABLE;
        state->segments[i].vmaddr = addr + size_per_seg * i;
        state->segments[i].vmsize = size_per_seg;
        state->segments[i].offset = 0;
        state->segments[i].size = size_per_seg;
    }

    return 0;
}



int ibsen_create_macos_image(struct ibsen_image** handle, const struct noravm_info* vm_info, size_t bc_size)
{
    *handle = NULL;
    struct ibsen_image* image = NULL;

    int err = create_image(&image, IMAGE_MACOS);
    if (err != 0) {
        return err;
    }

    // Make lower 2 GB inaccessible
    struct ibsen_segment* null_segment;
    err = add_segment(&null_segment, image, SEGMENT_NULL, 4ULL << 30);
    if (err != 0) {
        ibsen_destroy_image(image);
        return err;
    }

    // Create segment containing the virtual machine code
    size_t vm_code_size = vm_info->start.size + vm_info->interrupt.size + vm_info->fault.size + vm_info->loader.size;
    struct ibsen_segment* vm_code;
    err = add_segment(&vm_code, image, SEGMENT_CODE, ALIGN(vm_code_size, 1ULL << 30));
    if (err != 0) {
        ibsen_destroy_image(image);
        return err;
    }

    err = load_noravm_code(vm_code, vm_info);
    if (err != 0) {
        ibsen_destroy_image(image);
        return err;
    }

    // Create segment containing the virtual machine data
    size_t page_size = sysconf(_SC_PAGESIZE);
    struct ibsen_segment* vm_data;
    //err = add_segment(&vm_data, image, SEGMENT_DATA, (1ULL << 30) * NORAVM_MAX_SEGS + page_size);
    err = add_segment(&vm_data, image, SEGMENT_DATA, (1ULL << 30) * NORAVM_MAX_SEGS);
    if (err != 0) {
        ibsen_destroy_image(image);
        return err;
    }

    err = load_noravm_data(vm_data, bc_size);
    if (err != 0) {
        ibsen_destroy_image(image);
        return err;
    }

    *handle = image;
    return 0;
}



void ibsen_destroy_image(struct ibsen_image* image)
{
    ibsen_list_for_each(seg_it, &image->segments) {
        struct ibsen_segment* seg = container_of(seg_it, struct ibsen_segment, list);
        remove_segment(seg);
    }

    free(image);
}


int ibsen_write_image(FILE* fp, struct ibsen_image* image, const void* bytecode)
{
    switch (image->type) {
#ifdef __APPLE__
        case IMAGE_MACOS:
            return ibsen_write_macho(fp, image, bytecode);
#endif

        default:
            return EINVAL;
    }
}

