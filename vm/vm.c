#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <noravm_vm.h>
#include <noravm_list.h>
#include "syscall.h"


static inline __attribute__((always_inline))
size_t print_str(int fd, const char* str)
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



static inline __attribute__((always_inline))
void copy_state(struct noravm_state* new, const struct noravm_state* old)
{

}



void __noravm_interrupt(struct noravm_registers* regs, struct noravm_region* mem)
{
    //char data[16] = "Hello\n";
    //write(1, data, 6);
    print_str(1, "hello\n");
}


void __noravm(struct noravm_data* vm)
{
//    struct noravm_state* init_state;
//    init_state.regs.ip = 0;
//    init_state.regs.off = 0;
//    init_state.regs.mask = NORAVM_INTR_DEFAULT_MASK;
//    init_state.regs.intr = 0;
//
//    init_state.regs[0x00] = 0x0;
//    for (int i = 0x01; i <= 0xff; ++i) {
//        init_state.regs.r[i] = 0xffffffff;
//    }
//
//    init_state.curr_state = NORAVM_STATE_OPCODE;
//    init_state.prev_state = NORAVM_STATE_ABORT;
//   
//    copy_state(&data->states[0], &init_state);
    // I hope this is inlined...
//    size_t len = *((unsigned char*) vm->code_addr);
//    const char* str = ((const char*) vm->code_addr) + 1;
//    write(1, str, len);

    vm->intr(NULL, NULL);
}


void __noravm_loader(void)
{
    // Get address descriptions from known location
    struct noravm_entry_point* ep = (void*) NORAVM_MEM_ADDR;

    // Bootstrap initial state 
    struct noravm_data* data = (void*) ep->data_addr;
    data->intr = (noravm_interrupt_t) ep->intr_addr;
    data->addr = (void*) ep->mem_addr;
    data->size = ep->mem_size;
    data->stack_idx = 0;
    data->stack_size = ep->stack_size;

    // Create memory regions
    size_t state_end = sizeof(struct noravm_data) + ep->stack_size * sizeof(struct noravm_state);
    noravm_list_init(&data->regions);

    struct noravm_region* regions = (void*) (((unsigned char*) ep->data_addr) + state_end);
    for (size_t i = 0; i < (ep->data_size - state_end) / sizeof(struct noravm_region); ++i) {
        noravm_list_insert(&data->regions, &regions[i]);
        regions[i].vm_addr = i * ep->region_size;
        regions[i].ph_addr = i * ep->region_size + ep->mem_addr;
        regions[i].size = ep->region_size;
    }

    // Make function pointer to virtual machine
    void (*vm)(struct noravm_data*) = (void*) ep->machine_addr;
    vm(data);
    
    // Return value in R00
    ibsen_exit(0);
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
