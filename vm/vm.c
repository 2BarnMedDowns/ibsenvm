#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <noravm_vm.h>
#include <noravm_list.h>


void __noravm_interrupt(struct noravm_registers* regs, struct noravm_region** region)
{
    char data[16] = "Hello\n";
    write(1, data, 6);
}


void __noravm(struct noravm_data* vm)
{
    // I hope this is inlined...
//    size_t len = *((unsigned char*) vm->code_addr);
//    const char* str = ((const char*) vm->code_addr) + 1;
//    write(1, str, len);

    //vm->intr(NULL, NULL);

    while (1);
}


void __noravm_loader(void)
{
    // Get address descriptions from known location
    struct noravm_entry_point* ep = (void*) NORAVM_MEM_ADDR;

    // Get pointer to zeroed out machine state
    void (*vm)(struct noravm_data*) = (void*) ep->machine_addr;

    // Return value in R00
    __asm__ volatile (
            "syscall"
            : /* no output */
            : "a" (60), "D" (15)
            );
}


void noravm_get_functions(struct noravm_functions* ep)
{
    strcpy(ep->id, NORAVM_ID_STRING);

    strcpy(ep->intr.name, "__noravm_interrupt");
    ep->intr.addr = (uint64_t) __noravm_interrupt;
    ep->intr.size = (uint64_t) __noravm - (uint64_t) __noravm_interrupt;

    strcpy(ep->vm.name, "__noravm");
    ep->vm.addr = (uint64_t) __noravm;
    ep->vm.size = (uint64_t) __noravm_loader - (uint64_t) __noravm;

    strcpy(ep->loader.name, "__noravm_loader");
    ep->loader.addr = (uint64_t) __noravm_loader;
    ep->loader.size = (uint64_t) noravm_get_functions - (uint64_t) __noravm_loader;
}
