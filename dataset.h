#ifndef __dataset_h__
#define __dataset_h__

#include <stdbool.h>
#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _sanitized_dataset {
    int num_columns;
    int num_rows;
    bool*column_is_categorical;
    dictionary_t*word2category;
    example_t*rows;
} sanitized_dataset_t;

example_t**example_list_to_array(dataset_t*d);

sanitized_dataset_t* dataset_sanitize(dataset_t*dataset);

#ifdef __cplusplus
}
#endif
#endif
