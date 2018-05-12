#ifndef __NORAVM_ENTRY_H__
#define __NORAVM_ENTRY_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Forward declaration */
struct noravm;


/*
 * Describe a VM entry point.
 */
struct __attribute__((aligned (16))) noravm_ep
{
    char                name[32];   // Symbol name of the function
    void*               addr;       // Address of the entry point
    size_t              size;       // Size of the function in memory
};


/*
 * Entry point description.
 */
struct __attribute__((aligned (16))) noravm_info
{
    char                id[16];     // Identifier string
    struct noravm_ep    interrupt;  // Interrupt routine
    struct noravm_ep    start;      // Start virtual machine
    struct noravm_ep    loader;     // Launch virtual machine
    struct noravm_ep    fault;      // Fault handler
    struct noravm_ep    abort;      // Abort the virtual machine
};


/*
 * Retrieve entry point descriptions.
 */
void noravm_entry(struct noravm_info* info);


typedef void (*noravm_start_t)(struct noravm*);


#ifdef __cplusplus
}
#endif
#endif /* __NORAVM_ENTRY_H__ */
