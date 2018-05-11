#ifndef __IBSEN_IMAGE_H__
#define __IBSEN_IMAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <ibsen/list.h>

struct ibsen_image
{
    struct ibsen_list   segments;
};


int ibsen_image_create(struct ibsen_image** image);


void ibsen_image_destroy(struct ibsen_image* image);

#ifdef __cplusplus
}
#endif
#endif /* __IBSEN_IMAGE_H__ */
