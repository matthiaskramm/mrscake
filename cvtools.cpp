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
        if(dataset->columns[j]->is_categorical) {
            this->change_var_type(j, CV_VAR_CATEGORICAL);
        }
    }

    for(i=0;i<dataset->num_rows;i++) {
        float* ddata = values->data.fl + total_columns*i;
        for(j=0;j<input_columns;j++) {
            column_t*c = dataset->columns[j];
            ddata[j] = c->is_categorical?
                    c->entries[i].c :
                    c->entries[i].f;
        }
        ddata[response_idx] = dataset->desired_response->entries[i].c;
    }

    train_sample_count = dataset->num_rows;
}

CvMLDataFromExamples::~CvMLDataFromExamples()
{
}

int cvmat_get_max_index(CvMat*mat)
{
    assert(CV_MAT_TYPE(mat->type) == CV_32FC1);
    int elements = mat->cols * mat->rows;
    float max = mat->data.fl[0];
    int c = 0;
    int t;
    for(t=1;t<elements;t++) {
        if(mat->data.fl[t] > max) {
            max = mat->data.fl[t];
            c = t;
        }
    }
    return c;
}
void cvmat_print(CvMat*mat)
{
    assert(CV_MAT_TYPE(mat->type) == CV_32FC1);
    int x,y;
    float*p = mat->data.fl;
    for(y=0;y<mat->rows;y++) {
        for(x=0;x<mat->cols;x++) {
            printf("%f\n", *p++);
        }
        printf("\n");
    }
}

CvMat*cvmat_from_row(sanitized_dataset_t*dataset, row_t*row, bool expand_categories, bool add_one)
{
    int width = !expand_categories ? dataset->num_columns : dataset->expanded_num_columns;
    if(add_one)
        width++;

    CvMat* matrix_row = cvCreateMat(1, width, CV_32FC1);
    
    if(!expand_categories) {
        int t;
        for(t=0;t<row->num_inputs;t++) {
            matrix_row->data.fl[t] = variable_value(&row->inputs[t]);
        }
    } else {
        int t;
        int pos = 0;
        for(t=0;t<row->num_inputs;t++) {
            variable_t*v = &row->inputs[t];
            if(!dataset->columns[t]->is_categorical) {
                matrix_row->data.fl[pos++] = variable_value(v);
            } else {
                constant_t c = variable_to_constant(v);
                int s;
                for(s=0;s<dataset->columns[t]->num_classes;s++) {
                    if(constant_equals(&c, &dataset->columns[t]->classes[s])) {
                        matrix_row->data.fl[pos] = 1.0;
                    } else {
                        matrix_row->data.fl[pos] = 0.0;
                    }
                    pos++;
                }
            }
        }
        assert(pos == dataset->expanded_num_columns);
    }

    if(add_one)
        matrix_row->data.fl[row->num_inputs] = 0;
    return matrix_row;
}


