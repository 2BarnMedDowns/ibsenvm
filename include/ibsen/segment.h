#ifndef __IBSEN_SEGMENT_H__
#define __IBSEN_SEGMENT_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <ibsen/list.h>


enum ibsen_segment_type
{
    SEGMENT_NULL,
    SEGMENT_TEXT,
    SEGMENT_DATA,
    SEGMENT_NORAVM
};


struct ibsen_segment
{
    struct ibsen_list       list;
    enum ibsen_segment_type type;
    size_t                  page_size;
    uint64_t                vm_addr;
    size_t                  vm_size;
    struct ibsen_list       sections;
};


int ibsen_segment_create(struct ibsen_image* image,
                         struct ibsen_segment** segment,
                         enum ibsen_segment_type type,
                         uint64_t start_addr);


void ibsen_segment_destroy(struct ibsen_segment* segment);


#ifdef __cplusplus
}
#endif
#endif /* __IBSEN_SEGMENT_H__ */
