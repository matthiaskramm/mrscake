/* model_knearest.cpp
   k-nearest neighbors model

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
#include "mrscake.h"
#include "dataset.h"
#include "easy_ast.h"
#include "model_select.h"
#include "transform.h"

typedef struct _knearest_model_factory {
    model_factory_t head;
    int k;
} knearest_model_factory_t;

static node_t*knearest_train(knearest_model_factory_t*factory, dataset_t*d)
{
    d = expand_categorical_columns(d);

    int * class_count = d->desired_response->class_occurence_count;
    int num_classes = d->desired_response->num_classes;
    int * class_pos = calloc(num_classes, sizeof(int));
    int k = factory->k;
    
    START_CODE(code)
    BLOCK
        int cls;
        for(cls=0;cls<num_classes;cls++) {
            assert(class_count[cls]>0);
            SETLOCAL(cls);
                NEW_ZERO_FLOAT_ARRAY(class_count[cls]);
            END;
        }
        int i,j;
        for(i=0;i<d->num_rows;i++) {
            int cls = d->desired_response->entries[i].c;
            SET_ARRAY_AT_POS
                GETLOCAL(cls); 
                INT_CONSTANT(class_pos[cls]);
                ADD
                for(j=0;j<d->num_columns;j++) {
                    SQR
                        SUB
                            FLOAT_CONSTANT(d->columns[j]->entries[i].f);
                            PARAM(j);
                        END;
                    END;
                }
                END;
            END;
            class_pos[cls]++;
        }
        for(cls=0;cls<num_classes;cls++) {
            /* FIXME: we might want to only pick the k*num_classes best entries,
                      not sort the entire array */
            SORT_FLOAT_ARRAY_ASC
                GETLOCAL(cls);
            END;
        }
        int position_var = num_classes;
        int class_count_minus_one_var = num_classes+1;
        int best_class = num_classes+2;
        int loop_counter = num_classes+3;

        SETLOCAL(position_var);
            NEW_ZERO_INT_ARRAY(num_classes);
        END;
        SETLOCAL(class_count_minus_one_var);
            array_t*a = array_new(num_classes);
            for(cls=0;cls<num_classes;cls++) {
                a->entries[cls] = int_constant(class_count[cls]-1);
            }
            ARRAY_CONSTANT(a);
        END;
        FOR_LOCAL_FROM_N_TO_M(loop_counter)
            INT_CONSTANT(0);
            INT_CONSTANT(k);
            BLOCK
                SETLOCAL(best_class);
                    ARG_MIN_F
                        for(cls=0;cls<num_classes;cls++) {
                            ARRAY_AT_POS
                                GETLOCAL(cls);
                                ARRAY_AT_POS
                                    GETLOCAL(position_var);
                                    INT_CONSTANT(cls);
                                END;
                            END;
                        }
                    END;
                END;
                /* we do a simplification here: if any given category c is maxed out (i.e.,
                   *all* the examples of that category are within the k nearest entries), we
                   keep the worst example around for further comparisons. That means that
                   this class will potentially shadow other classes that have more entries
                   in the nearest neighbors, but less entries that are better than the worst
                   entry of class c. This is usually what you want, anyway.
                */
                IF
                    LT_I
                        ARRAY_AT_POS
                            GETLOCAL(position_var);
                            GETLOCAL(best_class);
                        END;
                        ARRAY_AT_POS
                            GETLOCAL(class_count_minus_one_var);
                            GETLOCAL(best_class);
                        END;
                    END;
                THEN
                    INC_ARRAY_AT_POS
                        GETLOCAL(position_var);
                        GETLOCAL(best_class);
                    END;
                ELSE
                    NOP;
                END;
            END;
        END;
        RETURN
            ARRAY_AT_POS
                ARRAY_CONSTANT(dataset_classes_as_array(d));
                ARRAY_ARG_MAX_I
                    GETLOCAL(position_var);
                END;
            END;
        END;
    END;
    END_CODE;
            
    d = dataset_revert_one_transformation(d, &code);
    return code;
}
static knearest_model_factory_t knearest_model_factory_1 = {
    head: {
        name: "knearest_1",
        train: (training_function_t)knearest_train,
    },
    k: 1,
};
static knearest_model_factory_t knearest_model_factory_2 = {
    head: {
        name: "knearest_2",
        train: (training_function_t)knearest_train,
    },
    k: 2,
};
static knearest_model_factory_t knearest_model_factory_4 = {
    head: {
        name: "knearest_4",
        train: (training_function_t)knearest_train,
    },
    k: 4,
};

model_factory_t* knearest_models[] =
{
    (model_factory_t*)&knearest_model_factory_1,
    (model_factory_t*)&knearest_model_factory_2,
    (model_factory_t*)&knearest_model_factory_4,
};
int num_knearest_models = sizeof(knearest_models) / sizeof(knearest_models[0]);
