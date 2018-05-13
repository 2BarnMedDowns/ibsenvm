#ifndef __NORAVM_VIRTUAL_MACHINE_H__
#define __NORAVM_VIRTUAL_MACHINE_H__

#include <stddef.h>
#include <stdint.h>
#include <noravm_list.h>


/* Forward declarations */
struct noravm_region;
struct noravm_registers;


/*
 * Virtual memory region flags
 */
enum 
{
    NORAVM_MEM_FLAG_READ            = 0x01,     // Region can be read from
    NORAVM_MEM_FLAG_WRITE           = 0x02,     // Region can be written to
    NORAVM_MEM_FLAG_EXECUTABLE      = 0x04,     // Region can contain executable byte code
    NORAVM_MEM_FLAG_PRESENT         = 0x08,     // Region is loaded into memory
};


/*
 * Virtual memory region used by the virtual machine, which works like a kind
 * of virtual memory paging.
 *
 * Memory regions should be allocated contiguously.
 */
struct __attribute__((aligned (16))) noravm_region
{
    struct noravm_list          list;       // List node
    uint64_t                    vm_addr;    // VM virtual address of region (virtual to the virtual machine)
    void*                       addr;       // Virtual address of the region (physical to the virtual machine)
    size_t                      size;       // Size of the region
    uint32_t                    flags;      // Region flags
};


/*
 * Exceptions that can occur in the virtual machine.
 */
enum 
{
    NORAVM_INTR_DEBUG_STATE         = 0x0,  // Write all registers of current state
    NORAVM_INTR_WRITE               = 0x1,  // Write syscall
    NORAVM_INTR_READ                = 0x2,  // Read syscall
    NORAVM_INTR_REGION              = 0x3,  // Retrieve information about the current memory region
    NORAVM_INTR_ARITHMETIC_ERROR    = 0x7,  // Arithmetical error, e.g. division by zero
    NORAVM_INTR_REGION_NOT_PRESENT  = 0x8,  // Memory region must be loaded
    NORAVM_INTR_REGION_FAULT        = 0x9,  // Virtual machine has accessed memory beyond bounds (page fault)
    NORAVM_INTR_INVALID_OPCODE      = 0xa,  // Malformed instruction
    NORAVM_INTR_PROTECTION_FAULT    = 0xb,  // Illegal memory operation
    NORAVM_INTR_OUT_OF_BOUNDS       = 0xc,  // Trying to access memory outside virtual memory segment
    NORAVM_INTR_STACK_OVERFLOW      = 0xd,  // State stack overflow
    NORAVM_INTR_USER_EXCEPTION      = 0xe,  // Signal was sent outside virtual machine
    NORAVM_INTR_ILLEGAL_STATE       = 0xf,  // Virtual machine is in an illegal state
};


/* 
 * Indicates if an exception is maskable.
 * Non-maskable interrupts will cause the virtual machine to abort.
 */
#define NORAVM_INTR_NONMASKABLE \
    (NORAVM_INTR_PROTECTION_FAULT | NORAVM_INTR_OUT_OF_BOUNDS | NORAVM_INTR_STACK_OVERFLOW | NORAVM_INTR_USER_EXCEPTION | NORAVM_INTR_ILLEGAL_STATE)

#define NORAVM_INTR_IS_MASKABLE(i) !(NORAVM_INTR_NONMASKABLE & (1 << (i)))


/*
 * Check if an interrupt is raised.
 */
#define NORAVM_INTR_IS_SET(registers, i) !!((registers)->intr & (1 << (i)))


/*
 * Default interrupt mask.
 * Unmasked interrupts will invoke the interrupt routine.
 * Masked interrupts will pop the state stack.
 */
#define NORAVM_INTR_DEFAULT_MASK    NORAVM_INTR_NONMASKABLE


/*
 * Clear an interrupt from the interrupt bitfield.
 */
#define NORAVM_INTR_CLEAR(registers, i)   \
    do { \
        (registers)->intr &= (~(1 << (i)) | NORAVM_INTR_NONMASKABLE); \
    } while (0)


/*
 * Interrupt routine signature.
 *
 * Handle the raised interrupts and clear them. If an interrupt is not cleared 
 * after this function returns, the virtual machine will attempt to pop the 
 * state stack.
 *
 * Non-maskable interrupts can not be cleared; clearing them will be ignored.
 *
 * Note that additional memory regions can be allocated (one by one), 
 * if necessary.
 */
typedef void (*noravm_interrupt_t)(struct noravm_registers*, struct noravm_region**);



/*
 * Registers used by the virtual machine.
 */
struct __attribute__((aligned(16))) noravm_registers
{
    uint32_t ip;        // Current instruction pointer
    uint16_t off;       // Current memory offset pointer
    uint16_t mask;      // Interrupt mask
    uint16_t intr;      // Interrupt bitfield
    uint32_t r[256];    // General purpose registers
};


/*
 * Current instruction read by the virtual machine.
 */
struct __attribute__((aligned (16))) noravm_instruction
{
    uint8_t     opcode;
    uint8_t     num_operands;
    uint8_t     operands[4];
    uint32_t    word;
};


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
    NORAVM_STATE_EXCEPTION      = 0x05,
};


/*
 * Virtual machine state.
 */
struct __attribute__((aligned (16))) noravm_state
{
    struct noravm_registers     regs;       // Registers
    struct noravm_instruction   instr;      // Current instruction
    uint8_t                     curr_state; // Current state
    uint8_t                     prev_state; // Previous state
};


/*
 * Virtual machine data.
 */
struct __attribute__((aligned (16))) noravm_data
{
    noravm_interrupt_t      intr;       // Address to the interrupt routine
    void*                   mem_addr;   // Base memory address
    size_t                  mem_size;   // Total size of memory segment
    uint8_t                 stack_idx;  // Current state stack pointer
    uint8_t                 stack_size; // Maximum depth of state stack
    struct noravm_list      regions;    // Memory regions
    struct noravm_state     states[];   // State stack
};


/*
 * Entry point description.
 */
struct __attribute__((aligned (16))) noravm_entry_point
{
    char                    id[16];         // Identifier string
    size_t                  stack_size;     // Virtual machine state stack size
    uint64_t                data_addr;      // Pointer to virtual machine data
    uint64_t                mem_addr;       // Base memory address
    size_t                  mem_size;       // Size of memory segment
    uint64_t                machine_addr;   // Address to virtual machine
    uint64_t                intr_addr;      // Address to interrupt routine
};



/*
 *  Nora virtual machine functions.
 */
struct noravm_function
{
    char        name[32];   // Symbol name of the function
    uint64_t    addr;       // Address of the function
    size_t      size;       // Size of the function in memory
};

struct noravm_functions
{
    char                    id[16];         // Identifier string
    struct noravm_function  intr;           // Interrupt routine
    struct noravm_function  vm;             // Virtual machine code
    struct noravm_function  loader;         // Address to the loader
};


#ifdef __cplusplus
extern "C"
#endif
void noravm_get_functions(struct noravm_functions* info);


#endif /* __NORAVM_VIRTUAL_MACHINE_H__ */
