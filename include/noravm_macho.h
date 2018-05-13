#ifndef __NORAVM_MACHO_H__
#define __NORAVM_MACHO_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <noravm_image.h>


int noravm_macho_write(FILE* fp, struct noravm_image* image, const void* bytecode);


#ifdef __cplusplus
}
#endif
#endif /* __NORAVM_MACHO_H__ */
