/* model_cv_svm.cpp
   Support Vector Machine (SVM) model

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

#include "cvtools.h"
#include "mrscake.h"
#include "dataset.h"
#include "transform.h"
#include "easy_ast.h"
#include "model_select.h"

//#define VERIFY 1

typedef struct _svm_model_factory {
    model_factory_t head;
    int kernel;
} svm_model_factory_t;

class CodeGeneratingLinearSVM: public CvSVM
{
    public:
    CodeGeneratingLinearSVM(dataset_t*dataset)
        :CvSVM()
    {
        this->dataset = dataset;
    }
    ~CodeGeneratingLinearSVM()
    {
    }

    node_t* compare_classes(int _i, int _j) const
    {
        CvSVMDecisionFunc* df = (CvSVMDecisionFunc*)decision_func;

        /* Find decision function in array. This could be optimized. */
        int class_count = class_labels->cols;
        int i;
        for(i=0; i<class_count; i++) {
            int j;
            for(j=i+1; j<class_count; j++) {
                if(i==_i && j==_j)
                    break;
                df++;
            }
            if(j<class_count)
                break;
            assert(i<class_count-1);
        }

        int var_count = get_var_count();
        START_CODE(code)
        ADD
            FLOAT_CONSTANT(-df->rho);
            int sv_count = df->sv_count;
            int v;
            for(v=0;v<var_count;v++) {
                double sum = 0.0;
                int k;
                for(k = 0; k < sv_count; k++) {
                    sum += sv[df->sv_index[k]][v]*df->alpha[k];
                }
                MUL
                    PARAM(v);
                    FLOAT_CONSTANT(sum)
                END;
            }
        END;
        END_CODE;
        return(code);
    }

    /* Does the class count of the desired_response column match
       the number of distinct entries actually present? */
    bool class_counts_match()
    {
        int class_count = this->dataset->desired_response->num_classes;
        return class_labels->cols == class_count;
    }

    node_t* get_program() const
    {
        assert(kernel);
        assert(params.svm_type == CvSVM::C_SVC);
        assert(params.kernel_type == CvSVM::LINEAR);

        int var_count = get_var_count();
        int class_count = this->dataset->desired_response->num_classes;

        assert(class_labels->cols == class_count);

        START_CODE(program)
        BLOCK

        if(class_count == 2) {
            IF
                GT
                    INSERT_NODE(compare_classes(0,1));
                    FLOAT_CONSTANT(0.0);
                END;
            THEN
                GENERIC_CONSTANT(dataset->desired_response->classes[0]);
            ELSE
                GENERIC_CONSTANT(dataset->desired_response->classes[1]);
            END;
        } else {
            int vote_offset = 0;
            int i;
            for(i = 0; i < class_count; i++) {
                SETLOCAL(vote_offset+i)
                    INT_CONSTANT(0);
                END;
            }

            for(i=0; i<class_count; i++) {
                int j;
                for(j=i+1; j<class_count; j++) {
                    IF
                        GT
                            INSERT_NODE(compare_classes(i,j));
                            FLOAT_CONSTANT(0.0);
                        END;
                    THEN
                        INCLOCAL(vote_offset + i)
                    ELSE
                        INCLOCAL(vote_offset + j)
                    END;
                }
            }

            ARRAY_AT_POS
                ARRAY_CONSTANT(dataset_classes_as_array(dataset));
                ARG_MAX_I
                    int j;
                    for(j=0; j<class_count; j++) {
                        GETLOCAL(vote_offset+j);
                    }
                END;
            END;
        }

        END;
        END_CODE;

        return program;
    }
    dataset_t*dataset;
};

static node_t*svm_train(svm_model_factory_t*factory, dataset_t*d)
{
    d = expand_text_columns(d);
    d = expand_categorical_columns(d);
   
    assert(!dataset_has_categorical_columns(d));

    int num_rows = training_set_size(d->num_rows);

    CodeGeneratingLinearSVM svm(d);
    CvSVMParams params = CvSVMParams(CvSVM::C_SVC, CvSVM::LINEAR,
                                     /*degree*/0, /*gamma*/1, /*coef0*/0, /*C*/1,
                                     /*nu*/0, /*p*/0, /*class_weights*/0,
                                     cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
    CvMat* input;
    CvMat* response;
    make_ml_multicolumn(d, &input, &response, num_rows, false);

    if(svm.train_auto(input, response, 0, 0, params, 5)) {
        // ok
    } else if(svm.train(input, response, 0, 0, params)) {
        // ok
    } else {
        return NULL;
    }
    /* check whether we were operating on a sane subset of the data */
    if(!svm.class_counts_match())
        return NULL;

    node_t*code = svm.get_program();

    cvReleaseMat(&input);
    cvReleaseMat(&response);
    
    d = dataset_revert_one_transformation(d, &code);
    d = dataset_revert_one_transformation(d, &code);
    return code;
}

static svm_model_factory_t simplified_linear_svm_model_factory = {
    head: {
        name: "simplified linear svm",
        train: (training_function_t)svm_train,
    },
    CvSVM::LINEAR
};


model_factory_t* linear_models[] =
{
    (model_factory_t*)&simplified_linear_svm_model_factory,
};
int num_linear_models = sizeof(linear_models) / sizeof(linear_models[0]);
