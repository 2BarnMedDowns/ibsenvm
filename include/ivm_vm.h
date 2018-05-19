#ifndef __IBSENVM_VIRTUAL_MACHINE_H__
#define __IBSENVM_VIRTUAL_MACHINE_H__

#include <stddef.h>
#include <stdint.h>
#include <ivm_interrupt.h>
#include <ivm_memory.h>
#include <ivm_syscall.h>


/*
 * Registers used by the Ibsen virtual machine.
 */
struct __attribute__((aligned(16))) ivm_registers
{
    uint32_t ip;        // Current instruction pointer/program counter
    uint32_t sb;        // Stack base pointer
    uint16_t sp;        // Current stack pointer
    uint32_t bp;        // Memory base offset
    uint16_t imask;     // Masked interrupts
    uint16_t intr;      // Interrupts
    uint32_t iv[16];    // Interrupt vectors
    uint32_t r[256];    // General purpose registers
};



/*
 * Ibsen VM finite state machine.
 */
struct __attribute__((aligned (16))) ivm_state
{
    uint8_t  state;         // Current state
    uint8_t  opcode;        // Instruction opcode
    uint8_t  num_operands;  // Operand count
    uint8_t  operands[4];   // Operands
    uint32_t word;          // Constant
};



/*
 * Possible states of the state machine.
 */
enum 
{
    IVM_STATE_ABORT         = 0x00,
    IVM_STATE_OPCODE        = 0x01,
    IVM_STATE_OPERANDS      = 0x02,
    IVM_STATE_WORD          = 0x03,
    IVM_STATE_EXECUTE       = 0x04,
};



/*
 * Main data structure for the Ibsen virtual machine.
 */
struct __attribute__((aligned (16))) ivm_data
{
    char                    id[16];     // Identifier string
    uint64_t                vm_addr;    // Address to the virtual machine
    struct ivm_registers*   registers;  // Virtual machine registers
    ivm_interrupt_t         interrupt;  // Interrupt routine
    size_t                  state_size; // Maximum size of the internal state stack
    size_t                  state_pos;  // Internal state stack position
    struct ivm_state*       states;     // Internal state stack
    struct ivm_frame_table* frames;     // Frame table
    struct ivm_call_table*  syscalls;   // System function table
};


#endif /* __IBSENVM_VIRTUAL_MACHINE_H__ */
