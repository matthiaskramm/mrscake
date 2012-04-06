/* mrscake.py.c

   Python wrapper for the prediction API.

   Copyright (c) 2011 Matthias Kramm <kramm@quiss.org>
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <Python.h>
#include <stdarg.h>
#include <setjmp.h>
#include "mrscake.h"
#include "list.h"
#include "stringpool.h"
#include "settings.h"

#if PY_MAJOR_VERSION >= 3
#define PYTHON3
#define M_FLAGS (METH_VARARGS|METH_KEYWORDS)
#else
#define M_FLAGS (METH_KEYWORDS)
#endif

typedef struct _state {
    void*dummy;
} state_t;

#ifdef PYTHON3
#define STATE(m) ((state_t*)PyModule_GetState(m))
#else
static state_t global_state = {0};
#define STATE(m) &global_state;
#endif

static PyTypeObject ModelClass;
static PyTypeObject DataSetClass;

DECLARE_LIST(example);

typedef struct {
    PyObject_HEAD
    trainingdata_t*data;
} DataSetObject;

typedef struct {
    PyObject_HEAD
    model_t*model;
} ModelObject;

static char* strf(char*format, ...)
{
    char buf[1024];
    int l;
    va_list arglist;
    va_start(arglist, format);
    vsnprintf(buf, sizeof(buf)-1, format, arglist);
    va_end(arglist);
    return strdup(buf);
}
static inline PyObject*pystring_fromstring(const char*s)
{
#ifdef PYTHON3
    return PyUnicode_FromString(s);
#else
    return PyString_FromString(s);
#endif
}
static inline int pystring_check(PyObject*o)
{
#ifdef PYTHON3
    return PyUnicode_Check(o);
#else
    return PyString_Check(o);
#endif
}
static inline PyObject*pyint_fromlong(long l)
{
#ifdef PYTHON3
    return PyLong_FromLong(l);
#else
    return PyInt_FromLong(l);
#endif
}
static inline const char*pystring_asstring(PyObject*s)
{
#ifdef PYTHON3
    return PyUnicode_AS_DATA(s);
#else
    return PyString_AsString(s);
#endif
}
PyObject*forward_getattr(PyObject*self, char *a)
{
    PyObject*o = pystring_fromstring(a);
    PyObject*ret = PyObject_GenericGetAttr(self, o);
    Py_DECREF(o);
    return ret;
}

#define PY_ERROR(s,args...) (PyErr_SetString(PyExc_Exception, strf(s, ## args)),(void*)NULL)
#define PY_NONE Py_BuildValue("s", 0)

//--------------------helper functions --------------------------------
int add_item(example_t*e, int pos, PyObject*item)
{
    if(PyInt_Check(item)) {
        e->inputs[pos] = variable_new_continuous(PyInt_AS_LONG(item));
    } else if(PyFloat_Check(item)) {
        e->inputs[pos] = variable_new_continuous(PyFloat_AS_DOUBLE(item));
    } else if(PyString_Check(item)) {
        e->inputs[pos] = variable_new_text(PyString_AsString(item));
    } else {
        PY_ERROR("bad object %s in list", item->ob_type->tp_name);
        return 0;
    }
    return 1;
}
example_t* pylist_to_example(PyObject*input)
{
    example_t*e = 0;
    if(PyList_Check(input)) {
        int size = PyList_Size(input);
        e = example_new(size);
        int t;
        for(t=0;t<size;t++) {
            PyObject*item = PyList_GetItem(input, t);
            if(!add_item(e, t, item))
                return NULL;
        }
    } else if(PyDict_Check(input)) {
        int size = PyDict_Size(input);
        PyObject*pkey = 0;
        PyObject*pvalue = 0;
        size_t pos = 0;
        int t = 0;
        e = example_new(size);
        e->input_names = (const char**)malloc(sizeof(e->input_names[0])*size);
        while(PyDict_Next(input, &pos, &pkey, &pvalue)) {
            if(!PyString_Check(pkey))
                return PY_ERROR("dict object must use strings as keys");
            const char*s = pystring_asstring(pkey);
            if(!add_item(e, t, pvalue))
                return NULL;
            e->input_names[t] = register_string(s);
            t++;
        }
    } else {
        return PY_ERROR("first argument must be a list or a dict");
    }
    return e;
}
//---------------------------------------------------------------------
static void model_dealloc(PyObject* _self) {
    ModelObject* self = (ModelObject*)_self;
    PyObject_Del(self);
}
static PyObject* model_getattr(PyObject * _self, char* a)
{
    ModelObject*self = (ModelObject*)_self;
    return forward_getattr(_self, a);
}
static int model_setattr(PyObject * self, char* a, PyObject * o) {
    return -1;
}
static int py_model_print(PyObject * _self, FILE *fi, int flags)
{
    ModelObject*self = (ModelObject*)_self;
    model_print(self->model);
    return 0;
}
PyDoc_STRVAR(model_save_doc, \
"save(filename)\n\n"
"Saves the trained model to a file\n"
);
static PyObject* py_model_save(PyObject* _self, PyObject* args, PyObject* kwargs)
{
    ModelObject* self = (ModelObject*)_self;
    char*filename = 0;
    static char *kwlist[] = {"filename", NULL};
    int ret;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &filename))
        return NULL;

    model_save(self->model, filename);
    return PY_NONE;
}
PyDoc_STRVAR(model_predict_doc, \
"predict(data)\n\n"
"Evaluate the model for a given input. I.e. tries to estimate the target value (do a prediction).\n"
);
static PyObject* py_model_predict(PyObject* _self, PyObject* args, PyObject* kwargs)
{
    ModelObject* self = (ModelObject*)_self;
    PyObject*data = 0;
    static char *kwlist[] = {"data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &data))
        return NULL;
    example_t*e = pylist_to_example(data);
    if(!e)
        return NULL;
    if(e->num_inputs != self->model->sig->num_inputs) {
        PY_ERROR("You supplied %d inputs for a model with %d inputs", e->num_inputs, self->model->sig->num_inputs);
        example_destroy(e);
        return NULL;
    }
    row_t*row = example_to_row(e, self->model->sig->column_names);
    if(!row)
        return PY_ERROR("Can't create row from data");
    variable_t i = model_predict(self->model, row);
    row_destroy(row);
    example_destroy(e);

    if(i.type == TEXT)
        return PyString_FromString(i.text);
    else if(i.type == CATEGORICAL)
        return pyint_fromlong(i.category);
    else if(i.type == CONTINUOUS)
        return PyFloat_FromDouble(i.value);
    else if(i.type == MISSING)
        return PY_NONE;
    else
        return PY_ERROR("internal error: bad variable type %d", i.type);
}
PyDoc_STRVAR(model_generate_code_doc, \
"generate_code(language)\n\n"
"Generate code for this model\n"
);
static PyObject* py_model_generate_code(PyObject* _self, PyObject* args, PyObject* kwargs)
{
    ModelObject* self = (ModelObject*)_self;
    char*language = 0;
    static char *kwlist[] = {"language", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s", kwlist, &language))
        return NULL;
    char*code = model_generate_code(self->model, language);
    return PyString_FromString(code);
}
PyDoc_STRVAR(model_load_doc, \
"load_model()\n\n"
"Load a model.\n"
);
static PyObject* py_model_load(PyObject* module, PyObject* args, PyObject* kwargs)
{
    char*filename = 0;
    static char *kwlist[] = {"filename", NULL};
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &filename))
        return NULL;
    ModelObject*self = PyObject_New(ModelObject, &ModelClass);
    self->model = model_load(filename);
    if(!self->model)
        return PY_ERROR("Couldn't load model from %s", filename);
    return (PyObject*)self;
}
PyDoc_STRVAR(model_new_doc, \
"Model()\n\n"
"Load a model.\n"
);
static PyObject* py_model_new(PyObject* module, PyObject* args, PyObject* kwargs)
{
    return py_model_load(module, args, kwargs);
}
static PyMethodDef model_methods[] =
{
    /* Model functions */
    {"save", (PyCFunction)py_model_save, METH_KEYWORDS, model_save_doc},
    {"predict", (PyCFunction)py_model_predict, METH_KEYWORDS, model_predict_doc},
    {"generate_code", (PyCFunction)py_model_generate_code, METH_KEYWORDS, model_generate_code_doc},
    {0,0,0,0}
};

//---------------------------------------------------------------------
static void dataset_dealloc(PyObject* _self) {
    DataSetObject* self = (DataSetObject*)_self;
    trainingdata_destroy(self->data);
    PyObject_Del(self);
}
static PyObject* dataset_getattr(PyObject * _self, char* a)
{
    DataSetObject*self = (DataSetObject*)_self;
    return forward_getattr(_self, a);
}
static int dataset_setattr(PyObject * self, char* a, PyObject * o) {
    return -1;
}
static int py_dataset_print(PyObject * _self, FILE *fi, int flags)
{
    DataSetObject*self = (DataSetObject*)_self;
    trainingdata_print(self->data);
    return 0;
}
PyDoc_STRVAR(dataset_add_doc, \
"add({feature1:value1,feature2:value2},output)\n\n"
"Adds a row of training data to the model.\n"
);
static PyObject* py_dataset_add(PyObject * _self, PyObject* args, PyObject* kwargs)
{
    DataSetObject*self = (DataSetObject*)_self;
    static char *kwlist[] = {"input","output",NULL};
    PyObject*input=0,*output=0;
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &input, &output))
        return NULL;

    if(!PyList_Check(input) && !PyDict_Check(input)) // && !PyTuple_Check(input))
        return PY_ERROR("first argument to train() must be a list or a dict");

    example_t*e = pylist_to_example(input);
    if(!e)
        return NULL;
    if(PyInt_Check(output)) {
        e->desired_response = variable_new_categorical(PyInt_AS_LONG(output));
    } else if(PyString_Check(output)) {
        e->desired_response = variable_new_text(PyString_AsString(output));
    } else {
        return PY_ERROR("output parameter must be an integer or a string");
    }

    trainingdata_add_example(self->data, e);
    return PY_NONE;
}
PyDoc_STRVAR(dataset_get_model_doc, \
"get_model()\n\n"
"Train a classifier\n"
);
static PyObject* py_dataset_get_model(PyObject*_self, PyObject* args, PyObject* kwargs)
{
    DataSetObject*self = (DataSetObject*)_self;
    static char *kwlist[] = {"name", NULL};
    const char*name = 0;
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "|s", kwlist, &name))
        return NULL;

    int num_examples = self->data->num_examples;
    if(!num_examples) {
        return PY_ERROR("No training data given. Can't build a model from no data.");
    }
    if(!trainingdata_check_format(self->data)) {
        return PY_ERROR("bad training data");
    }


    model_t*model = NULL;
    if(name == NULL) {
        model = trainingdata_train(self->data);
    } else {
        model = trainingdata_train_specific_model(self->data, name);
        if(!model)
            return PY_ERROR("unknown model %s", name);
    }

    if(!model)
        return PY_NONE;
    ModelObject*ret = PyObject_New(ModelObject, &ModelClass);
    ret->model = model;
    return (PyObject*)ret;
}
PyDoc_STRVAR(dataset_save_doc, \
"save(filename)\n\n"
"Save training data to a file.\n"
);
static PyObject* py_dataset_save(PyObject*_self, PyObject* args, PyObject* kwargs)
{
    DataSetObject*self = (DataSetObject*)_self;
    static char *kwlist[] = {"filename", NULL};
    const char*filename = 0;
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &filename))
        return NULL;
    trainingdata_save(self->data, filename);
    return PY_NONE;
}
PyDoc_STRVAR(dataset_load_doc, \
"load_data()\n\n"
"Load a dataset.\n"
);
static PyObject* py_dataset_load(PyObject* module, PyObject* args, PyObject* kwargs)
{
    char*filename = 0;
    static char *kwlist[] = {"filename", NULL};
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &filename))
        return NULL;
    DataSetObject*self = PyObject_New(DataSetObject, &DataSetClass);
    self->data = trainingdata_load(filename);
    if(!self->data)
        return PY_ERROR("Couldn't load model from %s", filename);
    return (PyObject*)self;
}
PyDoc_STRVAR(dataset_new_doc, \
"DataSet()\n\n"
"Creates a new (initially empty) dataset.\n"
);
static PyObject* py_dataset_new(PyObject* module, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {NULL};
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
        return NULL;
    DataSetObject*self = PyObject_New(DataSetObject, &DataSetClass);
    self->data = trainingdata_new();
    return (PyObject*)self;
}
static PyMethodDef dataset_methods[] =
{
    /* DataSet functions */
    {"add", (PyCFunction)py_dataset_add, METH_KEYWORDS, dataset_add_doc},
    {"train", (PyCFunction)py_dataset_get_model, METH_KEYWORDS, dataset_get_model_doc},
    {"get_model", (PyCFunction)py_dataset_get_model, METH_KEYWORDS, dataset_get_model_doc},
    {"save", (PyCFunction)py_dataset_save, METH_KEYWORDS, dataset_save_doc},
    {0,0,0,0}
};

//---------------------------------------------------------------------

#ifndef PYTHON3
#define PYTHON23_HEAD_INIT \
    PyObject_HEAD_INIT(NULL) \
    0,
#else
#define PYTHON23_HEAD_INIT \
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
#endif

PyDoc_STRVAR(dataset_doc,
"A DataSet object stores training data (examples of features and\n"
"desired output values). DataSets can be converted to models in\n"
"order to do prediction.\n"
);
static PyTypeObject DataSetClass =
{
    PYTHON23_HEAD_INIT
    tp_name: "predict.DataSet",
    tp_basicsize: sizeof(DataSetObject),
    tp_itemsize: 0,
    tp_dealloc: dataset_dealloc,
    tp_print: py_dataset_print,
    tp_getattr: dataset_getattr,
    tp_setattr: dataset_setattr,
    tp_doc: dataset_doc,
    tp_methods: dataset_methods
};
PyDoc_STRVAR(model_doc,
"A Model can be used to predict values from (so far unknown)\n"
"input data. Models are \"lightweight\", in that they don't\n"
"store any data other than that needed to do predictions (in\n"
"particular, they don't contain any examplicit training data)\n"
);
static PyTypeObject ModelClass =
{
    PYTHON23_HEAD_INIT
    tp_name: "predict.Model",
    tp_basicsize: sizeof(ModelObject),
    tp_itemsize: 0,
    tp_dealloc: model_dealloc,
    tp_print: py_model_print,
    tp_getattr: model_getattr,
    tp_setattr: model_setattr,
    tp_doc: model_doc,
    tp_methods: model_methods
};

//=====================================================================

PyDoc_STRVAR(mrscake_add_server_doc, \
"add_server(host, port=3075)\n\n"
);
static PyObject* mrscake_add_server(PyObject* module, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"host", "port", NULL};
    char*host=0;
    int port=3075;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i", kwlist, &host, &port))
        return NULL;
    state_t*state = STATE(module);
    config_add_remote_server(host, port);
    return PY_NONE;
}

PyDoc_STRVAR(mrscake_setparameter_doc, \
"setparameter(key,value)\n\n"
);
static PyObject* mrscake_setparameter(PyObject* module, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"key", "value", NULL};
    char*key=0,*value=0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss", kwlist, &key, &value))
        return NULL;
    state_t*state = STATE(module);
    return PY_NONE;
}

static PyMethodDef mrscake_methods[] =
{
    {"add_server", (PyCFunction)mrscake_add_server, M_FLAGS, mrscake_add_server_doc},
    {"setparameter", (PyCFunction)mrscake_setparameter, M_FLAGS, mrscake_setparameter_doc},
    {"load_model", (PyCFunction)py_model_load, M_FLAGS, model_load_doc},
    {"load_data", (PyCFunction)py_dataset_load, M_FLAGS, dataset_load_doc},

    {"DataSet", (PyCFunction)py_dataset_new, M_FLAGS, dataset_new_doc},
    {"Model", (PyCFunction)py_model_new, M_FLAGS, model_new_doc},

    /* sentinel */
    {0, 0, 0, 0}
};

PyDoc_STRVAR(mrscake_doc, \
"Data prediction python wrapper\n"
);
void mrscake_free(void*module)
{
    state_t*state = STATE(module);
    memset(state, 0, sizeof(state_t));
}

#ifdef PYTHON3
static struct PyModuleDef mrscake_moduledef = {
        PyModuleDef_HEAD_INIT,
        "mrscake",
        mrscake_doc,
        sizeof(state_t),
        mrscake_methods,
        /*reload*/NULL,
        /*traverse*/NULL,
        /*clear*/NULL,
        mrscake_free,
};
#endif

PyObject * PyInit_mrscake(void)
{
#ifdef PYTHON3
    PyObject*module = PyModule_Create(&mrscake_moduledef);
#else
    PyObject*module = Py_InitModule3("mrscake", mrscake_methods, mrscake_doc);
    ModelClass.ob_type = &PyType_Type;
    DataSetClass.ob_type = &PyType_Type;
#endif

    state_t* state = STATE(module);
    memset(state, 0, sizeof(state_t));

    //PyObject*module_dict = PyModule_GetDict(module);
    //PyDict_SetItemString(module_dict, "DataSet", (PyObject*)&DataSetClass);
    //PyDict_SetItemString(module_dict, "Model", (PyObject*)&ModelClass);
    return module;
}
#ifndef PYTHON3
void initmrscake(void) {
    PyInit_mrscake();
}
#endif
