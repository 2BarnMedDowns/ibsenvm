#ifndef __IBSENVM_MEMORY_H__
#define __IBSENVM_MEMORY_H__

#include <stdint.h>



/*
 * The Ibsen Virtual Machine divides memory into 256 byte-sized memory
 * regions, or "frames", that work like a pseudo-paging mechanism and allows 
 * IVM to allocate memory dynamically on demand.
 *
 * It also allows a program running in IVM to control memory with access
 * rights, and also means that memory can be backed by a file.
 */
struct __attribute__((aligned (16))) ivm_frame
{
    uint64_t    addr;   // "Physical" address of the frame
    uint16_t    attr;   // Frame attributes
    int16_t     file;   // File descriptor (>= 0 if backed by file)
    uint32_t    offs;   // File offset (file >= 0 if backed by file)
};



/*
 * Frame attributes.
 */
enum 
{
    IVM_FRAME_ATTR_READ             = 0x0001, // IVM guest can read from frame
    IVM_FRAME_ATTR_WRITE            = 0x0002, // IVM guest can write to frame
    IVM_FRAME_ATTR_EXEC             = 0x0004, // IVM guest can execute code contained in frame
    IVM_FRAME_ATTR_ALLOC            = 0x0008, // Frame is loaded and present in memory
    IVM_FRAME_ATTR_ALLOC_ON_FAULT   = 0x0010, // Frame should be loaded if not present
    IVM_FRAME_ATTR_ZERO_ON_ALLOC    = 0x0020, // Frame should be zeroed out on load
    IVM_FRAME_ATTR_STALE            = 0x0040, // Frame contains data that must be saved on free
};




/*
 * Get the frame number.
 */
#define IVM_FNUM(addr, fnum_shift) (((uint32_t) (addr)) >> (fnum_shift))



/*
 * Get the offset within a frame.
 */
#define IVM_FOFF(addr, fnum_shift) (((uint32_t) (addr)) & ((1UL << (fnum_shift)) - 1))



/*
 * Get the frame from the frame table.
 */
/*#define IVM_FRAME(ftbl, addr) \
//    ((struct ivm_frame*) ( \
//        (IVM_FNUM(addr, (ftbl)->fnum_shift) < (ftbl)->num_entries) \
//        ? &(ftbl)->entries[IVM_FNUM(addr, (ftbl)->fnum)] \
//        : NULL))
*/


#endif /* __IBSENVM_MEMORY_H__ */
