/* model_cv_ann.cpp
   Artificial Neuronal Network (ANN) model

   Part of the data prediction package.
   
   Copyright (c) 2011 Matthias Kramm <kramm@quiss.org> 
 
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
#include "transform.h"

//#define VERIFY 1

typedef struct _ann_model_factory {
    model_factory_t head;
    int activation_function;
    int num_layers;
} ann_model_factory_t;

class CodeGeneratingANN: public CvANN_MLP
{
    public:
    CodeGeneratingANN(dataset_t*dataset, 
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
        delete[] var_offset;
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
        BLOCK
	int l_count = layer_sizes->cols;

	const double* w = weights[0];
	int j;
        int pos = 0;
        for(j=0;j<input_size;j++) {
	    SETLOCAL(var_offset[0]+j)
		ADD
		    MUL
                        PARAM(j);
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
	    int o = var_offset[j];
	    w = weights[j];
	    for(x=0;x<w_cols;x++) {
		SETLOCAL(o+x)
		    ADD
		    for(y=0;y<w_rows;y++) {
			MUL
			    GETLOCAL(var_offset[j-1]+y);
			    FLOAT_CONSTANT(w[w_cols*y+x]);
			END;
		    }
		    END;
		END;
	    }
	    const double*bias = w + w_rows*w_cols;
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
			SETLOCAL(o+x)
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
                        END;
			break;
		    }
		}
	    }
	}

        int final = var_offset[l_count-1];
	w = weights[l_count];
	for(j=0;j<output_size;j++) {
	    SETLOCAL(final+j)
		ADD
		    MUL
			GETLOCAL(final+j);
			FLOAT_CONSTANT(w[j*2]);
		    END;
		    FLOAT_CONSTANT(w[j*2+1]);
		END;
	    END;
	}

        ARRAY_AT_POS
            ARRAY_CONSTANT(dataset_classes_as_array(dataset));
            ARG_MAX_F
                for(j=0;j<output_size;j++) {
                    GETLOCAL(final+j);
                }
            END;
        END;

        END_BLOCK;
	END_CODE;
	return program;
    }

    constant_t predict(row_t*row, bool debug) const
    {
        CvMat* matrix_row = cvmat_from_row(dataset, row, false);
        CvMat* output = cvCreateMat(1, dataset->desired_response->num_classes, CV_32FC1);
        if(debug)
            cvmat_print(matrix_row);
        CvANN_MLP::predict(matrix_row, output);
        int index = cvmat_get_max_index(output);
        cvReleaseMat(&matrix_row);
        cvReleaseMat(&output);
        return dataset_map_response_class(dataset, index);
    }

    dataset_t*dataset;
    int*var_offset;
    int input_size;
    int output_size;
};

#ifdef VERIFY
void verify(dataset_t*dataset, model_t*m, CodeGeneratingANN*ann)
{
    example_t*e = dataset->first_example;
    int t;
    while(e) {
        row_t* r = example_to_row(e, m->column_names);
        constant_t p = ann->predict(r, false);
        variable_t c1 = constant_to_variable(&p);
        variable_t c2 = model_predict(m, r);

        variable_print(&c1, stdout);
        printf(" <-> ");
        variable_print(&c2, stdout);
        printf("\n");

        if(!variable_equals(&c1, &c2)) {
            ann->predict(r, true);
        }
        assert(variable_equals(&c1, &c2));
        row_destroy(r);
        e = e->next;
    }
}
#endif

static node_t*ann_train(ann_model_factory_t*factory, dataset_t*d)
{
    d = expand_categorical_columns(d);

    assert(!dataset_has_categorical_columns(d));
    
    int num_layers = factory->num_layers;

    CvMat* layers = cvCreateMat( 1, num_layers, CV_32SC1);
    int input_width = count_multiclass_columns(d);
    int output_width = d->desired_response->num_classes;
    int t;
    for(t=0;t<num_layers;t++) {
        int size = (input_width+output_width)/2;
        if(t==0) {
            size = input_width;
        } else if(t==num_layers-1) {
            size = output_width;
        } else {
            if(size<=1)
                size = 2;
        }
        cvmSetI(layers, 0, t, size);
    }

    int num_rows = training_set_size(d->num_rows);

    CvANN_MLP_TrainParams ann_params;
    CodeGeneratingANN ann(d, input_width, output_width, layers, factory->activation_function, 0.0, 0.0);
    CvMat* ann_input;
    CvMat* ann_response;
    make_ml_multicolumn(d, &ann_input, &ann_response, num_rows, true);
    ann.train(ann_input, ann_response, NULL, NULL, ann_params, 0x0000);

    node_t*code = ann.get_program();
    d = dataset_revert_one_transformation(d, &code);

#ifdef VERIFY
    verify(dataset, m, &ann);
#endif

    cvReleaseMat(&layers);
    cvReleaseMat(&ann_input);
    cvReleaseMat(&ann_response);
    return code;
}

static ann_model_factory_t ann_2sigmoid_model_factory = {
    head: {
        name: "neuronal network (sigmoid) with 2 layers",
        train: (training_function_t)ann_train,
    },
    activation_function: CvANN_MLP::SIGMOID_SYM,
    num_layers: 2,
};

static ann_model_factory_t ann_2gaussian_model_factory = {
    head: {
        name: "neuronal network (gaussian) with 2 layers",
        train: (training_function_t)ann_train,
    },
    activation_function: CvANN_MLP::GAUSSIAN,
    num_layers: 2,
};

static ann_model_factory_t ann_2identity_model_factory = {
    head: {
        name: "neuronal network (id) with 2 layers",
        train: (training_function_t)ann_train,
    },
    activation_function: CvANN_MLP::IDENTITY,
    num_layers: 2,
};

static ann_model_factory_t ann_3sigmoid_model_factory = {
    head: {
        name: "neuronal network (sigmoid) with 3 layers",
        train: (training_function_t)ann_train,
    },
    activation_function: CvANN_MLP::SIGMOID_SYM,
    num_layers: 3,
};

static ann_model_factory_t ann_3gaussian_model_factory = {
    head: {
        name: "neuronal network (gaussian) with 3 layers",
        train: (training_function_t)ann_train,
    },
    activation_function: CvANN_MLP::GAUSSIAN,
    num_layers: 3,
};

static ann_model_factory_t ann_3identity_model_factory = {
    head: {
        name: "neuronal network (id) with 3 layers",
        train: (training_function_t)ann_train,
    },
    activation_function: CvANN_MLP::IDENTITY,
    num_layers: 3,
};

model_factory_t* ann_models[] =
{
    (model_factory_t*)&ann_2sigmoid_model_factory,
    (model_factory_t*)&ann_2gaussian_model_factory,
    (model_factory_t*)&ann_2identity_model_factory,
    (model_factory_t*)&ann_3sigmoid_model_factory,
    (model_factory_t*)&ann_3gaussian_model_factory,
    (model_factory_t*)&ann_3identity_model_factory,
};
int num_ann_models = sizeof(ann_models) / sizeof(ann_models[0]);
