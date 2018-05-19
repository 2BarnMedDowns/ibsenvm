#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ivm_vm.h>
#include <ivm_image.h>


int print_usage(char** argv) {
    fprintf(stderr, "Usage: %s output\n", argv[0]);
    return 1;
}

int main(int argc, char** argv)
{
    int result;

    if (argc != 2) {
        return print_usage(argv);
    } else {
        if ( strcmp(argv[1], "-h") == 0) {
            return print_usage(argv);
        }
        if ( strcmp(argv[1], "-u") == 0) {
            return print_usage(argv);
        }
    }

    struct ivm_vm_functions funcs;
    ivm_get_vm_functions(&funcs);
    const char* string = "\x0eHello, world!\n";

    struct ivm_image* image;
    result = ivm_image_create(&image, 32, 0x400, 256);
    if (result != 0) {
        fprintf(stderr, "Failed to create image: %s\n", strerror(result));
        return result;
    }

    result = ivm_image_load_vm(image, &funcs, 0x400000);
    if (result != 0) {
        fprintf(stderr, "Failed to load VM code: %s\n", strerror(result));
        return result;
    }

    result = ivm_image_reserve_vm_data(image, IVM_ENTRY, 16);
    if (result != 0) {
        fprintf(stderr, "Failed to reserve VM memory for data: %s\n", strerror(result));
        return result;
    }


    FILE* fp = fopen(argv[1], "w");
    if (fp == NULL) {
        fprintf(stderr, "%s\n", strerror(errno));
        return errno;
    }

    result = ivm_image_write(fp, image, string);
    if (result != 0) {
        fprintf(stderr, "Failed to write to output file: %s\n", strerror(result));
        return result;
    }

    fclose(fp);

    ivm_image_remove(image);

    return 0;
}
