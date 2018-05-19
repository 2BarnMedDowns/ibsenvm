#ifndef __IBSENVM_INTERRUPT_H__
#define __IBSENVM_INTERRUPT_H__

#include <stddef.h>
#include <stdint.h>

/* Forward declaration */
struct ivm_frame;
struct ivm_data;



/*
 * Exceptions that can occur in the Ibsen virtual machine.
 * A guest program can specify interrupt vectors for maskable interrupts.
 * Non-maskable interrupts will abort the VM.
 */
enum 
{
    IVM_INTR_DEBUG                  = 0x0,  // Reserved for future use
    IVM_INTR_USER_DEFINED           = 0x1,  // User defined interrupt vector
    IVM_INTR_RESERVED0              = 0x2,  // Reserved for future use
    IVM_INTR_RESERVED1              = 0x3,  // Reserved for future use
    IVM_INTR_RESERVED2              = 0x4,  // Reserved for future use
    IVM_INTR_RESERVED3              = 0x5,  // Reserved for future use
    IVM_INTR_ARITHMETIC_ERROR       = 0x6,  // Arithmetical error, e.g. division by zero or arithmetic overflow
    IVM_INTR_INVALID_OPCODE         = 0x7,  // Malformed instruction
    IVM_INTR_IO_ERROR               = 0x8,  // Unexpected I/O error, for example invalid file descriptor
    IVM_INTR_FRAME_FAULT            = 0x9,  // Virtual machine memory region is not present or access error
    IVM_INTR_STACK_OVERFLOW         = 0xa,  // Operation would overflow stack pointer
    IVM_INTR_SYSCALL                = 0xb,  // Call system function
    IVM_INTR_PROTECTION_FAULT       = 0xc,  // Illegal memory operation
    IVM_INTR_EXCEPTION_OVERFLOW     = 0xd,  // State stack overflow, which happens when too many interrupts are raised
    IVM_INTR_EXTERNAL_EVENT         = 0xe,  // Signal was sent outside virtual machine
    IVM_INTR_ILLEGAL_STATE          = 0xf,  // Virtual machine is in an illegal state
};



/* 
 * Indicates if an exception is maskable.
 * Non-maskable interrupts, except syscalls, will cause the virtual machine to abort.
 */
#define IVM_INTR_NONMASKABLE \
    ((1 << IVM_INTR_ILLEGAL_STATE) | (1 << IVM_INTR_EXTERNAL_EVENT) | (1 << IVM_INTR_EXCEPTION_OVERFLOW) | (1 << IVM_INTR_PROTECTION_FAULT) | (1 << IVM_INTR_CALL_SYSCALL))


#define IVM_INTR_IS_MASKABLE(i) !(IVM_INTR_NONMASKABLE & (1 << (i)))


/*
 * Check if an interrupt is raised.
 */
#define IVM_INTR_IS_SET(registers, i) !!((registers)->intr & (1 << (i)))



/*
 * Default interrupt mask.
 * Masked interrupts are ignored.
 * Unmasked interrupts will invoke the interrupt routine.
 */
#define IVM_INTR_DEFAULT_MASK    0xffff



/*
 * Clear an interrupt from the interrupt bitfield.
 */
#define IVM_INTR_CLEAR(registers, i)   \
    do { \
        (registers)->intr &= (~(1 << (i)) | IVM_INTR_NONMASKABLE); \
    } while (0)



/*
 * Interrupt handler routine signature.
 * 
 * Before the interrupt routine is invoked, the interrupt bit will be set
 * and the current state will be stored on the internal state stack.
 *
 * For non-maskable interrupts:
 * - If the interrupt is a syscall, execute the syscall and return normally.
 * - Otherwise, abort the VM.
 * 
 * For maskable interrupts:
 * - If the interrupt is masked, ignore the interrupt.
 * - If the interrupt is unmasked, but no vector is set, abort the VM.
 * - If the interrupt is unmasked and a vector is set, set up the
 *   guest vector.
 *
 * Once the routine returns, the VM will pop the state off the internal state
 * stack and resume execution.
 */
typedef void (*ivm_interrupt_t)(struct ivm_data* vm, int file, uint64_t addr);


#endif /* __IBSENVM_INTERRUPT_H__ */
