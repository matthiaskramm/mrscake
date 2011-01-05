#include "cvtools.h"
#include "dataset.h"

CvMLDataFromExamples::CvMLDataFromExamples(dataset_t*dataset)
    :CvMLData()
{
    example_t**examples = example_list_to_array(dataset);
    int input_columns = examples[0]->num_inputs;
    int response_idx = input_columns;
    int total_columns = input_columns+1;
   
    /* train on half the examples */
    int train_sample_count = (dataset->num_examples+1)>>1;
    
    this->values = cvCreateMat(dataset->num_examples, total_columns, CV_32FC1);
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

    for(i=0;i<dataset->num_examples;i++) {
        float* ddata = values->data.fl + total_columns*i;
        for(j=0;j<input_columns;j++) {
            variable_t*v = &examples[i]->inputs[j];
            
            ddata[j] = v->type == CATEGORICAL ?
                    v->category :
                    v->value;
        }
        ddata[response_idx] = examples[i]->desired_output;
    }

    train_sample_count = dataset->num_examples;
}
    
CvMLDataFromExamples::~CvMLDataFromExamples()
{
}

CvMat*cvmat_from_row(row_t*row, bool add_one)
{
    CvMat* matrix_row = cvCreateMat(1, row->num_inputs+(add_one?1:0), CV_32FC1);
    int t;
    for(t=0;t<row->num_inputs;t++) {
        matrix_row->data.fl[t] = variable_value(&row->inputs[t]);
    }
    if(add_one)
        matrix_row->data.fl[row->num_inputs] = 0;
    return matrix_row;
}


