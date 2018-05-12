#ifndef __NORAVM_INTERRUPT_H__
#define __NORAVM_INTERRUPT_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


/*
 * Exceptions that can occur in the virtual machine.
 */
enum 
{
    NORAVM_INTR_DUMP_STATE          = 0x0,
    NORAVM_INTR_WRITE               = 0x1,
    NORAVM_INTR_READ                = 0x2,
    NORAVM_INTR_DIVIDE_BY_ZERO      = 0x9,
    NORAVM_INTR_OUT_OF_BOUNDS       = 0xa,
    NORAVM_INTR_PROTECTION_FAULT    = 0xb,
    NORAVM_INTR_SEGMENT_NOT_PRESENT = 0xc,
    NORAVM_INTR_USER_EXCEPTION      = 0xd,
    NORAVM_INTR_ILLEGAL_STATE       = 0xe,
    NORAVM_INTR_INVALID_OPCODE      = 0xf
};


/* 
 * Indicates if an exception is maskable.
 */
#define NORAVM_INTR_IS_MASKABLE(i) ((i) <= 0xa)


/* Forward declarations */
struct noravm;
struct noravm_seg;
struct noravm_regs;


/*
 * Callback type for the Nora VM interrupt routine.
 * Use this to raise an interrupt.
 */
typedef void (*noravm_intr_t)(struct noravm*, uint16_t intr_no);



/* 
 * Nora VM exception handler callback type.
 * Should return true if the interrupt was masked/handled.
 * If false is returned, the VM will abort.
 */
typedef bool (*noravm_exc_t)(const struct noravm* vm_state, // Current state of the virtual machine 
                             struct noravm_regs* vm_regs,   // Current machine registers 
                             struct noravm_seg* mem_segs,   // Pointer to segment list
                             uint16_t intr_no);             // Raised interrupt.

#ifdef __cplusplus
}
#endif
#endif /* __NORAVM_INTERRUPT_H__ */
