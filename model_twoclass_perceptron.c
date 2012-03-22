/* model_perceptron.cpp
   Perceptron model

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

typedef struct _perceptron_model_factory {
    model_factory_t head;
} perceptron_model_factory_t;

category_t predict(dataset_t*d, double*weights, int row)
{
    double result = 0;
    int t;
    for(t=0;t<d->num_columns;t++)
    {
        column_t*column = d->columns[t];
        double v = column->entries[row].f;
        result += weights[t]*v;
    }
    return result > 0 ? 1 : 0;
}

void update_weights(double*weights, dataset_t*d, int row, double eta)
{
    double y = d->desired_response->entries[row].c ? 1 : -1;
    int t;
    for(t=0;t<d->num_columns;t++)
    {
        column_t*column = d->columns[t];
        weights[t] += column->entries[row].f * y;
    }
}

static model_t*twoclass_perceptron_train(perceptron_model_factory_t*factory, dataset_t*d)
{
    int num_iterations = d->num_rows*100;
    double base_eta = 0.1;
    double lastperf = 1.0;
    double currentperf = -1;
    int t;

    double*weights = (double*)calloc(sizeof(double), d->num_columns);

    if(dataset_has_categorical_columns(d))
        return NULL;
    if(d->desired_response->num_classes > 2)
        return NULL;

    double class_to_level[2] = {-1, 1};
    for(t=1;t<num_iterations;t++)
    {
        int i = lrand48() % d->num_rows;
        double eta = base_eta / t;
        if(predict(d, weights, i) != d->desired_response->entries[i].c) {
            update_weights(weights, d, i, eta);
        }
    }

    expanded_columns_t*expanded_columns = expanded_columns_new(d);
    START_CODE(program)
    BLOCK

        IF
            LT
            ADD
                for(t=0;t<d->num_columns;t++) {
                    MUL
                        INSERT_NODE(expanded_columns_parameter_code(expanded_columns, t))
                        FLOAT_CONSTANT(weights[t])
                    END;
                }
            END;
            FLOAT_CONSTANT(0.0);
            END;
        THEN
            GENERIC_CONSTANT(d->desired_response->classes[0]);
        ELSE
            GENERIC_CONSTANT(d->desired_response->classes[1]);
        END;

    END_CODE;
    expanded_columns_destroy(expanded_columns);

    model_t*m = model_new(d);
    m->code = program;
    return m;
}

static perceptron_model_factory_t perceptron_model_factory = {
    head: {
        name: "twoclass_perceptron",
        train: (training_function_t)twoclass_perceptron_train,
    },
};

model_factory_t* twoclass_perceptron_models[] =
{
    (model_factory_t*)&perceptron_model_factory,
};
int num_twoclass_perceptron_models = sizeof(twoclass_perceptron_models) / sizeof(twoclass_perceptron_models[0]);
