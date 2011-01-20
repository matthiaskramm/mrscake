/* predict.rb.c

   Ruby wrapper for the prediction library

   Part of the prediction package.

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

#include <ruby.h>
#include "model.h"

static VALUE predict;
static VALUE DataSet, Model;
static ID id_doc;

typedef struct dataset_internal {
    trainingdata_t* trainingdata;
} dataset_internal_t;

typedef struct model_internal {
    model_t* model;
} model_internal_t;

#define Get_DataSet(d,cls) dataset_internal_t*d=0;Data_Get_Struct(cls, dataset_internal_t, d);
#define Get_Model(m,cls) model_internal_t*m=0;Data_Get_Struct(cls, model_internal_t, m);

static VALUE rb_model_allocate(VALUE cls);
static VALUE rb_dataset_allocate(VALUE cls);

// ------------------------ dataset ------------------------------------

variable_t value_to_variable(VALUE v)
{
    if(TYPE(v) == T_SYMBOL) {
        return variable_make_text(rb_id2name(SYM2ID(v)));
    } else if(TYPE(v) == T_FLOAT) {
        return variable_make_continuous(NUM2DBL(v));
    } else if(TYPE(v) == T_FIXNUM) {
        return variable_make_continuous(INT2DBL(v));
    } else {
        return variable_make_missing();
    }
}

static VALUE rb_dataset_add(VALUE cls, VALUE input, VALUE response)
{
    Get_DataSet(dataset,cls);
    if(TYPE(input) != T_ARRAY)
	rb_raise(rb_eArgError, "first argument of add must be an array");
    int len = RARRAY(input)->len;
    int t;
    example_t*e = example_new(len);
    for(t=0;t<len;t++) {
        VALUE item = RARRAY(input)->ptr[t];
        e->inputs[t] = value_to_variable(item);
        if(e->inputs[t].type == MISSING) {
            rb_raise(rb_eArgError, "bad element in array at pos %d", t+1);
        }
    }
    e->desired_response = value_to_variable(response);
    if(e->desired_response.type == MISSING) {
        rb_raise(rb_eArgError, "bad element in array at pos %d", t+1);
    }
    trainingdata_add_example(dataset->trainingdata, e);
    return cls;
}
static VALUE rb_dataset_get_model(VALUE cls)
{
    Get_DataSet(dataset,cls);
    VALUE model_value = rb_model_allocate(Model);
    Get_Model(model, model_value);
    model->model = model_select(dataset->trainingdata);
    if(!model->model)
	rb_raise(rb_eArgError, "bad (empty?) data");
    return model_value;
}
static VALUE rb_dataset_print(VALUE cls)
{
    Get_DataSet(dataset,cls);
    trainingdata_print(dataset->trainingdata);
    return cls;
}
static void rb_dataset_mark(dataset_internal_t*dataset)
{
}
static void rb_dataset_free(dataset_internal_t*dataset)
{
    if(dataset->trainingdata) {
        trainingdata_destroy(dataset->trainingdata);
        dataset->trainingdata = 0;
    }
    free(dataset);
}
static VALUE rb_dataset_allocate(VALUE cls)
{
    dataset_internal_t*dataset = 0;
    VALUE v = Data_Make_Struct(cls, dataset_internal_t, rb_dataset_mark, rb_dataset_free, dataset);
    memset(dataset, 0, sizeof(dataset_internal_t));
    dataset->trainingdata = trainingdata_new();
    return v;
}

// ------------------------ model ---------------------------------------

static void rb_model_free(model_internal_t*model)
{
    if(!model) return;
    if(model->model) {
	model_destroy(model->model);
	model->model = 0;
    }
    free(model);
}
static VALUE rb_model_print(VALUE cls)
{
    Get_Model(model,cls);
    model_print(model->model);
    return cls;
}
static VALUE rb_model_save(VALUE cls, VALUE _filename)
{
    Check_Type(_filename, T_STRING);
    const char*filename = StringValuePtr(_filename);
    Get_Model(model,cls);
    model_save(model->model, filename);
    return cls;
}
static VALUE rb_model_generate_code(VALUE cls, VALUE _language)
{
    Check_Type(_language, T_STRING);
    const char*language = StringValuePtr(_language);
    Get_Model(model,cls);
    char*code = model_generate_code(model->model, language);
    return rb_str_new2(code);
}
static VALUE rb_load_model(VALUE module, VALUE _filename)
{
    Check_Type(_filename, T_STRING);
    const char*filename = StringValuePtr(_filename);
    VALUE cls = rb_model_allocate(Model);
    Get_Model(model,cls);
    model->model = model_load(filename);
    if(!model->model) {
	rb_raise(rb_eIOError, "couldn't open %s", filename);
    }
    return cls;
}
static VALUE rb_model_predict(VALUE cls, VALUE input)
{
    Get_Model(model,cls);
    int len = RARRAY(input)->len;
    row_t*row = row_new(len);
    if(len != model->model->num_inputs)
        rb_raise(rb_eArgError, "You supplied %d inputs for a model with %d inputs", len, model->model->num_inputs);
    int t;
    for(t=0;t<len;t++) {
        VALUE item = RARRAY(input)->ptr[t];
        row->inputs[t] = value_to_variable(item);
        if(row->inputs[t].type == MISSING) {
            rb_raise(rb_eArgError, "bad element in array at pos %d", t+1);
        }
    }
    variable_t prediction = model_predict(model->model, row);
    row_destroy(row);

    if(prediction.type == CONTINUOUS)
        return rb_float_new(prediction.value);
    else if(prediction.type == CATEGORICAL)
        return INT2FIX(prediction.category);
    else if(prediction.type == TEXT)
        return rb_str_new2(prediction.text);
    else
        return T_NIL;
}
static void rb_model_mark(model_internal_t*model)
{
}
static VALUE rb_model_allocate(VALUE cls)
{
    model_internal_t*model = 0;
    VALUE v = Data_Make_Struct(cls, model_internal_t, rb_model_mark, rb_model_free, model);
    memset(model, 0, sizeof(model_internal_t));
    return v;
}

// --------------------------------------------------------------------------

void Init_prediction()
{
    predict = rb_define_module("Prediction");

    rb_define_module_function(predict, "load_model", rb_load_model, 1);

    DataSet = rb_define_class_under(predict, "DataSet", rb_cObject);
    rb_define_alloc_func(DataSet, rb_dataset_allocate);
    rb_define_method(DataSet, "add", rb_dataset_add, 2);
    rb_define_method(DataSet, "get_model", rb_dataset_get_model, 0);
    rb_define_method(DataSet, "print", rb_dataset_print, 0);

    Model = rb_define_class_under(predict, "Model", rb_cObject);
    rb_define_alloc_func(Model, rb_model_allocate);
    rb_define_method(Model, "print", rb_model_print, 0);
    rb_define_method(Model, "save", rb_model_save, 1);
    rb_define_method(Model, "predict", rb_model_predict, 1);
    rb_define_method(Model, "generate_code", rb_model_generate_code, 1);
}

