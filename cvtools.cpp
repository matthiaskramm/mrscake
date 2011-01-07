#include "cvtools.h"
#include "dataset.h"

void cvmSetI(CvMat*m, int y, int x, int v)
{
    int*e = (int*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = v;
}
void cvmSetF(CvMat*m, int y, int x, float f)
{
    float*e = (float*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = f;
}
int cvmGetI(const CvMat*m, int y, int x)
{
    return CV_MAT_ELEM((*m), int, y, x);
}
float cvmGetF(const CvMat*m, int y, int x)
{
    return CV_MAT_ELEM((*m), float, y, x);
}

CvMLDataFromExamples::CvMLDataFromExamples(sanitized_dataset_t*dataset)
    :CvMLData()
{
    int input_columns = dataset->num_columns;
    int response_idx = input_columns;
    int total_columns = input_columns+1;

    /* train on half the examples */
    int train_sample_count = (dataset->num_rows+1)>>1;

    this->values = cvCreateMat(dataset->num_rows, total_columns, CV_32FC1);
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
        if(dataset->columns[j]->type == CATEGORICAL ||
           dataset->columns[j]->type == TEXT) {
            this->change_var_type(j, CV_VAR_CATEGORICAL);
        }
    }

    for(i=0;i<dataset->num_rows;i++) {
        float* ddata = values->data.fl + total_columns*i;
        for(j=0;j<input_columns;j++) {
            column_t*c = dataset->columns[j];
            ddata[j] = c->type == CONTINUOUS?
                    c->entries[i].f :
                    c->entries[i].c;
        }
        ddata[response_idx] = dataset->desired_response->entries[i].c;
    }

    train_sample_count = dataset->num_rows;
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


