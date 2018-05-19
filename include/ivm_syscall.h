#ifndef __IBSENVM_SYSCALL_H__
#define __IBSENVM_SYSCALL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>



/*
 * System calls supported by the Ibsen virtual machine.
 */
enum
{
    IVM_SYSCALL_WRITE               = 0x0000,   // Write to specified file descriptor
    IVM_SYSCALL_READ                = 0x0001,   // Read from specified file descriptor
    // TODO: Open and close fd
    // TODO: mmap and munmap for allocating memory
};




#ifdef __cplusplus
}
#endif
#endif /* __IBSENVM_SYSCALL_H__ */
