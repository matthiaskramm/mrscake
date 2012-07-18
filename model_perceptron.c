/* model_perceptron.c
   Perceptron model

   Part of mrscake.
   
   Copyright (c) 2012 Matthias Kramm <kramm@quiss.org> 
 
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

typedef struct _perceptron_model_factory {
    model_factory_t head;
    int halfside;
} perceptron_model_factory_t;

typedef struct _perceptron {
    float**weights;
    bool*disabled;
    int intercept;
} perceptron_t;

static double category_score(perceptron_t*p, category_t c, dataset_t*d, int row)
{
    double result = 0;
    int i;
    for(i=0;i<d->num_columns;i++)
    {
        if(p->disabled[i])
            continue;
        column_t*column = d->columns[i];
        double v = column->entries[row].f;
        result += p->weights[c][i]*v;
    }
    result += p->weights[c][p->intercept];
    return result;
}

static perceptron_t* perceptron_new(dataset_t*d)
{
    perceptron_t*p = (perceptron_t*)malloc(sizeof(perceptron_t));
    p->weights = malloc(sizeof(float*) * d->desired_response->num_classes);
    int i;
    for(i=0;i<d->desired_response->num_classes;i++)
        p->weights[i] = calloc(sizeof(float), d->num_columns + 1);
    p->disabled = calloc(sizeof(bool), d->num_columns);
    p->intercept = d->num_columns;
    return p;
}

static category_t perceptron_predict(perceptron_t*p, dataset_t*d, int row)
{
    int best_category = 0;
    int c;
    double best_score = category_score(p, 0, d, row);
    for(c=1;c<d->desired_response->num_classes;c++)
    {
        double score = category_score(p, c, d, row);
        if(score > best_score) {
            best_score = score;
            best_category = c;
        }
    }
    return best_category;
}

static void perceptron_update_weights(perceptron_t*p, dataset_t*d, int row)
{
    category_t label = d->desired_response->entries[row].c;
    category_t guess = perceptron_predict(p, d, row);
    if(label == guess)
        return;

    int x;
    for(x=0;x<d->num_columns;x++)
    {
        if(p->disabled[x])
            continue;
        column_t*column = d->columns[x];
        double d = column->entries[row].f;
        p->weights[label][x] += d;
        p->weights[guess][x] -= d;
    }
    p->weights[label][p->intercept] += 1;
    p->weights[guess][p->intercept] -= 1;
}

void perceptron_train(perceptron_t*p, dataset_t*d)
{
    int num_iterations = d->num_rows*100;
    int i;
    for(i=0;i<num_iterations;i++)
    {
        int row = lrand48() % d->num_rows;
        if(perceptron_predict(p, d, row) != d->desired_response->entries[row].c) {
            perceptron_update_weights(p, d, row);
        }
    }
}

void perceptron_train_halfsided(perceptron_t*p, dataset_t*d, int sign)
{
    int num_iterations = d->num_rows*100;
    int i;
    int drop;
    int num_disabled = 0;
    do {
        for(i=0;i<num_iterations;i++)
        {
            int row = lrand48() % d->num_rows;
            if(perceptron_predict(p, d, row) != d->desired_response->entries[row].c) {
                perceptron_update_weights(p, d, row);
            }
        }

#define DEBUG
#ifdef DEBUG
        int right = 0;
        for(i=0;i<d->num_rows;i++) {
            if(perceptron_predict(p, d, i) == d->desired_response->entries[i].c) {
                right++;
            }
        }
        printf("%.1f\n", right * 100.0 / d->num_rows);
#endif

        int x;
        drop = 0;
        int worst = -1;
        double worst_weight = 0;
        for(x=0;x<d->num_columns;x++) {
            double w = p->weights[0][x]*sign;
            if(!p->disabled[x] && w < 0) {
                if(w < worst_weight) {
                    worst_weight = w;
                    worst = x;
                }
            }
        }
        if(worst>=0) {
            p->disabled[worst] = true;
            drop++;
            num_disabled++;
        }
    } while(drop && num_disabled < d->num_rows);
}

node_t* perceptron_get_code(dataset_t*d, perceptron_t*p)
{
    int i;
    START_CODE(program)
    if(d->desired_response->num_classes == 2) {
        /* two classes */
        int class0 = 0;
        int class1 = 1;
        BLOCK
            IF
                GT
                    ADD
                        for(i=0;i<d->num_columns;i++) {
                            if(p->disabled[i])
                                continue;
                            MUL
                                PARAM(i);
                                FLOAT_CONSTANT(p->weights[class0][i])
                            END;
                        }
                    END;
                    FLOAT_CONSTANT(-p->weights[class0][p->intercept]);
                END;
            THEN
                GENERIC_CONSTANT(d->desired_response->classes[class0]);
            ELSE
                GENERIC_CONSTANT(d->desired_response->classes[class1]);
            END;
        END;
    } else {
        /* generic case, any number of classes */
        BLOCK
            int c;
            ARRAY_AT_POS
                ARRAY_CONSTANT(dataset_classes_as_array(d));
                ARG_MAX_F;
                    for(c=0;c<d->desired_response->num_classes;c++) {
                        ADD
                            for(i=0;i<d->num_columns;i++) {
                                if(p->disabled[i])
                                    continue;
                                MUL
                                    PARAM(i);
                                    FLOAT_CONSTANT(p->weights[c][i])
                                END;
                            }
                            FLOAT_CONSTANT(p->weights[c][p->intercept]);
                        END;
                    }
                END;
            END;
        END;
    }
    END_CODE;
    return program;
}

static void perceptron_destroy(perceptron_t*p)
{
    if(p->weights) {
        free(p->weights);
        p->weights = 0;
    }
    free(p);
}

static node_t*perceptron_train_model(perceptron_model_factory_t*factory, dataset_t*d)
{
    d = expand_text_columns(d);
    d = expand_categorical_columns(d);
    d = find_clear_cut_columns(d);

    perceptron_t*p = perceptron_new(d);
    if(factory->halfside) {
        perceptron_train_halfsided(p, d, factory->halfside);
    } else {
        perceptron_train(p, d);
    }
    node_t*program = perceptron_get_code(d, p);
    perceptron_destroy(p);

    d = dataset_revert_one_transformation(d, &program);
    d = dataset_revert_one_transformation(d, &program);
    d = dataset_revert_one_transformation(d, &program);
    return program;
}

static perceptron_model_factory_t perceptron_model_factory = {
    head: {
        name: "perceptron",
        train: (training_function_t)perceptron_train_model,
    },
    halfside: 0,
};

static perceptron_model_factory_t positive_perceptron_model_factory = {
    head: {
        name: "positive halfperceptron",
        train: (training_function_t)perceptron_train_model,
    },
    halfside: 1,
};

static perceptron_model_factory_t negative_perceptron_model_factory = {
    head: {
        name: "negative halfperceptron",
        train: (training_function_t)perceptron_train_model,
    },
    halfside: -1,
};

model_factory_t* perceptron_models[] =
{
    (model_factory_t*)&perceptron_model_factory,
    (model_factory_t*)&positive_perceptron_model_factory,
    (model_factory_t*)&negative_perceptron_model_factory,
};
int num_perceptron_models = sizeof(perceptron_models) / sizeof(perceptron_models[0]);
