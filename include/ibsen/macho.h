#ifndef __IBSEN_MACHO_H__
#define __IBSEN_MACHO_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <ibsen/image.h>


int ibsen_write_macho(FILE* fp, struct ibsen_image* image, const void* bytecode);


#ifdef __cplusplus
}
#endif
#endif /* __IBSEN_MACHO_H__ */
