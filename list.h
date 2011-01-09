/* list.h
   Linked lists

   Part of the data prediction package.
   
   Copyright (c) 2009-2011 Matthias Kramm <kramm@quiss.org> 
 
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

#ifndef __list_h__
#define __list_h__

#ifdef __cplusplus
extern "C" {
#endif

#define DECLARE(x) struct _##x;typedef struct _##x x##_t;
#define DECLARE_LIST(x) \
struct _##x##_list { \
    struct _##x* x; \
    struct _##x##_list*next; \
}; \
typedef struct _##x##_list x##_list_t;
int list_length_(void*_list);
void*list_clone_(void*_list);
void list_append_(void*_list, void*entry);
void list_prepend_(void*_list, void*entry);
void list_free_(void*_list);
void list_deep_free_(void*_list);
void list_concat_(void*l1, void*l2);
#define list_new() ((void*)0)
#define list_append(list, e) {sizeof((list)->next);list_append_(&(list),(e));}
#define list_concat(l1, l2) {sizeof((l1)->next);sizeof((l2)->next);list_concat_(&(l1),&(l2));}
#define list_prepend(list, e) {sizeof((list)->next);list_prepend_(&(list),(e));}
#define list_free(list) {sizeof((list)->next);list_free_(&(list));}
#define list_deep_free(list) {sizeof((list)->next);list_deep_free_(&(list));}
#define list_clone(list) (sizeof((list)->next),(list?list_clone_(&(list)):0))
#define list_length(list) (sizeof((list)->next),list_length_(list))

#ifdef __cplusplus
}
#endif

#endif
