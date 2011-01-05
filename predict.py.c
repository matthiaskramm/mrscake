/* predict.py.c

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
#include "model.h"
#include "list.h"

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
    example_list_t*examples;
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
example_t* pylist_to_example(PyObject*input)
{
    if(!PyList_Check(input)) // && !PyTuple_Check(input))
        return PY_ERROR("first argument must be a list");

    int size = PyList_Size(input);
    example_t*e = example_new(size);
    int t;
    for(t=0;t<size;t++) {
        PyObject*item = PyList_GetItem(input, t);
        if(PyInt_Check(item)) {
            e->inputs[t] = variable_make_categorical(PyInt_AS_LONG(item));
        } else if(PyFloat_Check(item)) {
            e->inputs[t] = variable_make_continuous(PyFloat_AS_DOUBLE(item));
/*          FIXME
        } else if(PyString_Check(item)) {
            e->inputs[t] = variable_make_text(PyString_AsString(item));
*/
        } else {
            return PY_ERROR("bad object %s in list", item->ob_type->tp_name);
        }
    }

    return e;
}
static example_t**example_list_to_array(example_list_t*example_list)
{
    int num_examples = list_length(example_list);
    example_t**examples = (example_t**)malloc(sizeof(example_t*)*num_examples);
    int pos = 0;
    example_list_t*i = example_list;
    while(i) {
        examples[pos] = i->example;
        i = i->next;
        pos++;
    }
    return examples;
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
    row_t*row = example_to_row(e);
    category_t i = model_predict(self->model, row);
    return pyint_fromlong(i);
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
    {0,0,0,0}
};

//---------------------------------------------------------------------
static void dataset_dealloc(PyObject* _self) {
    DataSetObject* self = (DataSetObject*)_self;
    list_free(self->examples);
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
static int dataset_print(PyObject * _self, FILE *fi, int flags)
{
    DataSetObject*self = (DataSetObject*)_self;
    int num_examples = list_length(self->examples);
    example_t**examples = example_list_to_array(self->examples);
    examples_print(examples, num_examples);
    free(examples);
    return 0;
}
PyDoc_STRVAR(dataset_train_doc, \
"train(feature1=value1,feature2=value2,...).should_be(5)\n\n"
"Adds a row of training data to the model.\n"
);
static PyObject* dataset_train(PyObject * _self, PyObject* args, PyObject* kwargs)
{
    DataSetObject*self = (DataSetObject*)_self;
    static char *kwlist[] = {"input","output",NULL};
    PyObject*input=0,*output=0;
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &input, &output))
	return NULL;

    if(!PyList_Check(input)) // && !PyTuple_Check(input))
        return PY_ERROR("first argument to train() must be a list");

    example_t*e = pylist_to_example(input);
    if(!e)
        return NULL;
    if(!PyInt_Check(output)) {
        return PY_ERROR("output parameter must be an integer");
    }
    e->desired_output = PyInt_AS_LONG(output);

    list_append(self->examples, e);
    return PY_NONE;
}
PyDoc_STRVAR(dataset_get_model_doc, \
"get_model()\n\n"
"Adds a row of training data to the model.\n"
);
static PyObject* dataset_get_model(PyObject*_self, PyObject* args, PyObject* kwargs)
{
    DataSetObject*self = (DataSetObject*)_self;
    static char *kwlist[] = {NULL};
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
	return NULL;

    int num_examples = list_length(self->examples);
    if(!num_examples) {
        return PY_ERROR("No training data given. Can't build a model from no data.");
    }

    example_t**examples = example_list_to_array(self->examples);
    examples_check_format(examples, num_examples);
    model_t* model = model_select(examples, num_examples);
    free(examples);

    ModelObject*ret = PyObject_New(ModelObject, &ModelClass);
    ret->model = model;
    return (PyObject*)ret;
}
PyDoc_STRVAR(dataset_new_doc, \
"DataSet()\n\n"
"Creates a new (initially empty) dataset.\n"
);
static PyObject* dataset_new(PyObject* module, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {NULL};
    if (args && !PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
	return NULL;
    DataSetObject*self = PyObject_New(DataSetObject, &DataSetClass);
    self->examples = NULL;
    return (PyObject*)self;
}
static PyMethodDef dataset_methods[] =
{
    /* DataSet functions */
    {"train", (PyCFunction)dataset_train, METH_KEYWORDS, dataset_train_doc},
    {"get_model", (PyCFunction)dataset_get_model, METH_KEYWORDS, dataset_get_model_doc},
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
    tp_print: dataset_print,
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

PyDoc_STRVAR(predict_setparameter_doc, \
"setparameter(key,value)\n\n"
);
static PyObject* predict_setparameter(PyObject* module, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"key", "value", NULL};
    char*key=0,*value=0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss", kwlist, &key, &value))
	return NULL;
    state_t*state = STATE(module);
    return PY_NONE;
}

static PyMethodDef predict_methods[] =
{
    {"setparameter", (PyCFunction)predict_setparameter, M_FLAGS, predict_setparameter_doc},
    {"load_model", (PyCFunction)py_model_load, M_FLAGS, model_load_doc},

    {"DataSet", (PyCFunction)dataset_new, M_FLAGS, dataset_new_doc},
    {"Model", (PyCFunction)py_model_new, M_FLAGS, model_new_doc},

    /* sentinel */
    {0, 0, 0, 0}
};

PyDoc_STRVAR(predict_doc, \
"Data prediction python wrapper\n"
);
void predict_free(void*module)
{
    state_t*state = STATE(module);
    memset(state, 0, sizeof(state_t));
}

#ifdef PYTHON3
static struct PyModuleDef predict_moduledef = {
        PyModuleDef_HEAD_INIT,
        "predict",
        predict_doc,
        sizeof(state_t),
        predict_methods,
        /*reload*/NULL,
        /*traverse*/NULL,
        /*clear*/NULL,
        predict_free,
};
#endif

PyObject * PyInit_predict(void)
{
#ifdef PYTHON3
    PyObject*module = PyModule_Create(&predict_moduledef);
#else
    PyObject*module = Py_InitModule3("predict", predict_methods, predict_doc);
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
void initpredict(void) {
    PyInit_predict();
}
#endif
