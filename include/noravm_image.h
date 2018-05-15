#ifndef __NORAVM_MEMORY_IMAGE_H__
#define __NORAVM_MEMORY_IMAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <noravm_vm.h>
#include <noravm_list.h>



/* 
 * Convenience macro for aligning addresses to a specified alignment.
 * Alignment must be a power of two.
 */
#define NORAVM_ALIGN_ADDR(size, alignment) (((uint64_t) (size) + (alignment) - 1) & ~(((uint64_t) (alignment) - 1)))


/*
 * Intermediate image format.
 * Use this structure to create a structure representing a complete image,
 * describing memory segments and where in memory to place the Nora 
 * virtual machine and related functions.
 */
struct noravm_image
{
    struct noravm_entry_point   noravm_entry;       // VM entry point and function addresses
    size_t                      noravm_file_offset; // VM entry point location in file
    void*                       vm_code;            // Code of the VM
    uint64_t                    vm_entry_point;     // Address of the entry point
    uint64_t                    vm_start;           // Starting address of image
    size_t                      file_size;          // Total file size of image
    size_t                      num_segments;       // Number of segments in image
    size_t                      num_sections;       // Number of sections in image
    size_t                      page_size;          // System page size
    struct noravm_list          segments;           // List of segments
};


/*
 * Memory segment types.
 */
enum noravm_segment_type
{
    NORAVM_SEG_NULL,
    NORAVM_SEG_NORAVM,
    NORAVM_SEG_DATA
};


/*
 * Memory segment descriptor.
 * This structure describes a segment of memory and where it should be 
 * located once the image is loaded.
 */
struct noravm_segment
{
    enum noravm_segment_type    type;               // Segment type
    struct noravm_image*        image;              // Parent reference
    struct noravm_list          list;               // Linked list node
    size_t                      vm_align;           // VM alignment
    uint64_t                    vm_start;           // Virtual memory address of the segment
    size_t                      vm_size;            // Size of segment in virtual memory
    size_t                      file_start;         // Absolute position in file
    size_t                      file_offset_to_prev;// Offset in file from previous segment
    size_t                      file_size;          // File size
    struct noravm_list          sections;           // List of sections
};


/*
 * Memory sections.
 */
enum noravm_section_type
{
    NORAVM_SECT_DATA,
    NORAVM_SECT_TEXT,
    NORAVM_SECT_CODE,
    NORAVM_SECT_BYTECODE,
    NORAVM_SECT_BSS,
    NORAVM_SECT_CONST
};


/*
 * Memory section descriptor.
 */
struct noravm_section
{
    enum noravm_section_type    type;               // Section type
    struct noravm_segment*      segment;            // Parent reference
    struct noravm_list          list;               // Linked list node
    uint64_t                    vm_offset_to_seg;   // Relative offset to segment start
    uint64_t                    vm_offset_to_prev;  // Relative offset to previous section
    size_t                      vm_align;           // Memory alignment
    size_t                      vm_size;            // Size of section in virtual memory
    size_t                      size;               // Size of data
    const void*                 data;               // Pointer to section data
    size_t                      file_offset_to_seg; // Offset to segment in file
    size_t                      file_offset_to_prev;// Offset to previous section in file
    size_t                      file_align;         // File alignment (NB! if 0, section has no content)
    size_t                      file_padding;       // Padding in file to meet alignment requirements
};


/*
 * Allocate a new image structure.
 */
int noravm_image_create(struct noravm_image** image, uint64_t start_addr);


/*
 * Delete image and free resources.
 * This will also recursively destroy any associated segments and sections.
 */
void noravm_image_remove(struct noravm_image* image);



/*
 * Create a memory segment and add it to the image.
 */
int noravm_image_add_segment(struct noravm_segment** segment, 
                             struct noravm_image* image, 
                             enum noravm_segment_type type, 
                             size_t vm_align,
                             size_t vm_size);



/*
 * Create a section and add it to a segment.
 */
int noravm_image_add_section(struct noravm_section** section,
                             struct noravm_segment* segment,
                             enum noravm_section_type type,
                             size_t vm_align,
                             size_t file_align,
                             const void* data,
                             size_t size);


/*
 * Load the code of the Nora virtual machine in to memory.
 */
int noravm_image_load_vm(struct noravm_image* image,
                         const struct noravm_functions* funcs,
                         size_t vm_align,
                         size_t code_align);



/*
 * Reserve memory segment for virtual machine data.
 */
int noravm_image_reserve_vm_data(struct noravm_image* image, 
                                 uint64_t data_addr,
                                 size_t data_size, 
                                 size_t stack_entries,
                                 size_t bytecode_size);

#ifdef __cplusplus
}
#endif
#endif /* __NORAVM_MEMORY_IMAGE_H__ */
