#include "cvtools.h"

CvMLDataFromExamples::CvMLDataFromExamples(example_t**examples, int num_examples)
    :CvMLData()
{
    int input_columns = examples[0]->num_inputs;
    int response_idx = input_columns;
    int train_sample_count = (num_examples+1)>>1;
    int total_columns = input_columns+1;
    
    this->values = cvCreateMat(num_examples, total_columns, CV_32FC1);
    cvZero(this->values);
    this->var_idx_mask = cvCreateMat( 1, total_columns, CV_8UC1);
    cvSet(var_idx_mask, cvRealScalar(1), 0);
    this->var_types = cvCreateMat( 1, total_columns, CV_8U );
    cvZero(this->var_types);

    this->set_response_idx(response_idx);
    this->change_var_type(response_idx, CV_VAR_CATEGORICAL);
    CvTrainTestSplit spl(train_sample_count);
    this->set_train_test_split(&spl);

    int i,j;
    
    for(j=0;j<input_columns;j++) {
        if(examples[0]->inputs[j].type == CATEGORICAL) {
            this->change_var_type(j, CV_VAR_CATEGORICAL);
        }
    }

    for(i=0;i<num_examples;i++) {
        float* ddata = values->data.fl + total_columns*i;
        for(j=0;j<input_columns;j++) {
            variable_t*v = &examples[i]->inputs[j];
            
            ddata[j] = v->type == CATEGORICAL ?
                    v->category :
                    v->value;
        }
        ddata[response_idx] = examples[i]->desired_output;
    }

    train_sample_count = num_examples;
}
    
CvMLDataFromExamples::~CvMLDataFromExamples()
{
}

