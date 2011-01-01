#include "cvtools.h"

CvMLDataFromExamples::CvMLDataFromExamples(example_t*examples, int num_examples)
{
    int input_columns = examples[0].num_inputs;
    int response_idx = input_columns+1;
    int train_sample_count = (num_examples+1)>>1;
    values = cvCreateMat(num_examples, response_idx, CV_32FC1);
    var_idx_mask = cvCreateMat( 1, input_columns+1, CV_8UC1 );
    cvSet(var_idx_mask, cvRealScalar(1), 0);
    this->set_response_idx(response_idx);
    this->change_var_type(response_idx, CV_VAR_CATEGORICAL);
    CvTrainTestSplit spl(train_sample_count);
    this->set_train_test_split(&spl);

    int i,j;
    for(j=0;j<input_columns;j++) {
        if(examples[j].inputs[j].type == CATEGORICAL) {
            this->change_var_type(j, CV_VAR_CATEGORICAL);
        }
    }

    int cols_count = input_columns+1;
    for(i=0;i<num_examples;i++) {
        float* ddata = values->data.fl + cols_count*i;
        for(j=0;j<input_columns;j++) {
            variable_t*v = &examples[i].inputs[j];
            ddata[j] =
                v->type == CATEGORICAL ?
                    v->category :
                    v->value;
        }
        ddata[input_columns] = examples[i].desired_output;
    }

    train_sample_count = num_examples;
}

