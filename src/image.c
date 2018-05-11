#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <ibsen/list.h>
#include <ibsen/image.h>
#include <ibsen/segment.h>
#include <ibsen/section.h>



int ibsen_image_create(struct ibsen_image** handle)
{
    struct ibsen_image* image = malloc(sizeof(struct ibsen_image));

    if (image == NULL) {
        return errno;
    }

    ibsen_list_init(&image->segments);

    *handle = image;
    return 0;
}


void ibsen_image_destroy(struct ibsen_image* image)
{
    ibsen_list_for_each(seg_it, &image->segments) {
        struct ibsen_segment* seg = container_of(seg_it, struct ibsen_segment, list);
        ibsen_segment_destroy(seg);
    }

    free(image);
}


int ibsen_segment_create(struct ibsen_image* image,
                         struct ibsen_segment** handle,
                         enum ibsen_segment_type type,
                         uint64_t start_addr)
{
    struct ibsen_segment* segment = malloc(sizeof(struct ibsen_segment));

    if (segment == NULL) {
        return errno;
    }

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 0) {
        free(segment);
        return errno;
    }

    segment->type = type;
    segment->page_size = (size_t) page_size;
    segment->vm_addr = start_addr;
    segment->vm_size = 0;
    ibsen_list_init(&segment->sections);

    ibsen_list_insert_back(&segment->list, &image->segments);
    *handle = segment;
    return 0;
}


void ibsen_segment_destroy(struct ibsen_segment* segment)
{
    ibsen_list_remove(&segment->list);

    ibsen_list_for_each(sec_it, &segment->sections) {
        struct ibsen_section* sec = container_of(sec_it, struct ibsen_section, list);
        ibsen_section_destroy(sec);
    }

    free(segment);
}


int ibsen_section_create(struct ibsen_section** handle,
                         enum ibsen_section_type type,
                         size_t size)
{
    struct ibsen_section* section = malloc(sizeof(struct ibsen_section) + size);

    if (section == NULL) {
        return errno;
    }

    section->segment = NULL;
    ibsen_list_init(&section->list);
    section->type = type;
    section->vm_offset = 0;
    section->vm_size = 0;
    section->size = size;

    *handle = section;
    return 0;
}


void ibsen_section_destroy(struct ibsen_section* section)
{
    if (section->segment != NULL) {
        section->segment->vm_size -= section->vm_size;
        ibsen_list_remove(&section->list);
    }
    free(section);
}


void ibsen_section_add_to_segment(struct ibsen_segment* segment,
                                  struct ibsen_section* section,
                                  size_t offset,
                                  size_t virtual_size)
{
    if (section->segment == NULL) {
        ibsen_list_insert_back(&section->list, &segment->sections);
        section->vm_offset = offset;
        section->vm_size = virtual_size;

        segment->vm_size += virtual_size;
        section->segment = segment;
    }
}

