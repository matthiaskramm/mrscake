#ifndef __ast_transforms__
#define __ast_transforms__

#include "ast.h"

node_t* node_prepare_for_code_generation(node_t*n);
node_t* node_insert_brackets(node_t*n) ;
node_t* node_cascade_returns(node_t*n) ;
bool node_has_consumer_parent(node_t*n);
node_t* node_add_return(node_t*n);
bool node_has_minus_prefix(node_t*n);
bool node_is_missing(node_t*n);
bool node_terminates(node_t*n);

#endif
