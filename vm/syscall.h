#ifndef __IBSEN_VM_SYSCALL_H__
#define __IBSEN_VM_SYSCALL_H__

#include <stdint.h>
#include <stddef.h>



static inline __attribute__((always_inline))
long ibsen_syscall1(long nr, long long arg1)
{
    long ret;
    __asm__ volatile ("syscall" : "=a" (ret) : "a" (nr), "D" (arg1) : "memory");
    return ret;
}


static inline __attribute__((always_inline))
long ibsen_syscall3(long nr, long long arg1, long long arg2, long long arg3)
{
    long ret;
    __asm__ volatile ("syscall" : "=a" (ret) : "a" (nr), "D" (arg1), "S" (arg2), "d" (arg3) : "memory");
    return ret;
}



static inline __attribute__((always_inline))
void ibsen_exit(int status)
{
    ibsen_syscall1(60, status);
}



static inline __attribute__((always_inline))
size_t ibsen_write(int fd, const char* ptr, size_t len)
{
    return ibsen_syscall3(1, fd, (long long) ptr, (long long) len);
}


#endif /* __IBSEN_VM_SYSCALL_H__ */
