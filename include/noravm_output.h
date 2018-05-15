#ifndef __NORAVM_OUTPUT_H__
#define __NORAVM_OUTPUT_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <noravm_image.h>


#ifdef __APPLE__
int noravm_macho_write(FILE* fp, struct noravm_image* image, const void* bytecode);
#else
int noravm_elf_write(FILE* fp, struct noravm_image* image, const void* bytecode);
#endif




#ifdef __cplusplus
}
#endif
#endif /* __NORAVM_OUTPUT_H__ */
