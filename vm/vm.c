#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ivm_vm.h>
#include <ivm_interrupt.h>
#include <ivm_list.h>
#include <ivm_syscall.h>
#include <ivm_entry.h>
#include "syscall.h"


static inline __attribute__((always_inline))
size_t print(int fd, const char* str)
{
    size_t n;
    for (n = 0; *(str + n) != '\0'; ++n);
    return ibsen_write(fd, str, n);
}



static inline __attribute__((always_inline))
size_t print_uint(int fd, size_t pad, uint64_t value)
{
    size_t i, n;
    char placeholder[32];

    for (i = 0; i < sizeof(placeholder); ++i) {
        placeholder[i] = '0';
    }

    do {
        uint8_t v = value % 16;
        value /= 16;
        --i;
        ++n;

        if (v < 10) {
            placeholder[i] = '0' + v;
        } 
        else {
            placeholder[i] = 'A' + (v - 10);
        }
    } while (value > 0);

    if (n < pad) {
        n = pad;
    }

    return ibsen_write(fd, &placeholder[sizeof(placeholder) - n], n);
}




void __interrupt(struct ivm_data* vm, int fd, uint64_t addr)
{
    //char data[16] = "Hello\n";
    //write(1, data, 6);
}


int64_t __vm(struct ivm_data* vm)
{
    return 2;
}


void __loader(void)
{
   ibsen_exit(15);
}


void ivm_get_vm_functions(struct ivm_vm_functions* funcs)
{
    strcpy(funcs->id, IVM_ID_STRING);

    strcpy(funcs->interrupt.name, "__interrupt");
    funcs->interrupt.addr = (uint64_t) __interrupt;
    funcs->interrupt.size = (uint64_t) __vm - (uint64_t) __interrupt;

    strcpy(funcs->vm.name, "__vm");
    funcs->vm.addr = (uint64_t) __vm;
    funcs->vm.size = (uint64_t) __loader - (uint64_t) __vm;

    strcpy(funcs->loader.name, "__loader");
    funcs->loader.addr = (uint64_t) __loader;
    funcs->loader.size = (uint64_t) ivm_get_vm_functions - (uint64_t) __loader;
}
