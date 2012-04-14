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

void test_language(language_interpreter_t*lang, int test_num)
{
    trainingdata_t*tdata1 = trainingdata1(16, 256);
    dataset_t*data1 = trainingdata_sanitize(tdata1);

    bool verbose = test_num>0;

    const char*const*model_names = mrscake_get_model_names();
    int count = 0;
    while(*model_names) {
        const char*model_name = *model_names++;
        count++;
        if(test_num > 0 && count != test_num) {
            continue;
        }
        model_t*model = model_train_specific_model(data1, model_name);
        bool fail_generate = model == NULL;
        bool fail_predict = false;
        bool fail_define_function = false;
        bool fail_call_function = false;
        int count_total = 0;
        int count_good = 0;
        if(model) {
            if(verbose) {
                printf("-------------------------------------------------------------\n");
                model_print(model);
            }
            char*code = model_generate_code(model, lang->name);
            if(verbose) {
                printf("-------------------------------------------------------------\n");
                printf("%s\n", code);
                printf("-------------------------------------------------------------\n");
            }

            bool success = lang->define_function(lang, code);
            fail_define_function = !success;
            if(success) {
                example_t*e;
                for(e = tdata1->first_example; e; e = e->next) {
                    row_t*r = example_to_row(e, 0);
                    if(verbose) {
                        printf("debug output (ast):\n");
                    }
                    variable_t v = model_predict(model, r);
                    if(verbose) {
                        printf("debug output (%s):\n", lang->name);
                    }
                    int c = lang->call_function(lang, r);
                    if(c < 0) {
                        fail_call_function = true;
                        break;
                    }
                    if(v.category != c) {
                        if(verbose) {
                            printf("-------------------------------------------------------------\n");
                            printf("%s\n", row_to_function_call(r, malloc(16384), true));
                            row_print(r);
                            printf("ast: %d\n", v.category);
                            printf("%s: %d\n", lang->name, c);
                            variable_t v = model_predict(model, r);
                        }
                        fail_predict = true;
                        if(test_num > 0)
                            break;
                    } else {
                        count_good++;
                    }
                    count_total++;
                }
            }
        }
        if(fail_generate) {
            printf("%5d [       ] %s %s\n", count, lang->name, model_name);
        } else if(fail_define_function) {
            printf("%5d [ERR DEF] %s %s\n", count, lang->name, model_name);
        } else if(fail_call_function) {
            printf("%5d [ERR RUN] %s %s\n", count, lang->name, model_name);
        } else if(fail_predict) {
            printf("%5d [%3d/%3d] %s %s\n", count, count_good, count_total, lang->name, model_name);
        } else {
            printf("%5d [  OK!  ] %s %s\n", count, lang->name, model_name);
        }
    }
    lang->destroy(lang);
    dataset_destroy(data1);
}

int main(int argn, char*argv[])
{
    char*language = "py";
    int test_number = -1;

    if(argn > 1) {
        language= argv[1];
    }
    if(argn > 2) {
        test_number = atoi(argv[2]);
    }
    language_interpreter_t*lang;

    if(!strcmp(language, "js")) {
        lang = javascript_interpreter_new();
    } else if(!strcmp(language, "py")) {
        lang = python_interpreter_new();
    } else if(!strcmp(language, "rb")) {
        lang = ruby_interpreter_new();
    } else {
        fprintf(stderr, "No such language: %s\n", language);
        exit(1);
    }

    if(test_number > 0) {
        lang->verbosity = 1;
    }

    test_language(lang, test_number);
}
