#include "cvtools.h"
#include "model.h"
#include "dataset.h"
#include "ast.h"
#include "model_select.h"

//#define VERIFY 1

class CodeGeneratingANN: public CvANN_MLP
{
    public:
    CodeGeneratingANN(sanitized_dataset_t*dataset, 
	              int input_size, 
		      int output_size, const CvMat* layer_sizes,
                      int activ_func, double f_param1, double f_param2)
        :CvANN_MLP(layer_sizes, activ_func, f_param1, f_param2)
    {
        this->dataset = dataset;
	this->input_size = input_size;
	this->output_size = output_size;
	var_offset = new int[layer_sizes->cols+1];
	int t;
	int o = 0;
	for(t=0;t<layer_sizes->cols;t++) {
	    var_offset[t] = o;
	    o += layer_sizes->data.i[t];
	}
	var_offset[t] = o;
    }
    ~CodeGeneratingANN() 
    {
	delete var_offset;
    }

    void scale_input()
    {
	const double* w = weights[0];
    }

    /*
       def predict(f,cls):
	   # initialize classes
           a = f
	   b = int(cls=="A")
	   c = int(cls=="B")

	   # input scaling
           a = a*0.3+0.5	
           b = b*0.3+0.5	
           c = c*0.3+0.5
	   # layer0 -> layer1
	   l0 = a*0.9 + b*0.2 + c*0.5
	   l1 = a*0.1 + b*0.3 + c*0.1
	   l2 = a*0.2 + b*0.6 + c*0.9
	   l0 = exp(l0*0.7 + 0.9)
	   l0 = (1-l0)/(1+l0)*0.1
	   l1 = exp(l0*0.7 + 0.9)
	   l1 = (1-l0)/(1+l0)*0.1
	   l2 = exp(l0*0.7 + 0.9)
	   l2 = (1-l0)/(1+l0)*0.1
	   # layer1 -> layer2
	   l3 = l0*0.9 + l1*0.2 + l2*0.5
	   l4 = l0*0.1 + l1*0.3 + l2*0.1
	   l5 = l0*0.2 + l1*0.6 + l2*0.9
	    ...
	   # output scaling
	   r0 = l6*0.3 + 0.7
	   r1 = l7*0.1 + 0.2
	   r2 = l8*0.9 + 0.5

	   return ["A","B","C"][[r0,r1,r2].index(max([r0,r1,r2]))]
     */

    node_t* get_program() const
    {
        START_CODE(program);
	int l_count = layer_sizes->cols;

	const double* w = weights[0];
	int j;
	for(j=0;j<input_size;j++) {
	    SETLOCAL(var_offset[0]+j)
		ADD
		    MUL
			VAR(j)
			FLOAT_CONSTANT(w[j*2])
		    END;
		    FLOAT_CONSTANT(w[j*2+1])
		END;
	    END;
	}

	int cols = input_size;
	for( j = 1; j < l_count; j++ )
	{
	    int layer_in_cols = cols;
	    cols = layer_sizes->data.i[j];
	    int layer_out_cols = cols;

	    int w_rows = layer_in_cols;
	    int w_cols = layer_out_cols;
	    int x,y;
	    w = weights[j];
	    for(x=0;x<w_cols;x++) {
		SETLOCAL(var_offset[j+1]+x)
		    ADD
		    for(y=0;y<w_rows;y++) {
			MUL
			    GETLOCAL(var_offset[j]+y);
			    FLOAT_CONSTANT(w[w_cols*y+x]);
			END;
		    }
		    END;
		END;
	    }
	    const double*bias = w + w_rows*w_cols;
	    int o = var_offset[j+1];
	    for(x=0;x<w_cols;x++) {
		double scale2 = f_param2;
		switch( activ_func )
		{
		    case IDENTITY: {
			SETLOCAL(o+x)
			    ADD
				GETLOCAL(o+x)
				FLOAT_CONSTANT(bias[x]);
			    END;
			END;
			break;
		    }
		    case SIGMOID_SYM: {
			double scale = -f_param1;
			SETLOCAL(o+x)
			    EXP
				MUL
				    ADD
					GETLOCAL(o+x)
					FLOAT_CONSTANT(bias[x]);
				    END;
				    FLOAT_CONSTANT(scale);
				END;
			    END;
			END;
			SETLOCAL(o+x)
			    MUL
				DIV
				    SUB
					FLOAT_CONSTANT(1.0);
					GETLOCAL(o+x)
				    END;
				    ADD	
					FLOAT_CONSTANT(1.0);
					GETLOCAL(o+x)
				    END;
				END;
				FLOAT_CONSTANT(scale2);
			    END;
			END;
			break;
		    }
		    case GAUSSIAN: {
			double scale = -f_param1*f_param1;
			MUL
			    EXP
				MUL
				    SQR
					ADD 
					    GETLOCAL(o+x);
					    FLOAT_CONSTANT(bias[x]);
					END;
				    END;
				    FLOAT_CONSTANT(scale);
				END;
			    END;
			    FLOAT_CONSTANT(scale2);
			END;
			break;
		    }
		}
	    }
	}
    
	w = weights[l_count];
	for(j=0;j<output_size;j++) {
	    SETLOCAL(var_offset[l_count]+j)
		ADD
		    MUL
			GETLOCAL(var_offset[l_count]+j);
			FLOAT_CONSTANT(w[j*2]);
		    END;
		    FLOAT_CONSTANT(w[j*2+1]);
		END;
	    END;
	}
	END_CODE;
	return program;
    }

    sanitized_dataset_t*dataset;
    int*var_offset;
    int input_size;
    int output_size;
};

static int set_column_in_matrix(column_t*column, CvMat*mat, int xpos, int rows)
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

static int count_multiclass_columns(sanitized_dataset_t*d)
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

void make_ml_multicolumn(sanitized_dataset_t*d, CvMat**in, CvMat**out)
{
    int x,y;
    int width = count_multiclass_columns(d);
    *in = cvCreateMat(d->num_rows, width, CV_32FC1);
    *out = cvCreateMat(d->num_rows, d->desired_response->num_classes, CV_32FC1);
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
    int input_width = count_multiclass_columns(d);
    int output_width = d->desired_response->num_classes;
    cvmSetI(layers, 0, 0, input_width);
    cvmSetI(layers, 0, 1, output_width);
    cvmSetI(layers, 0, 2, output_width);
    CvANN_MLP_TrainParams ann_params;
    CodeGeneratingANN ann(d, input_width, output_width, layers, CvANN_MLP::SIGMOID_SYM, 0.0, 0.0);
    CvMLDataFromExamples data(d);
    CvMat* ann_input;
    CvMat* ann_response;
    make_ml_multicolumn(d, &ann_input, &ann_response);
    ann.train(ann_input, ann_response, NULL, NULL, ann_params, 0x0000);

    model_t*m = (model_t*)malloc(sizeof(model_t));
    m->code = ann.get_program();

    sanitized_dataset_destroy(d);
    return m;
}

model_factory_t ann_model_factory = {
    name: "neuronal network",
    train: ann_train,
    internal: 0,
};

