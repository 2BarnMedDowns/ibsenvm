#include <stdio.h>
#include <Python.h>
#include <stdint.h>
#include <dlfcn.h>
#include "structmember.h"

#include <ivm_entry.h>
#include <ivm_image.h>



typedef struct {
    PyObject_HEAD
        PyObject* dict;
    int count;
} ivm;



static int get_functions(const char* filename, struct ivm_vm_functions* functions)
{
    void* handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        return 0;
    }

    void (*function)(struct ivm_vm_functions*) = dlsym(handle, "ivm_get_vm_functions");
    if (!function) {
        dlclose(handle);
        return 0;
    }

    function(functions);

    dlclose(handle);
    return 1;
}



static int create_image_from_python(const char* vm_filename, 
                                    const char* filename, 
                                    const char* bytecode, 
                                    size_t bcode_size, 
                                    uint64_t startaddr, 
                                    size_t statesize, 
                                    size_t framesize, 
                                    size_t numframes, 
                                    uint64_t dataaddr)
{

    struct ivm_vm_functions functions;
    if (!get_functions(vm_filename, &functions)) {
        return 0;
    }

    struct ivm_image* image;
    int ret_image_create = ivm_image_create(&image, statesize, framesize, numframes);
    
    if (!ret_image_create) {
        return 0;
    }

    int vm_ret = ivm_image_load_vm(image, &functions, startaddr);

    if (!vm_ret) {
        ivm_image_remove(image);
        return 0;
    }
    
    int vm_data = ivm_image_reserve_vm_data(image, dataaddr, bcode_size);

    if (!vm_data) {
        ivm_image_remove(image);
        return 0;
    }
    
    FILE* imagefile = fopen(filename, "wb");
    if (!imagefile) {
        ivm_image_remove(image);
        return 0;
    }

    int write_status = ivm_image_write(imagefile, image, bytecode);
    
    fclose(imagefile);
    ivm_image_remove(image);

    return write_status == 0;
}



static PyObject* ibsen_image_create(ivm* self, PyObject *args) 
{
    const char* vm_filename;
    const char* bytecode;
    const char* filename;
    size_t framesize = 0x400;
    size_t statesize = 0x10;
    size_t numframes = 0x100;
    uint64_t startaddr = 0x400000;
    uint64_t dataaddr = 0x8000000;
    size_t len_bcode;

    if (!PyArg_ParseTuple(args, "sssn", &filename, &vm_filename, &bytecode, &len_bcode)) {
        return NULL;
    }

    int ret = create_image_from_python(vm_filename, filename, bytecode, len_bcode, startaddr, statesize, framesize, numframes, dataaddr);

    fprintf(stderr, "vi tror den ikke feiler: %d\n", ret);
    return Py_BuildValue("i", ret);
}



static int ibsen_init(ivm *self, PyObject *args, PyObject *kwds) {
    self->dict = PyDict_New();
    self->count = 0;
    return 0;
}



static void ibsen_dealloc(ivm *self) {
    Py_XDECREF(self->dict);
    self->ob_type->tp_free((PyObject*)self);
}



// Example members, these are just examples
static PyMemberDef ibsen_members[] =
{
    {"dict", T_OBJECT, offsetof(ivm, dict), 0, "Description"},
    {"count", T_INT, offsetof(ivm, count), 0, "Description"},
    { NULL }
};



static PyMethodDef ibsen_methods[] = 
{
    { "image_create", (PyCFunction) ibsen_image_create, METH_VARARGS, "CREATE THEM IMAGES APPLES" },
    { NULL }
};



/* Annoying BEHOMT object for defining a 'type' */
static PyTypeObject ibsenType = 
{
    PyObject_HEAD_INIT(NULL)
        0,                         /* ob_size */
    "Astar",                   /* tp_name */
    sizeof(ivm),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)ibsen_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags*/
    "Astar object",            /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    ibsen_methods,             /* tp_methods */
    ibsen_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)ibsen_init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};



PyMODINIT_FUNC initibsen(void)
{
    PyObject* mod;
    mod = Py_InitModule("ibsen", ibsen_methods);
    // Fill in some slots in the type, and make it ready
    ibsenType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&ibsenType) < 0) {
        return;
    }

    // Add the type to the module.
    Py_INCREF(&ibsenType);
    PyModule_AddObject(mod, "Astar", (PyObject*)&ibsenType);
}
