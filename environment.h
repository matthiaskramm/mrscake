/* environment.h
   Container for objects visible during AST evaluation.

   Part of the data prediction package.
   
   Copyright (c) 2010-2011 Matthias Kramm <kramm@quiss.org> 
 
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

#ifndef __environment_h__
#define __environment_h__

#include "model.h"
#include "constant.h"

typedef struct _environment {
    row_t* row;
    constant_t* locals;
    int num_locals;
} environment_t;

environment_t* environment_new(void*node, row_t*row);
void environment_destroy(environment_t*e);
#endif
