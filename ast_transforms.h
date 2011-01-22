/* ast_transforms.h
   AST transformations and queries

   Part of the data prediction package.
   
   Copyright (c) 2011 Matthias Kramm <kramm@quiss.org> 
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

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

constant_type_t node_type(node_t*n, model_t*m);
int node_array_size(node_t*n);

int node_highest_local(node_t*node);
constant_type_t*node_local_types(node_t*node, model_t*m, int* num_locals);
constant_type_t model_param_type(model_t*m, int var);
bool node_has_child(node_t*n, nodetype_t*type);


#endif
