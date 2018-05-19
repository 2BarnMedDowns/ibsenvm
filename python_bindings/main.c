#include <stdio.h>
#include <Python.h>
#include "structmember.h"

//#include <ivm_image.h>  // XXX Add this to call vm shit

typedef struct {
    PyObject_HEAD
        PyObject* dict;
    int count;
} ivm;


static PyObject *ibsen_image_create(ivm *self, PyObject *args) {
    // XXX Parse and fill out
}

static PyObject *ibsen_image_remove(ivm *self, PyObject *args) {
    // XXX Parse and fill out
}

static PyObject *ibsen_add_segment(ivm *self, PyObject *args) {
    // XXX Parse and fill out
}

static PyObject *ibsen_add_section(ivm *self, PyObject *args) {
    // XXX Parse and fill out
}

static PyObject *ibsen_reserve_vm_data(ivm *self, PyObject *args) {
    // XXX Parse and fill out
}

static PyObject *ibsen_write(ivm *self, PyObject *args) {
    // XXX Parse and fill out
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
    { "ivm_image_create", (PyCFunction) ibsen_image_create, METH_VARARGS, "Set Astar parameters" },
    { "ivm_image_remove", (PyCFunction) ibsen_image_remove, METH_VARARGS, "Set Astar parameters" },
    { "ivm_image_add_segment", (PyCFunction) ibsen_add_segment, METH_VARARGS, "Set Astar parameters" },
    { "ivm_image_add_section", (PyCFunction) ibsen_add_section, METH_VARARGS, "Set Astar parameters" },
    { "ivm_image_reserve_vm_data", (PyCFunction) ibsen_reserve_vm_data, METH_VARARGS, "Set Astar parameters" },
    { "ivm_image_write", (PyCFunction) ibsen_write, METH_VARARGS, "Set Astar parameters" },
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
