#ifndef __IBSEN_VIRTUAL_MACHINE_H__
#define __IBSEN_VIRTUAL_MACHINE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <noravm/entry.h>
#include <ibsen/section.h>
#include <ibsen/segment.h>


int ibsen_vm_create_sections(struct ibsen_segment* segment, struct noravm_info* entry);


#ifdef __cplusplus
}
#endif
#endif /* __IBSEN_VIRTUAL_MACHINE_H__ */
