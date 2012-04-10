#include "language_interpreter.h"

char* row_to_function_call(row_t*row, char*buffer, bool add_brackets)
{
    char*pos = buffer;
    pos += sprintf(pos, "predict(");
    if(add_brackets) {
        *pos++ = '[';
    }
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
    if(add_brackets) {
        *pos++ = ']';
    }
    pos += sprintf(pos, ")");
    return buffer;
}

