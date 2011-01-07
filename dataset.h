#ifndef __dataset_h__
#define __dataset_h__

#include <stdbool.h>
#include "model.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _column;
typedef struct _column column_t;

typedef struct _sanitized_dataset {
    int num_columns;
    int num_rows;
    column_t**columns;

    column_t*desired_response;
} sanitized_dataset_t;

struct _column {
    bool is_categorical;

    int num_classes;
    constant_t*classes;
    
    union {
        float f;
        category_t c;
    } entries[0];
};

void sanitized_dataset_destroy(sanitized_dataset_t*dataset);
example_t**example_list_to_array(dataset_t*d);
sanitized_dataset_t* dataset_sanitize(dataset_t*dataset);
void sanitized_dataset_print(sanitized_dataset_t*s);

#ifdef __cplusplus
}
#endif
#endif
