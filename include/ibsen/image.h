#ifndef __IBSEN_MEMORY_IMAGE_H__
#define __IBSEN_MEMORY_IMAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <ibsen/list.h>
#include <noravm/entry.h>
#include <noravm/state.h>


enum ibsen_image_type
{
    IMAGE_MACOS, 
    IMAGE_LINUX
};


/*
 * Segment types.
 *  NULL = The first 4 gigabytes of memory are inaccessible
 *  CODE = Segment containing executable read-only data
 *  TEXT = Segment containing executable read-only data
 *  DATA = Segment is both readable and writeable but not executable
 */
enum ibsen_segment_type
{
    SEGMENT_NULL,
    SEGMENT_CODE,
    SEGMENT_TEXT,
    SEGMENT_DATA
};


/*
 * Sections represent Nora virtual memory segments.
 *
 *  BC  = Section is byte code
 *  RW  = Section is readable and writeable but not executable
 *  RE  = Section is readable and executable
 */
enum ibsen_section_type
{
    SECTION_BC,
    SECTION_RW,
    SECTION_RE
};


/*
 * Memory image layout.
 */
struct ibsen_image
{
    enum ibsen_image_type   type;           // Memory image type
    struct ibsen_list       segments;       // Memory segments
    struct noravm           state;          // Virtual machine state struct
    uint64_t                state_addr;     // Address to the state struct
    uint64_t                ep_vm_addr;     // Virtual machine entry point
    size_t                  ep_file_offset; // Offset in file
    size_t                  file_size;      // Total file size
};


struct ibsen_segment
{
    struct ibsen_image*     image;
    struct ibsen_list       list;
    enum ibsen_segment_type type;
    size_t                  page_size;
    uint64_t                vm_addr;
    size_t                  vm_size;
    struct ibsen_list       sections;
    size_t                  file_start;
    size_t                  file_size;
};


struct ibsen_section
{
    struct ibsen_segment*   segment;
    struct ibsen_list       list;
    enum ibsen_section_type type;
    size_t                  vm_offset;
    size_t                  vm_size;
    const void*             data;
    size_t                  file_padding;
    size_t                  file_size;
    size_t                  file_offset;
};



int ibsen_create_macos_image(struct ibsen_image** image, 
                             const struct noravm_info* vm_info,
                             size_t bytecode_length);


void ibsen_destroy_image(struct ibsen_image* image);


int ibsen_write_image(FILE* fp, struct ibsen_image* image, const void* bytecode_ptr);


#ifdef __cplusplus
}
#endif
#endif /* __IBSEN_IMAGE_H__ */
