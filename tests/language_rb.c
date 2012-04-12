#include <assert.h>
#include <ruby.h>
#include <stdbool.h>
#include <string.h>
#include "language_interpreter.h"
#include "../mrscake.h"

typedef struct _rb_internal {
} rb_internal_t;

static int rb_reference_count = 0;

static bool init_rb(rb_internal_t*rb)
{
    if(rb_reference_count==0) {
        ruby_init();
    }
    rb_reference_count++;
}

static void rb_report_error()
{
    VALUE lasterr = rb_gv_get("$!");
    VALUE message = rb_obj_as_string(lasterr);
    char*msg = RSTRING(message)->ptr;
    if(msg && *msg) {
        printf("Ruby Error:\n");
        printf("%s\n", msg);
    }
}

static bool define_function_rb(language_interpreter_t*li, const char*script)
{
    rb_internal_t*rb = (rb_internal_t*)li->internal;
    VALUE ret = rb_eval_string(script);
    rb_report_error();
    return true;
}

static int call_function_rb(language_interpreter_t*li, row_t*row)
{
    rb_internal_t*rb = (rb_internal_t*)li->internal;

    volatile VALUE array = rb_ary_new2(row->num_inputs);
    int i;
    for(i=0;i<row->num_inputs;i++) {
        assert(row->inputs[i].type == CONTINUOUS);
        rb_ary_store(array, i, rb_float_new(row->inputs[i].value));
    }

    volatile ID fname = rb_intern("predict");
    volatile VALUE object = rb_eval_string("Object");
    VALUE ret = rb_funcall(object, fname, 1, array);

    int result = NUM2INT(ret);
    rb_report_error();
    return result;
}

static void destroy_rb(language_interpreter_t* li)
{
    rb_internal_t*rb = (rb_internal_t*)li->internal;
    if(--rb_reference_count == 0) {
        ruby_finalize();
    }
}

language_interpreter_t* ruby_interpreter_new()
{
    language_interpreter_t * li = calloc(1, sizeof(language_interpreter_t));
    li->name = "rb";
    li->define_function = define_function_rb;
    li->call_function = call_function_rb;
    li->destroy = destroy_rb;
    li->internal = calloc(1, sizeof(rb_internal_t));
    rb_internal_t*rb = (rb_internal_t*)li->internal;
    init_rb(rb);
    return li;
}

