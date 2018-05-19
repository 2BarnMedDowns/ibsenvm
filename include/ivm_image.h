#ifndef __IBSENVM_FILE_IMAGE_H__
#define __IBSENVM_FILE_IMAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <ivm_vm.h>
#include <ivm_entry.h>
#include <ivm_list.h>



/* 
 * Convenience macro for aligning addresses to a specified alignment.
 * Alignment must be a power of two.
 */
#define IVM_ALIGN_ADDR(size, alignment) \
    (((uint64_t) (size) + (alignment) - 1) & ~(((uint64_t) (alignment) - 1)))


/*
 * Intermediate image format.
 * Use this structure to create a structure representing a complete image,
 * describing memory segments and where in memory to place the Ibsen virtual
 * machine and related functions.
 */
struct ivm_image
{
    struct ivm_data*            data;               // Initial VM data
    size_t                      data_size;          // Size of VM data
    size_t                      data_offset_to_regs;  // Offset to registers
    size_t                      data_offset_to_states; // Offset to states
    size_t                      data_offset_to_ft;  // Offset to frame table
    size_t                      data_offset_to_ct;  // Offset to call table
    size_t                      vm_file_offset;     // Offset in image file to entry point
    void*                       vm_code;            // Code of the VM
    uint64_t                    vm_entry_point;     // Address of the entry point
    size_t                      file_size;          // Total file size of image
    size_t                      num_segments;       // Number of segments in image
    size_t                      num_sections;       // Number of sections in image
    size_t                      page_size;          // System page size
    struct ivm_list             segments;           // List of segments
};


/*
 * Memory segment types.
 */
enum ivm_segment_type
{
    IVM_SEG_NULL,       // Segment contains nothing
    IVM_SEG_CODE,       // Segment contains VM code
    IVM_SEG_DATA,       // Segment contains VM data
};


/*
 * Memory segment descriptor.
 * This structure describes a segment of memory and where it should be 
 * located once the image is loaded.
 */
struct ivm_segment
{
    enum ivm_segment_type       type;               // Segment type
    struct ivm_image*           image;              // Parent reference
    struct ivm_list             list;               // Linked list node
    size_t                      vm_align;           // VM alignment
    uint64_t                    vm_start;           // Virtual memory address of the segment
    size_t                      vm_size;            // Size of segment in virtual memory
    size_t                      file_start;         // Absolute position in file
    size_t                      file_align;         // File alignment
    size_t                      file_size;          // File size
    struct ivm_list             sections;           // List of sections
};


/*
 * Memory sections.
 */
enum ivm_section_type
{
    IVM_SECT_DATA,
    IVM_SECT_TEXT,
    IVM_SECT_CODE,
    IVM_SECT_BYTECODE,
    IVM_SECT_BSS,
    IVM_SECT_CONST
};


/*
 * Memory section descriptor.
 */
struct ivm_section
{
    enum ivm_section_type       type;               // Section type
    struct ivm_segment*         segment;            // Parent reference
    struct ivm_list             list;               // Linked list node
    size_t                      size;               // Size of data
    const void*                 data;               // Pointer to section data
    size_t                      vm_size;            // Size of section in virtual memory
    size_t                      vm_align;           // Alignment in memory
    uint64_t                    vm_offset_to_seg;   // Relative offset to segment start
    uint64_t                    vm_offset_to_prev;  // Relative offset to previous section
    size_t                      file_offset_to_seg; // Offset to segment start in file
    size_t                      file_offset_to_prev;// Offset to previous section in file
    size_t                      file_padding;       // Padding in file to meet alignment requirements
};


/*
 * Allocate a new image structure.
 */
int ivm_image_create(struct ivm_image** image, 
                     size_t vm_state_stack_size,
                     size_t vm_frame_size,
                     size_t vm_total_num_frames);


/*
 * Delete image and free resources.
 * This will also recursively destroy any associated segments and sections.
 */
void ivm_image_remove(struct ivm_image* image);



/*
 * Create a memory segment and add it to the image.
 */
int ivm_image_add_segment(struct ivm_segment** segment, 
                          struct ivm_image* image, 
                          enum ivm_segment_type type, 
                          size_t vm_align,
                          uint64_t vm_addr,
                          size_t vm_size,
                          size_t file_align);



/*
 * Create a section and add it to a segment.
 */
int ivm_image_add_section(struct ivm_section** section,
                          struct ivm_segment* segment,
                          enum ivm_section_type type,
                          size_t vm_align,
                          const void* data,
                          size_t size);


/*
 * Load the code of the Nora virtual machine in to memory.
 */
int ivm_image_load_vm(struct ivm_image* image,
                      const struct ivm_vm_functions* funcs,
                      uint64_t code_addr);



/*
 * Reserve memory segment for virtual machine data.
 */
int ivm_image_reserve_vm_data(struct ivm_image* image, 
                              uint64_t data_addr,
                              size_t bytecode_size);



/*
 * Write image to file.
 */
int ivm_image_write(FILE* fp, const struct ivm_image* image, const void* bytecode);



#ifdef __cplusplus
}
#endif
#endif /* __IBSENVM_MEMORY_IMAGE_H__ */
