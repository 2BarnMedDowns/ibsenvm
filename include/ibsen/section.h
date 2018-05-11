#ifndef __IBSEN_SECTION_H__
#define __IBSEN_SECTION_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <ibsen/list.h>

struct ibsen_segment;


enum ibsen_section_type
{
    SECTION_NORAVM_BYTECODE,
    SECTION_NORAVM_ENTRY_POINT,
    SECTION_NORAVM_DATA_SEGMENT
};


struct ibsen_section
{
    struct ibsen_segment*   segment;
    struct ibsen_list       list;
    enum ibsen_section_type type;
    size_t                  vm_offset;
    size_t                  vm_size;
    size_t                  size;
    unsigned char           data[];
};


int ibsen_section_create(struct ibsen_section** section,
                         enum ibsen_section_type type,
                         size_t size);


void ibsen_section_destroy(struct ibsen_section* section);


void ibsen_section_add_to_segment(struct ibsen_segment* segment,
                                  struct ibsen_section* section,
                                  size_t offset,
                                  size_t virtual_size);

#ifdef __cplusplus
}
#endif
#endif /* __IBSEN_SECTION_H__ */
