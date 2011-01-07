#include "cvtools.h"
#include "model.h"
#include "dataset.h"
#include "ast.h"

//#define VERIFY 1

class CodeGeneratingANN: public CvANN_MLP
{
    public:
    CodeGeneratingANN(sanitized_dataset_t*dataset, const CvMat* layer_sizes,
                      int activ_func, double f_param1, double f_param2)
        :CvANN_MLP(layer_sizes, activ_func, f_param1, f_param2)
    {
        this->dataset = dataset;
    }
#ifdef VERIFY
    category_t predict(row_t*row)
    {
        CvMat* matrix_row = cvmat_from_row(row, 1);
        category_t c = (int)floor(CvDTree::predict(matrix_row, NULL, false)->value + FLT_EPSILON);
        cvReleaseMat(&matrix_row);
        return c;
    }
#endif
    node_t* get_program() const
    {
        //START_CODE(code);
        //walk(root,code);
        //END_CODE;
        return NULL;
    }
    sanitized_dataset_t*dataset;
};

static int set_column_in_matrix(column_t*column, CvMat*mat, int xpos, int rows)
{
    int y;
    int x = 0;
    if(column->type == CONTINUOUS) {
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

static int count_multiclass_columns(sanitized_dataset_t*d)
{
    int x;
    int width = 0;
    for(x=0;x<d->num_columns;x++) {
        if(d->columns[x]->type != CONTINUOUS) {
            width += d->columns[x]->num_classes;
        } else {
            width++;
        }
    }
    return width;
}

void make_ml_multicolumn(sanitized_dataset_t*d, CvMat**in, CvMat**out)
{
    int x,y;
    int width = count_multiclass_columns(d);
    *in = cvCreateMat(d->num_rows, width, CV_32SC1);
    *out = cvCreateMat(d->num_rows, d->desired_response->num_classes, CV_32SC1);
    int xpos = 0;
    for(x=0;x<d->num_columns;x++) {
        xpos += set_column_in_matrix(d->columns[x], *in, xpos, d->num_rows);
    }
    assert(xpos == width);
    set_column_in_matrix(d->desired_response, *out, 0, d->num_rows);
}

static model_t*ann_train(dataset_t*dataset)
{
    sanitized_dataset_t*d = dataset_sanitize(dataset);

    int num_layers = 3;
    CvMat* layers = cvCreateMat( 1, num_layers, CV_32SC1);
    cvmSetI(layers, 0, 0, d->num_columns);
    cvmSetI(layers, 0, 1, d->desired_response->num_classes);
    cvmSetI(layers, 0, 2, d->desired_response->num_classes);
    CvANN_MLP_TrainParams ann_params;
    CodeGeneratingANN ann(d, layers, CvANN_MLP::SIGMOID_SYM, 0.0, 0.0);
    CvMLDataFromExamples data(d);
    CvMat* ann_input;
    CvMat* ann_response;
    make_ml_multicolumn(d, &ann_input, &ann_response);
    ann.train(ann_input, ann_response, NULL, NULL, ann_params, 0x0000);

    model_t*m = (model_t*)malloc(sizeof(model_t));
    m->code = ann.get_program();
    m->wordmap = d->wordmap;d->wordmap=0;

    sanitized_dataset_destroy(d);
    return m;
}

model_factory_t ann_model_factory = {
    name: "neuronal network",
    train: ann_train,
    internal: 0,
};

