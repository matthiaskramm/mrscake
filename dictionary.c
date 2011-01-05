#include <stdlib.h>
#include "model.h"

typedef struct _internal {
    int num_entries;
} internal_t;

dictionary_t* dictionary_new()
{
    dictionary_t*d = (dictionary_t*)malloc(sizeof(dictionary_t));
    d->internal = (internal_t*)malloc(sizeof(internal_t));
    return d;
}
category_t dictionary_find_word(const char*word)
{
    return 0;
}
category_t dictionary_find_or_add_word(const char*word)
{
    return 0;
}
const char* dictionary_find_category(category_t c)
{
    return "XY";
}
void dictionary_destroy(dictionary_t*dict)
{
    free(dict->internal);
    free(dict);
}

