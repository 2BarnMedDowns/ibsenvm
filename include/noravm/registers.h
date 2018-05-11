#ifndef __NORAVM_REGISTERS_H__
#define __NORAVM_REGISTERS_H__


#include <stdint.h>


/*
 * Registers used by the Nora virtual machine.
 */
struct __attribute__((aligned(16))) noravm_regs
{
    uint16_t    is;     // Instruction segment
    uint32_t    ip;     // Current instruction pointer
    uint16_t    sp;     // Current segment pointer
    uint32_t    r[256]; // General purpose registers
};

#endif /* __NORAVM_REGISTERS_H__ */
