#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <noravm/state.h>
#include <noravm/bytecode.h>
#include <noravm/interrupt.h>
#include <noravm/registers.h>
#include <noravm/segment.h>
#include <noravm/entry.h>


/*
 * Mark an interrupt as raised in the interrupt mask.
 */
#define set_interrupt(vm, i) \
    do { \
        (vm)->mask |= 1 << (i); \
    } while (false)


/*
 * Clear/mask interrupt from interrupt mask.
 */
#define clear_interrupt(vm, i) \
    do { \
        if (NORAVM_INTR_IS_MASKABLE(i)) { \
            (vm)->mask &= ~(1 << (i)); \
        } \
    } while (false)



/*
 * Read bytes from specified segment.
 * Returns number of bytes read if successful, or raises an exception if not.
 */
__attribute__((always_inline)) static inline 
size_t read_bytes(void* dst, struct noravm* vm, uint16_t segment_idx, size_t offset, size_t length) 
{
    if (segment_idx > NORAVM_MAX_SEGS) {
        vm->interrupt(vm, NORAVM_INTR_SEGMENT_NOT_PRESENT);
        return 0;
    }

    const struct noravm_seg* seg = &vm->segments[segment_idx];

    if (!(seg->flags & NORAVM_SEG_READ)) {
        vm->interrupt(vm, NORAVM_INTR_PROTECTION_FAULT);
        return 0;
    }

    if (offset < seg->offset) {
        vm->interrupt(vm, NORAVM_INTR_OUT_OF_BOUNDS);
        return 0;
    }

    if (offset + length >= seg->offset + seg->size) {
        vm->interrupt(vm, NORAVM_INTR_OUT_OF_BOUNDS);
        return 0;
    }

    if (offset + length >= seg->vmsize) {
        vm->interrupt(vm, NORAVM_INTR_PROTECTION_FAULT);
        return 0;
    }

    const uint8_t* src = ((const uint8_t*) seg->vmaddr) + offset;
    const uint8_t* end = src + length;
    uint8_t* ptr = (uint8_t*) dst;

    while (src < end) {
        *ptr++ = *src++;
    }

    return ptr - ((uint8_t*) dst);
}


/*
 * Read opcode and operands.
 */
__attribute__((always_inline)) static inline 
void read_instruction(struct noravm* vm)
{
    struct noravm_instr* instr = &vm->instruction;
    struct noravm_regs* regs = &vm->registers;
    uint8_t remaining = 0;

    switch (vm->state) {

        case NORAVM_STATE_OPCODE:
            if (read_bytes(&instr->opcode, vm, regs->is, regs->ip, 1) == 1) {
                instr->num_operands = 0;
                regs->ip += 1;
                vm->state = NORAVM_STATE_OPERANDS;
            }
            return;

        case NORAVM_STATE_OPERANDS:
            switch ((instr->opcode & 0xe0) >> 5) {
                case 0:
                case 4:
                    remaining = 0;
                    break;

                case 1:
                case 5:
                    remaining = 1 - instr->num_operands;
                    break;

                case 2:
                case 6:
                    remaining = 2 - instr->num_operands;
                    break;

                case 3:
                    remaining = 3 - instr->num_operands;
                    break;

                case 7:
                    if (instr->opcode == SCALL) {
                        remaining = 4 - instr->num_operands;
                    } 
                    else {
                        remaining = 3 - instr->num_operands;
                    }
                    break;
            }

            if (remaining == 0) {
                vm->state = NORAVM_STATE_WORD;
            }
            else if (read_bytes(&instr->operands[instr->num_operands], vm, regs->is, regs->ip, 1) == 1) {
                instr->num_operands += 1;
                regs->ip += 1;
            }
            return;

        case NORAVM_STATE_WORD:
            if (!(0x80 & instr->opcode)) {
                instr->word = 0xffffffff;
                vm->state = NORAVM_STATE_EXECUTE;
            }
            else if (read_bytes(&instr->word, vm, regs->is, regs->ip, sizeof(uint32_t)) == sizeof(uint32_t)) {
                vm->state = NORAVM_STATE_EXECUTE;
            }
            return;

        default:
            vm->interrupt(vm, NORAVM_INTR_ILLEGAL_STATE);
            return;
    }
}


/*
 * Raise interrupt and invoke exception handler.
 */
void noravm_interrupt(struct noravm* vm, uint16_t intr_no)
{
    if (vm->stackptr < NORAVM_MAX_STATES - 1) {
        // Save state on stack
        vm->stack[vm->stackptr++] = vm->state;
        vm->state = NORAVM_STATE_EXCEPTION;

        set_interrupt(vm, intr_no);
        bool masked = !!vm->handler(vm, &vm->registers, vm->segments, intr_no);
        if (masked) {
            clear_interrupt(vm, intr_no);
        }
    } 
    else {
        // Stack exceeded, probably an interrupt loop
        set_interrupt(vm, NORAVM_INTR_ILLEGAL_STATE);
        set_interrupt(vm, intr_no);
        vm->state = NORAVM_STATE_ABORT;
    }
}


/*
 * Reset virtual machine state.
 */
__attribute__((always_inline)) static inline 
void reset_state(struct noravm* vm)
{
    struct noravm_regs* regs = &vm->registers;

    // Clear interrupts and reset state
    vm->mask = 0;
    vm->stack[0] = NORAVM_STATE_ABORT;
    vm->stackptr = 1;
    vm->state = NORAVM_STATE_OPCODE;

    // Reset registers
    regs->is = 0;
    regs->ip = 0;
    regs->sp = 0;
    for (size_t i = 0; i < 256; ++i) {
        regs->r[i] = 0xffffffff;
    }

    if (vm->interrupt == NULL) {
        vm->interrupt = noravm_interrupt;
    }

    if (vm->handler == NULL) {
        set_interrupt(vm, NORAVM_INTR_ILLEGAL_STATE);
        vm->state = NORAVM_STATE_ABORT;
    }

}


void noravm_start(struct noravm* vm)
{
    reset_state(vm);

    while (true) {
        switch (vm->state) {

            case NORAVM_STATE_ABORT:
                return;

            case NORAVM_STATE_OPCODE:
            case NORAVM_STATE_OPERANDS:
            case NORAVM_STATE_WORD:
                read_instruction(vm);
                break;

            case NORAVM_STATE_EXECUTE:
                break;

            case NORAVM_STATE_EXCEPTION:
                if (vm->mask != 0) {
                    // Interrupt was not masked
                    vm->state = NORAVM_STATE_ABORT;
                } 
                else {
                    // Pop state of stack and resume
                    vm->state = vm->stack[--vm->stackptr];
                }
                break;

            default:
                vm->interrupt(vm, NORAVM_INTR_ILLEGAL_STATE);
                break;
        }
    }
}


int noravm_load()
{
#ifdef NORAVM_KNOWN_LOCATION
    char status[16] = "Aborted\n";
    struct noravm* state = (struct noravm*) NORAVM_KNOWN_LOCATION;
    state->start(state);
    
    switch (state->state) {
        case NORAVM_STATE_ABORT:
            write(1, status, 8);
            return 1;

        default:
            return state->registers.r[0];
    }
#else 
    char status[16] = "Illegal state\n";
    write(1, status, 14);
    return 4;
#endif
}


bool noravm_fault(const struct noravm* vm, struct noravm_regs* regs, struct noravm_seg* segs, uint16_t intr_no)
{
    char string[32] = "hello\n";

    write(1, string, 6);

    if (intr_no < 0x9) {
        return true;
    }

    return false;
}


/*
 * Abort virtual machine.
 */
void noravm_abort(struct noravm* vm)
{
    // Save the state for debug purposes
    if (vm->stackptr < NORAVM_MAX_STATES - 1) {
        vm->stack[vm->stackptr++] = vm->state;
    }

    set_interrupt(vm, NORAVM_INTR_USER_EXCEPTION);
    vm->state = NORAVM_STATE_ABORT;
}


/*
 * Helper function for populating entry point list.
 */
__attribute__((always_inline)) static inline
void ep_offset(struct noravm_ep* ep, const char* name, void* func, void* offset_to)
{
    size_t i;
    for (i = 0; name[i] != '\0'; ++i) {
        ep->name[i] = name[i];
    }
    ep->name[i] = '\0';
    ep->addr = func;
    ep->size = ((uint64_t) offset_to) - ((uint64_t) func);
}


/*
 * Return descriptor of virtual machine entry points.
 */
void noravm_entry(struct noravm_info* info)
{
    size_t i;
    const char* version_string = NORAVM_VERSION;
    for (i = 0; version_string[i] != '\0'; ++i) {
        info->id[i] = version_string[i];
    }
    info->id[i] = '\0';

    ep_offset(&info->interrupt, "noravm_interrupt", noravm_interrupt, noravm_start);
    ep_offset(&info->start, "noravm_start", noravm_start, noravm_load);
    ep_offset(&info->loader, "noravm_load", noravm_load, noravm_fault);
    ep_offset(&info->fault, "noravm_fault", noravm_fault, noravm_abort);
    ep_offset(&info->abort, "noravm_abort", noravm_abort, noravm_entry);
}

