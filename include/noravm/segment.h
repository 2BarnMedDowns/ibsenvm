#ifndef __NORAVM_SEGMENT_H__
#define __NORAVM_SEGMENT_H__

#include <stddef.h>
#include <stdint.h>

#define NORAVM_MAX_SEGS 4

/*
 * Segment flags.
 */
enum 
{
    NORAVM_SEG_READ         = 0x01,
    NORAVM_SEG_WRITE        = 0x02,
    NORAVM_SEG_EXECUTABLE   = 0x04,
    NORAVM_SEG_RESIZEABLE   = 0x08,
};


/*
 * Describe a memory region used by the Nora virtual machine.
 * Each VM instance has a code segment and a data segment.
 */
struct __attribute__((aligned (16))) noravm_seg
{
    uint32_t    flags;  // Segment flags
    uint64_t    vmaddr; // Base address of segment (virtual memory)
    size_t      vmsize; // Maximum size of the segment in virtual memory
    size_t      offset; // Offset to memory that has been loaded
    size_t      size;   // Size of memory that has been loaded
};

#endif /* __NORAVM_SEGMENT_H__ */
