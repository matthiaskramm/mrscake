/* model_cv_knearest.cpp
   k Nearest Neighbors

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
#include "easy_ast.h"
#include "model_select.h"

typedef struct _knearest_model_factory {
    model_factory_t head;
} svm_model_factory_t;

class CodeGeneratingKNearest: public CvKNearest
{
    public:

    CodeGeneratingKNearest(dataset_t*dataset)
        :CvKNearest()
    {
        this->dataset = dataset;
    }

    ~CodeGeneratingKNearest()
    {
    }

    node_t* get_program() const
    {
        START_CODE(program)
        BLOCK
        END;
        END_CODE;

        return program;
    }
    dataset_t*dataset;
};

static node_t*knearest_train(svm_model_factory_t*factory, dataset_t*d)
{
    int num_rows = training_set_size(d->num_rows);
    
    assert(!dataset_has_categorical_columns(dataset_t*data));

    CodeGeneratingKNearest knearest(d);

    CvMat* input;
    CvMat* response;
    make_ml_multicolumn(d, &input, &response, num_rows, false);
    
    // ----------

    CvKNearest knearest;
    //bool CvKNearest::train( const Mat& _train_data, const Mat& _responses,
    //                const Mat& _sample_idx, bool _is_regression,
    //                int _max_k, bool _update_base )
    bool is_classifier = var_types->data.ptr[var_types->cols-1] == CV_VAR_CATEGORICAL;
    assert(is_classifier);
    int max_k = 10;
    knearest.train(values, response, train_sidx, is_regression, max_k, false);

    CvMat* new_response = cvCreateMat(response->rows, 1, values->type);
    //print_types();

    //const CvMat* train_sidx = data.get_train_sample_idx();
    knearest.find_nearest(values, max_k, new_response, 0, 0, 0);

    print_result(knearest_calc_error(values, response, new_response, train_sidx, is_regression, CV_TRAIN_ERROR),
                 knearest_calc_error(values, response, new_response, train_sidx, is_regression, CV_TEST_ERROR), 0);

    // ----------

    node_t* code = 0;
    if(svm.train_auto(input, response, 0, 0, params, 5)) {
        code = svm.get_program();
    } else if(svm.train(input, response, 0, 0, params)) {
        code = svm.get_program();
    }

    cvReleaseMat(&input);
    cvReleaseMat(&response);
    return code;
}

static svm_model_factory_t knearest_svm_model_factory = {
    head: {
        name: "k nearest neighbors",
        train: (training_function_t)knearest_train,
    },
};


model_factory_t* knearest_models[] =
{
    (model_factory_t*)&knearest_svm_model_factory,
};
int num_knearest_models = sizeof(knearest_models) / sizeof(knearest_models[0]);
