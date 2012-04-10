#include <Python.h>
#include <stdbool.h>
#include <string.h>
#include "language_interpreter.h"


typedef struct _py_internal {
    PyObject*module;
    PyObject*globals;
    char*buffer;
} py_internal_t;

static int py_reference_count = 0;

static bool init_py(py_internal_t*py)
{
    if(py_reference_count==0) {
        Py_Initialize();
    }
    py_reference_count++;

    py->module = PyImport_AddModule("__main__");
    if(py->module == NULL)
        return false;
    py->globals = PyModule_GetDict(py->module);
    PyDict_SetItem(py->globals, PyString_FromString("math"), PyImport_ImportModule("math"));
    py->buffer = malloc(65536);
}

static bool define_function_py(language_interpreter_t*li, const char*script)
{
    py_internal_t*py = (py_internal_t*)li->internal;
    PyObject* ret = PyRun_String(script, Py_file_input, py->globals, py->globals);
    if(ret == NULL) {
        PyErr_Print();
        PyErr_Clear();
    }
    return ret!=NULL;
}

static int call_function_py(language_interpreter_t*li, row_t*row)
{
    py_internal_t*py = (py_internal_t*)li->internal;

    char*script = row_to_function_call(row, py->buffer, true);
    PyObject* ret = PyRun_String(script, Py_eval_input, py->globals, py->globals);
    if(ret == NULL) {
        PyErr_Print();
        PyErr_Clear();
        return -1;
    } else {
        return PyInt_AsLong(ret);
    }
}

static void destroy_py(language_interpreter_t* li)
{
    py_internal_t*py = (py_internal_t*)li->internal;
    free(py->buffer);
    free(py);
    free(li);
    if(--py_reference_count==0) {
        Py_Finalize();
    }
}

language_interpreter_t* python_interpreter_new()
{
    language_interpreter_t * li = calloc(1, sizeof(language_interpreter_t));
    li->name = "py";
    li->define_function = define_function_py;
    li->call_function = call_function_py;
    li->destroy = destroy_py;
    li->internal = calloc(1, sizeof(py_internal_t));
    py_internal_t*py = (py_internal_t*)li->internal;
    init_py(py);
    return li;
}

