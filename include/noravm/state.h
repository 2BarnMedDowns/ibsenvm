#ifndef __NORAVM_STATE_H__
#define __NORAVM_STATE_H__

#include <noravm/interrupt.h>
#include <noravm/segment.h>
#include <noravm/registers.h>
#include <stddef.h>
#include <stdint.h>

#define NORAVM_MAX_STATES   4


/*
 * Possible states of the Nora virtual machine.
 */
enum 
{
    NORAVM_STATE_ABORT          = 0x00,
    NORAVM_STATE_OPCODE         = 0x01,
    NORAVM_STATE_OPERANDS       = 0x02,
    NORAVM_STATE_WORD           = 0x03,
    NORAVM_STATE_EXECUTE        = 0x04,
    NORAVM_STATE_EXCEPTION      = 0x05
};


/*
 * Nora virtual machine instruction.
 */
struct __attribute__((aligned (16))) noravm_instr
{
    uint8_t             opcode;
    uint8_t             num_operands;
    uint8_t             operands[4];
    uint32_t            word;
};



/*
 * The Nora virtual machine state machine.
 */
struct __attribute__((aligned (16))) noravm
{
    noravm_intr_t       interrupt;                  // Interrupt routine
    noravm_exc_t        handler;                    // Exception handler
    uint16_t            mask;                       // Interrupt mask
    uint8_t             state;                      // Current state
    uint8_t             stack[NORAVM_MAX_STATES];   // Stored states
    uint8_t             stackptr;                   // State stack pointer
    struct noravm_seg   segments[NORAVM_MAX_SEGS];  // Memory segments
    struct noravm_regs  registers;                  // Virtual machine registers
    struct noravm_instr instruction;                // Current instruction
};


#endif
