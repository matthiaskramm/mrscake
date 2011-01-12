#include "cvtools.h"
#include "model.h"
#include "dataset.h"
#include "ast.h"
#include "model_select.h"

//#define VERIFY 1

typedef struct _svm_model_factory {
    model_factory_t head;
    int kernel;
} svm_model_factory_t;

class CodeGeneratingSVM: public CvSVM
{
    public:
    CodeGeneratingSVM(sanitized_dataset_t*dataset)
        :CvSVM()
    {
        this->dataset = dataset;
    }

    node_t* get_program() const
    {
        return node_new_with_args(&node_constant, missing_constant());
    }

    float predict(const float* row_sample, int row_len) const
    {
        assert( kernel );
        assert( row_sample );
        assert( params.svm_type == CvSVM::C_SVC );

        int var_count = get_var_count();
        assert( row_len == var_count );

        int class_count = class_labels ? class_labels->cols :
                      params.svm_type == ONE_CLASS ? 1 : 0;

        float result = 0;
        bool local_alloc = 0;
        float* buffer = 0;
        int buf_sz = sv_total*sizeof(buffer[0]) + (class_count+1)*sizeof(int);
        if( buf_sz <= CV_MAX_LOCAL_SIZE )
        {
            buffer = (float*)cvStackAlloc( buf_sz );
            local_alloc = true;
        }
        else
            buffer = (float*)cvAlloc( buf_sz );

        if( params.svm_type == C_SVC || params.svm_type == NU_SVC )
        {
            CvSVMDecisionFunc* df = (CvSVMDecisionFunc*)decision_func;
            int* vote = (int*)(buffer + sv_total);
            int i, j, k;

            memset( vote, 0, class_count*sizeof(vote[0]));
            kernel->calc( sv_total, var_count, (const float**)sv, row_sample, buffer );
            double sum = 0.;

            for( i = 0; i < class_count; i++ )
            {
                for( j = i+1; j < class_count; j++, df++ )
                {
                    sum = -df->rho;
                    int sv_count = df->sv_count;
                    for( k = 0; k < sv_count; k++ )
                        sum += df->alpha[k]*buffer[df->sv_index[k]];

                    vote[sum > 0 ? i : j]++;
                }
            }

            for( i = 1, k = 0; i < class_count; i++ )
            {
                if( vote[i] > vote[k] )
                    k = i;
            }
            result = (float)(class_labels->data.i[k]);
        }
        if( !local_alloc )
            cvFree( &buffer );

        return result;
    }
    sanitized_dataset_t*dataset;
};

static model_t*svm_train(svm_model_factory_t*factory, dataset_t*dataset)
{
    sanitized_dataset_t*d = dataset_sanitize(dataset);

    if(factory->kernel == CvSVM::LINEAR && d->desired_response->num_classes > 4 ||
       factory->kernel == CvSVM::RBF    && d->desired_response->num_classes > 3) {
        /* if we have too many classes one-vs-one SVM classification is too slow */
        sanitized_dataset_destroy(d);
        return 0;
    }

    CodeGeneratingSVM svm(d);
    CvSVMParams params = CvSVMParams(CvSVM::C_SVC, factory->kernel,
                                     /*degree*/0, /*gamma*/1, /*coef0*/0, /*C*/1,
                                     /*nu*/0, /*p*/0, /*class_weights*/0,
                                     cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
    CvMLDataFromExamples data(d);
    const CvMat* values = data.get_values();
    const CvMat* response = data.get_responses();

    model_t*m = 0;
    if(svm.train_auto(values, response, 0, 0, params, 9)) {
        m = (model_t*)malloc(sizeof(model_t));
        m->code = svm.get_program();
    }

    sanitized_dataset_destroy(d);
    return m;
}

static svm_model_factory_t rbf_svm_model_factory = {
    head: {
        name: "rbf svm",
        train: (training_function_t)svm_train,
    },
    CvSVM::RBF
};

static svm_model_factory_t linear_svm_model_factory = {
    head: {
        name: "linear svm",
        train: (training_function_t)svm_train,
    },
    CvSVM::LINEAR
};

model_factory_t* svm_models[] =
{
    (model_factory_t*)&linear_svm_model_factory,
    (model_factory_t*)&rbf_svm_model_factory,
};
int num_svm_models = sizeof(svm_models) / sizeof(svm_models[0]);
