#include <stdio.h>
#include "var_selection.h"

void varorder_print(varorder_t*order, int num)
{
    int t;
    printf("[");
    for(t=0;t<num;t++) {
        if(t)
            printf(" ");
        printf("%d ", order->order[t]);
    }
    printf("]\n");
}
