#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <ibsen/section.h>
#include <noravm/entry.h>


//static void create_loader();


int ibsen_vm_create_sections(struct ibsen_segment* segment, struct noravm_info* entry)
{
    size_t total_size = entry->interrupt.size + entry->start.size + entry->abort.size;

    return 0;
}


