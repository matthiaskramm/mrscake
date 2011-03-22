#ifndef __var_selection_h__
#define __var_selection_h__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _varorder {
    int num;
    int*order;
} varorder_t;

void varorder_print(varorder_t*order, int num);

#ifdef __cplusplus
}
#endif
#endif //__var_selection_h__
