#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
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
    size_t len = *((unsigned char*) vm->code_addr);
    const char* str = ((const char*) vm->code_addr) + 1;
    write(1, str, len);

    vm->intr(NULL, NULL);
}


void __noravm_loader()
{
//    struct noravm_entry_point* ep = (void*) NORAVM_ENTRY_ADDR;
//
//    struct noravm_data* data = (void*) ep->data_addr;
//
//    data->intr = (noravm_interrupt_t) ep->intr_addr;
//    data->code_addr = (void*) ep->code_addr;
//
//    void (*vm)(struct noravm_data*) = (void*) ep->machine_addr;
//
//    vm(data);
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
