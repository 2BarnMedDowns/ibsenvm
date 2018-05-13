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
#include <noravm_macho.h>



int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s output\n", argv[0]);
        return 1;
    }

    struct noravm_functions funcs;
    noravm_get_functions(&funcs);
    const char* string = "\x0eHello, world!\n";

    struct noravm_image* image;
    noravm_image_create(&image, 0);

    noravm_image_add_segment(NULL, image, NORAVM_SEG_NULL, 0x100000, NORAVM_ENTRY_ADDR);

    struct noravm_segment* code;
    noravm_image_add_segment(&code, image, NORAVM_SEG_CODE, 0x1000, 0x3000);
    int r = noravm_image_vm_code(code, &funcs);
    fprintf(stderr, "%d\n", r);

    struct noravm_segment* data;
    r = noravm_image_add_segment(&data, image, NORAVM_SEG_DATA, 0x1000, 0x2000);
    fprintf(stderr, "%d\n", r);
    r = noravm_image_vm_data(data, 16, 0x1000);
    fprintf(stderr, "%d\n", r);


    FILE* fp = fopen(argv[1], "wb");

    noravm_macho_write(fp, image, string);

    noravm_image_remove(image);


    return 0;
}
