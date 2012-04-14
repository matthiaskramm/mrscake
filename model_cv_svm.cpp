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

class CodeGeneratingSVM: public CvSVM
{
    public:
    CodeGeneratingSVM(dataset_t*dataset)
        :CvSVM()
    {
        this->dataset = dataset;
    }
    ~CodeGeneratingSVM()
    {
    }

    node_t* get_program() const
    {
        assert( kernel );
        assert( params.svm_type == CvSVM::C_SVC );

        int var_count = get_var_count();
        int class_count = this->dataset->desired_response->num_classes;

        assert(class_labels->cols == class_count);

        START_CODE(program)
        BLOCK

        /* FIXME: we should evaluate parameter_code(i) only once for each i */
        if(params.kernel_type == CvSVM::RBF) {
            //calc_rbf(vcount, var_count, vecs, another, results);
            double gamma = -params.gamma;
            int j, k;
            for(j=0;j<sv_total;j++) {
                float*vec = sv[j];
                SETLOCAL(j)
                    EXP
                        MUL
                            ADD
                                for(k=0; k<var_count; k++) {
                                    SQR
                                        SUB
                                            FLOAT_CONSTANT(vec[k]);
                                            PARAM(k);
                                        END;
                                    END;
                                }
                            END;
                            FLOAT_CONSTANT(gamma);
                        END;
                    END;
                END;
            }
        } else if(params.kernel_type == CvSVM::POLY) {
            //calc_poly(vcount, var_count, vecs, another, results);
            assert(!"polynomial kernel not supported yet");
        } else if(params.kernel_type == CvSVM::SIGMOID) {
            //calc_sigmoid(vcount, var_count, vecs, another, results);
            int j;
            for(j=0;j<sv_total;j++) {
                float*vec = sv[j];
                double mul = -2*params.gamma; //"alpha"
                double add = -2*params.coef0; //"beta"
                SETLOCAL(sv_total+j)
                    ADD
                        int k;
                        for(k=0;k<var_count;k++) {
                            MUL
                                PARAM(k);
                                FLOAT_CONSTANT(vec[k]*mul)
                            END;
                        }
                        FLOAT_CONSTANT(add);
                    END;
                END;
                SETLOCAL(j)
                    EXP
                        NEG
                            ABS
                                GETLOCAL(sv_total+j)
                            END;
                        END;
                    END;
                END;
                SETLOCAL(j)
                    DIV
                        SUB
                            FLOAT_CONSTANT(1.0);
                            GETLOCAL(j);
                        END;
                        ADD
                            FLOAT_CONSTANT(1.0);
                            GETLOCAL(j);
                        END;
                    END;
                END;
                IF
                    LTE
                        GETLOCAL(sv_total+j)
                        FLOAT_CONSTANT(0.0)
                    END;
                THEN
                    SETLOCAL(j)
                        NEG
                            GETLOCAL(j);
                        END;
                    END;
                ELSE
                    NOP;
                END;
            }
        } else if(params.kernel_type == CvSVM::LINEAR) {
            //calc_non_rbf_base(vcount, var_count, vecs, another, results, 1, 0);
            int j;
            for(j=0;j<sv_total;j++) {
                float*vec = sv[j];
                SETLOCAL(j)
                    ADD
                        int k;
                        for(k=0;k<var_count;k++) {
                            MUL
                                PARAM(k);
                                FLOAT_CONSTANT(vec[k])
                            END;
                        }
                    END;
                END;
            }
        } else assert(!"invalid kernel type");

        int vote_offset = sv_total;
        int i;
        for(i = 0; i < class_count; i++) {
            SETLOCAL(vote_offset+i)
                INT_CONSTANT(0);
            END;
        }

        CvSVMDecisionFunc* df = (CvSVMDecisionFunc*)decision_func;
        for(i=0; i<class_count; i++) {
            int j;
            for(j=i+1; j<class_count; j++) {
                IF
                    GT
                        ADD
                            FLOAT_CONSTANT(-df->rho);
                            int sv_count = df->sv_count;
                            int k;
                            for(k = 0; k < sv_count; k++) {
                                MUL
                                    FLOAT_CONSTANT(df->alpha[k])
                                    GETLOCAL(df->sv_index[k]);
                                END;
                            }
                        END;
                        FLOAT_CONSTANT(0.0);
                    END;
                THEN
                    INCLOCAL(vote_offset + i)
                ELSE
                    INCLOCAL(vote_offset + j)
                END;
                df++;
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

        END;
        END_CODE;
        return program;
    }
    dataset_t*dataset;
};

static node_t*svm_train(svm_model_factory_t*factory, dataset_t*d)
{
    d = remove_text_columns(d);
    d = expand_categorical_columns(d);
    
    assert(!dataset_has_categorical_columns(d));

    if(factory->kernel == CvSVM::LINEAR && d->desired_response->num_classes > 4 ||
       factory->kernel == CvSVM::RBF    && d->desired_response->num_classes > 3) {
        /* if we have too many classes one-vs-one SVM classification is too slow */
        return 0;
    }

    int num_rows = training_set_size(d->num_rows);

    if(factory->kernel == CvSVM::LINEAR && d->num_rows > 1000) {
        num_rows = 1000;
    }
    if(factory->kernel == CvSVM::RBF && d->num_rows > 300) {
        num_rows = 300;
    }
    if(factory->kernel == CvSVM::SIGMOID && d->num_rows > 200) {
        num_rows = 200;
    }

    CodeGeneratingSVM svm(d);
    CvSVMParams params = CvSVMParams(CvSVM::C_SVC, factory->kernel,
                                     /*degree*/0, /*gamma*/1, /*coef0*/0, /*C*/1,
                                     /*nu*/0, /*p*/0, /*class_weights*/0,
                                     cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
    CvMat* input;
    CvMat* response;
    make_ml_multicolumn(d, &input, &response, num_rows, false);

    bool use_auto_training = d->desired_response->num_classes <= 3;

    node_t*code = 0;
    if(use_auto_training && svm.train_auto(input, response, 0, 0, params, 5)) {
        code = svm.get_program();
    }
    if(!code && svm.train(input, response, 0, 0, params)) {
        code = svm.get_program();
    }
    cvReleaseMat(&input);
    cvReleaseMat(&response);
    
    d = dataset_revert_one_transformation(d, &code);
    d = dataset_revert_one_transformation(d, &code);
    return code;
}

static svm_model_factory_t rbf_svm_model_factory = {
    head: {
        name: "rbf svm",
        train: (training_function_t)svm_train,
    },
    CvSVM::RBF
};

static svm_model_factory_t sigmoid_svm_model_factory = {
    head: {
        name: "sigmoid svm",
        train: (training_function_t)svm_train,
    },
    CvSVM::SIGMOID
};

static svm_model_factory_t linear_svm_model_factory = {
    head: {
        name: "linear svm",
        train: (training_function_t)svm_train,
    },
    CvSVM::LINEAR
};

model_factory_t* svm_models[] =
{
    (model_factory_t*)&linear_svm_model_factory,
    (model_factory_t*)&sigmoid_svm_model_factory,
    (model_factory_t*)&rbf_svm_model_factory,
};
int num_svm_models = sizeof(svm_models) / sizeof(svm_models[0]);
