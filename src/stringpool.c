/* stringpool.c
   Static dictionary storing constant strings

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

#include <stdlib.h>
#include <string.h>
#include "stringpool.h"
#include "dict.h"

static dict_t*stringpool = 0;

const char*register_string(const char*s)
{
    if(!stringpool) {
        stringpool = dict_new(&charptr_type);
    }
    char*stored_string = dict_lookup(stringpool, s);
    if(!stored_string) {
        stored_string = (char*)strdup(s);
        dict_put(stringpool, s, stored_string);
    }
    return stored_string;
}
const char*register_string_n(const char*s, int count)
{
    int l = strlen(s);
    if(l <= count)
        return register_string(s);

    char*n = malloc(count+1);
    memcpy(n, s, count);
    n[count] = 0;
    const char*r = register_string(n);
    free(n);
    return r;
}
const char*register_and_free_string(char*s)
{
    const char*r = register_string(s);
    free(s);
    return r;
}
