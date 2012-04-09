#include <stdlib.h>
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

char* row_to_function_call(row_t*row, char*buffer)
{
    char*pos = buffer;
    pos += sprintf(pos, "predict(");
    int i;
    for(i=0;i<row->num_inputs;i++) {
        variable_t v = row->inputs[i];
        switch(v.type) {
            case CATEGORICAL:
                pos += sprintf(pos, "%d", v.category);
            break;
            case CONTINUOUS:
                pos += sprintf(pos, "%f", v.value);
            break;
            case TEXT:
                pos += sprintf(pos, "\"%s\"", v.text);
            break;
            case MISSING:
                pos += sprintf(pos, "undefined");
            break;
        }
        if(i < row->num_inputs - 1) {
            pos += sprintf(pos, ", ");
        }
    }
    pos += sprintf(pos, ")");
    return buffer;
}

void test_language(language_interpreter_t*lang)
{
    trainingdata_t*tdata1 = trainingdata1(256, 16);
    dataset_t*data1 = trainingdata_sanitize(tdata1);

    char*buffer = malloc(65536);

    const char*const*model_names = mrscake_get_model_names();
    while(*model_names) {
        const char*model_name = *model_names++;
        model_t*model = model_train_specific_model(data1, model_name);
        bool fail_generate = model == NULL;
        bool fail_predict = false;
        if(model) {
            char*code = model_generate_code(model, lang->name);
            lang->exec(lang, code);

            example_t*e;
            for(e = tdata1->first_example; e; e = e->next) {
                row_t*r = example_to_row(e, 0);
                variable_t v = model_predict(model, r);
                char*script = row_to_function_call(r, buffer);
                int c = lang->eval(lang, script);
                if(v.category != c) {
                    fail_predict = true;
                    break;
                }
            }
        }
        if(fail_generate) {
            printf("[  ] %s %s\n", lang->name, model_name);
        } else if(fail_predict) {
            printf("[EE] %s %s\n", lang->name, model_name);
        } else {
            printf("[OK] %s %s\n", lang->name, model_name);
        }
    }
    lang->destroy(lang);
    dataset_destroy(data1);
}

int main()
{
    language_interpreter_t*lang = javascript_interpreter_new();
    test_language(lang);
}
