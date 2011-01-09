/* wordmap.c
   Deprecated

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

#include <stdlib.h>
#include <string.h>
#include "model.h"
#include "dict.h"

typedef struct _internal {
    int num_entries;
    dict_t*word2index;
    dict_t*index2word;
} internal_t;

wordmap_t* wordmap_new()
{
    internal_t*i = (internal_t*)calloc(1, sizeof(internal_t));
    wordmap_t*d = (wordmap_t*)malloc(sizeof(wordmap_t));
    d->internal = i;
    i->word2index = dict_new(&charptr_type);
    i->index2word = dict_new(&int_type);
    return d;
}
category_t wordmap_find_word(wordmap_t*d, const char*word)
{
    internal_t*i = (internal_t*)d->internal;
    category_t c = PTR_TO_INT(dict_lookup(i->word2index, word));
    return c;
}
category_t wordmap_find_or_add_word(wordmap_t*d, const char*word)
{
    internal_t*i = (internal_t*)d->internal;
    category_t c = PTR_TO_INT(dict_lookup(i->word2index, word));
    if(!c) {
        c = ++i->num_entries;
        dict_put(i->word2index, word, INT_TO_PTR(c));
        dict_put(i->index2word, INT_TO_PTR(c), strdup(word));
    }
    return c;
}
const char* wordmap_find_category(wordmap_t*d, category_t c)
{
    internal_t*i = (internal_t*)d->internal;
    return dict_lookup(i->index2word, INT_TO_PTR(c));
}
void wordmap_destroy(wordmap_t*d)
{
    internal_t*i = (internal_t*)d->internal;
    dict_destroy(i->word2index);
    dict_destroy_with_data(i->index2word);
    free(d->internal);
    free(d);
}
void wordmap_convert_row(wordmap_t*d, row_t*row)
{
    int i;
    for(i=0;i<row->num_inputs;i++) {
        if(row->inputs[i].type == TEXT) {
            row->inputs[i].category = wordmap_find_word(d, row->inputs[i].text);
            row->inputs[i].type = CATEGORICAL;
        }
    }
}
