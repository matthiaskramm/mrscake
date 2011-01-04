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
