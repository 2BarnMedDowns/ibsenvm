#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ibsen/image.h>
#include <noravm/entry.h>


int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    struct noravm_info info;

    noravm_entry(&info);

    struct ibsen_image* image;
    int result = ibsen_create_macos_image(&image, &info, 1);

    FILE* fp = fopen(argv[1], "wb");
    if (fp == NULL) {
        fprintf(stderr, "ack: %s\n", strerror(errno));
        ibsen_destroy_image(image);
        return 1;
    }

    result =ibsen_write_image(fp, image, "\x00");
    if (result != 0) {
        fprintf(stderr, "%s\n", strerror(result));
    }

    ibsen_destroy_image(image);

    return 0;
}
