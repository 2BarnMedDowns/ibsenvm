#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <string.h>
#include <ivm_vm.h>
#include <ivm_image.h>


//static int get_functions(const char* filename, struct ivm_vm_functions* funcs)
//{
//    void* handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
//    if (handle == NULL) {
//        fprintf(stderr, "Failed to dynamic lib: %s\n", dlerror());
//        return errno;
//    }
//
//    void (*get)(struct ivm_vm_functions*) = dlsym(handle, "ivm_get_vm_functions");
//    if (get == NULL) {
//        fprintf(stderr, "Failed to dynamic lib: %s\n", dlerror());
//        return errno;
//    }
//
//    get(funcs);
//
//    dlclose(handle);
//    return 0;
//}




int main(int argc, char** argv)
{
    int result;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <vm> <output>\n", argv[0]);
        return 1;
    }

//    struct ivm_vm_functions funcs;
//    get_functions(argv[1], &funcs);

    const char* string = "\x0eHello, world!\n";

    struct ivm_image* image;
    result = ivm_image_create(&image, 32, 0x400, 256);
    if (result != 0) {
        fprintf(stderr, "Failed to create image: %s\n", strerror(result));
        return result;
    }

    //result = ivm_image_load_vm(image, &funcs, 0x400000);
    result = ivm_image_load_vm_from_file(image, argv[1], 0x400000);
    if (result != 0) {
        fprintf(stderr, "Failed to load VM code: %s\n", strerror(result));
        return result;
    }

    result = ivm_image_reserve_vm_data(image, IVM_ENTRY, 16);
    if (result != 0) {
        fprintf(stderr, "Failed to reserve VM memory for data: %s\n", strerror(result));
        return result;
    }



    FILE* fp = fopen(argv[2], "w");
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
