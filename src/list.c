/* list.c
   Linked list implementation

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
#include <memory.h>
#include "list.h"

struct _commonlist;
typedef struct _listinfo {
    int size;
    struct _commonlist*last;
} listinfo_t;

typedef struct _commonlist {
    void*entry;
    struct _commonlist*next;
    listinfo_t info[0];
} commonlist_t;

int list_length_(void*_list)
{
    commonlist_t*l = (commonlist_t*)_list;
    if(!l)
        return 0;
    return l->info[0].size;
}
void list_concat_(void*_l1, void*_l2)
{
    commonlist_t**l1 = (commonlist_t**)_l1;
    commonlist_t**l2 = (commonlist_t**)_l2;

    if(!*l1) {
        *l1 = *l2;
    } else if(*l2) {
        (*l1)->info[0].last->next = *l2;
        (*l1)->info[0].last = (*l2)->info[0].last;
        (*l1)->info[0].size += (*l2)->info[0].size;
    }
    *l2 = 0;
}
void list_append_(void*_list, void*entry)
{
    commonlist_t**list = (commonlist_t**)_list;
    commonlist_t* n = 0;
    if(!*list) {
        n = (commonlist_t*)malloc(sizeof(commonlist_t)+sizeof(listinfo_t));
        *list = n;
        (*list)->info[0].size = 0;
    } else {
        n = malloc(sizeof(commonlist_t));
        (*list)->info[0].last->next = n;
    }
    n->next = 0;
    n->entry = entry;
    (*list)->info[0].last = n;
    (*list)->info[0].size++;
}
/* notice: prepending uses slighly more space than appending */
void list_prepend_(void*_list, void*entry)
{
    commonlist_t**list = (commonlist_t**)_list;
    commonlist_t* n = (commonlist_t*)malloc(sizeof(commonlist_t)+sizeof(listinfo_t));
    int size = 0;
    commonlist_t* last = 0;
    if(*list) {
        last = (*list)->info[0].last;
        size = (*list)->info[0].size;
    }
    n->next = *list;
    n->entry = entry;
    *list = n;
    (*list)->info[0].last = last;
    (*list)->info[0].size = size+1;
}
void list_free_(void*_list) 
{
    commonlist_t**list = (commonlist_t**)_list;
    commonlist_t*l = *list;
    while(l) {
        commonlist_t*next = l->next;
        free(l);
        l = next;
    }
    *list = 0;
}
void list_deep_free_(void*_list)
{
    commonlist_t**list = (commonlist_t**)_list;
    commonlist_t*l = *list;
    while(l) {
        commonlist_t*next = l->next;
        if(l->entry) {
            free(l->entry);l->entry=0;
        }
        free(l);
        l = next;
    }
    *list = 0;
}
void*list_clone_(void*_list) 
{
    commonlist_t*l = *(commonlist_t**)_list;

    void*dest = 0;
    while(l) {
        commonlist_t*next = l->next;
        list_append_(&dest, l->entry);
        l = next;
    }
    return dest;

}

