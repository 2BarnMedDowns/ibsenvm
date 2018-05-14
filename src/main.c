#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <noravm_vm.h>
#include <noravm_image.h>
#include <noravm_output.h>



int main(int argc, char** argv)
{
    int result;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s output\n", argv[0]);
        return 1;
    }

    struct noravm_functions funcs;
    noravm_get_functions(&funcs);
    const char* string = "\x0eHello, world!\n";

    struct noravm_image* image;
    result = noravm_image_create(&image, NORAVM_ENTRY_ADDR);
    if (result != 0) {
        fprintf(stderr, "Failed to create image: %s\n", strerror(result));
        return result;
    }

//    result = noravm_image_add_segment(NULL, image, NORAVM_SEG_NULL, 1ULL << 30, NORAVM_ENTRY_ADDR);
//    if (result != 0) {
//        fprintf(stderr, "Add segment: %s\n", strerror(result));
//        return result;
//    }

    result = noravm_image_reserve_vm_data(image, 0x100000, 32, 8);
    if (result != 0) {
        fprintf(stderr, "Failed to reserve VM memory for data: %s\n", strerror(result));
        return result;
    }

    result = noravm_image_load_vm(image, &funcs, 8);
    if (result != 0) {
        fprintf(stderr, "Failed to load VM code: %s\n", strerror(result));
        return result;
    }

    FILE* fp = fopen(argv[1], "w");
    if (fp == NULL) {
        fprintf(stderr, "%s\n", strerror(errno));
        return errno;
    }

    //noravm_macho_write(fp, image, string);
    result = noravm_elf_write(fp, image, string);
    if (result != 0) {
        fprintf(stderr, "Failed to write to output file: %s\n", strerror(result));
        return result;
    }

    fclose(fp);

    noravm_image_remove(image);

    return 0;
}
