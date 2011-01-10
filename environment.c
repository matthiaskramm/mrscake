#include <stdlib.h>
#include <memory.h>
#include "environment.h"
#include "ast.h"

environment_t* environment_new(void*node, row_t*row)
{
    environment_t*e = (environment_t*)malloc(sizeof(environment_t));
    e->num_locals = node_highest_local((node_t*)node)+1;
    e->locals = (constant_t*)calloc(sizeof(constant_t), e->num_locals);
    e->row = row;
    return e;
}

void environment_destroy(environment_t*e)
{
    free(e->locals);
    free(e);
}
