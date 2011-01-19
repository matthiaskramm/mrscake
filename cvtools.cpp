/* cvtools.cpp
   Utility functions for interfacing with OpenCV

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
#include "dataset.h"

void cvmSetI(CvMat*m, int y, int x, int v)
{
    int32_t*e = (int32_t*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = v;
}
void cvmSetF(CvMat*m, int y, int x, float f)
{
    float*e = (float*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = f;
}
int cvmGetI(const CvMat*m, int y, int x)
{
    return CV_MAT_ELEM((*m), int32_t, y, x);
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
    int width = dataset->num_columns;

    expanded_columns_t*e = 0;
    if(expand_categories) {
        e = expanded_columns_new(dataset);
        width = e->num;
    }

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
        assert(pos == e->num);
    }

    if(e) {
        expanded_columns_destroy(e);
    }

    if(add_one)
        matrix_row->data.fl[row->num_inputs] = 0;
    return matrix_row;
}

int set_column_in_matrix(column_t*column, CvMat*mat, int xpos, int rows)
{
    int y;
    int x = 0;
    if(!column->is_categorical) {
        for(y=0;y<rows;y++) {
            float*e = (float*)(CV_MAT_ELEM_PTR(*mat, y, xpos+x));
            *e =  column->entries[y].f;
        }
        x++;
    } else {
        int c = 0;
        for(c=0;c<column->num_classes;c++) {
            for(y=0;y<rows;y++) {
                float*e = (float*)(CV_MAT_ELEM_PTR(*mat, y, xpos+x));
                if(column->entries[y].c == c) {
                    *e = 1.0;
                } else {
                    *e = 0.0;
                }
            }
            x++;
        }
    }
    return x;
}

int count_multiclass_columns(sanitized_dataset_t*d)
{
    int x;
    int width = 0;
    for(x=0;x<d->num_columns;x++) {
        if(!d->columns[x]->is_categorical) {
            width++;
        } else {
            width += d->columns[x]->num_classes;
        }
    }
    return width;
}

void make_ml_multicolumn(sanitized_dataset_t*d, CvMat**in, CvMat**out, int num_rows, bool multicolumn_response)
{
    int x,y;
    int width = count_multiclass_columns(d);
    *in = cvCreateMat(num_rows, width, CV_32FC1);
    int xpos = 0;
    for(x=0;x<d->num_columns;x++) {
        xpos += set_column_in_matrix(d->columns[x], *in, xpos, num_rows);
    }
    assert(xpos == width);
    if(multicolumn_response) {
        *out = cvCreateMat(num_rows, d->desired_response->num_classes, CV_32FC1);
        set_column_in_matrix(d->desired_response, *out, 0, num_rows);
    } else {
        *out = cvCreateMat(num_rows, 1, CV_32SC1);
        int y;
        for(y=0;y<num_rows;y++) {
            int32_t*e = (int32_t*)(CV_MAT_ELEM_PTR(**out, y, 0));
            *e =  d->desired_response->entries[y].c;
        }
    }
}

