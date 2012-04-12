#include <stdlib.h>
#include <signal.h>
#include "../mrscake.h"
#include "../model_select.h"
#include "language_interpreter.h"

trainingdata_t* trainingdata1(int width, int height)
{
    trainingdata_t* data = trainingdata_new();

    int t;
    for(t=0;t<height;t++) {
        example_t*e = example_new(width);
        int s;
        for(s=0;s<width;s++) {
            e->inputs[s] = variable_new_continuous(lrand48()&255);
        }
        e->inputs[2] = variable_new_continuous(((t%2)+1)*100+(lrand48()&15));
        e->desired_response = variable_new_categorical(t&1);
        trainingdata_add_example(data, e);
    }
    return data;
}

void test_language(language_interpreter_t*lang)
{
    trainingdata_t*tdata1 = trainingdata1(16, 256);
    dataset_t*data1 = trainingdata_sanitize(tdata1);

    const char*const*model_names = mrscake_get_model_names();
    while(*model_names) {
        const char*model_name = *model_names++;
        model_t*model = model_train_specific_model(data1, model_name);
        bool fail_generate = model == NULL;
        bool fail_predict = false;
        bool fail_define_function = false;
        bool fail_call_function = false;
        int count_total = 0;
        int count_good = 0;
        if(model) {
            char*code = model_generate_code(model, lang->name);
            bool success = lang->define_function(lang, code);
            fail_define_function = !success;
            if(success) {
                example_t*e;
                for(e = tdata1->first_example; e; e = e->next) {
                    row_t*r = example_to_row(e, 0);
                    variable_t v = model_predict(model, r);
                    int c = lang->call_function(lang, r);
                    if(c < 0) {
                        fail_call_function = true;
                        break;
                    }
                    if(v.category != c) {
                        fail_predict = true;
                    } else {
                        count_good++;
                    }
                    count_total++;
                }
            }
        }
        if(fail_generate) {
            printf("[     ] %s %s\n", lang->name, model_name);
        } else if(fail_define_function) {
            printf("[EFUNC] %s %s\n", lang->name, model_name);
        } else if(fail_call_function) {
            printf("[ECALL] %s %s\n", lang->name, model_name);
        } else if(fail_predict) {
            printf("[%02x/%02x] %s %s\n", count_good, count_total, lang->name, model_name);
        } else {
            printf("[ OK! ] %s %s\n", lang->name, model_name);
        }
    }
    lang->destroy(lang);
    dataset_destroy(data1);
}

int main(int argn, char*argv[])
{
    if(argn > 1 && !strcmp(argv[1], "js")) {
        language_interpreter_t*lang = javascript_interpreter_new();
        test_language(lang);
    } else if (argn > 1 && !strcmp(argv[1], "rb")) {
        language_interpreter_t*ruby = ruby_interpreter_new();
        test_language(ruby);
    } else {
        language_interpreter_t*python = python_interpreter_new();
        test_language(python);
    }
}
