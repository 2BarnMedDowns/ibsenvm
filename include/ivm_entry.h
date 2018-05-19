#ifndef __IBSENVM_ENTRY_H__
#define __IBSENVM_ENTRY_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>


/*
 * Information about a VM function.
 */
struct ivm_function
{
    char        name[32];   // Symbol name of the function
    uint64_t    addr;       // Address of the function
    size_t      size;       // Size of the function in memory
};



/*
 *  Functions used by the Ibsen virtual machine.
 */
struct ivm_vm_functions
{
    char                id[16];     // Identifier string
    struct ivm_function interrupt;  // Interrupt routine
    struct ivm_function vm;         // Virtual machine code
    struct ivm_function loader;     // Address to the loader
};


/*
 * System calls.
 */
struct ivm_vm_calls
{
    size_t              num_calls;
    struct ivm_function calls[];
};


void ivm_get_vm_functions(struct ivm_vm_functions* info);


void ivm_get_vm_syscalls(struct ivm_vm_calls* calls);



#ifdef __cplusplus
}
#endif
#endif /* __IBSENVM_ENTRY_H__ */
