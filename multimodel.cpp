#include "ml.hpp"
#include "cvtools.h"
#include <stdio.h>
#include <assert.h>

void print_result(float train_err, float test_err, const CvMat* var_imp)
{
    printf( "train error    %.2f%%\n", train_err );
    printf( "test error    %.2f%%\n\n", test_err );
    
#ifndef VARIABLE_IMPORTANCE
    var_imp = 0;
#endif

    if (var_imp)
    {
        bool is_flt = false;
        if ( CV_MAT_TYPE( var_imp->type ) == CV_32FC1)
            is_flt = true;
        printf( "variable importance\n" );
        for( int i = 0; i < var_imp->cols; i++)
        {
            printf( "%d     %f\n", i, is_flt ? var_imp->data.fl[i] : var_imp->data.db[i] );
        }
    }
    printf("\n");
}

double knearest_calc_error(const CvMat*values, const CvMat*response, const CvMat*new_response, const CvMat*train_sidx, bool is_regression, int type)
{
    //printf("train data: %d, responses: %d\n", train_sidx->cols, response->rows);

    int t;

    int total = 0;
    int train_total = 0;

    double error = 0;
    double train_error = 0;

    int count = 0;
    for(t=0;t<response->rows;t++) {
        int s;
        char train = 0;
        for(s=0;s<train_sidx->cols;s++) {
            if(CV_MAT_ELEM((*train_sidx), unsigned, 0, s) == t) {
                train = 1;
                break;
            }
        }

        float a1 = CV_MAT_ELEM((*response), float, t, 0);
        float a2 = CV_MAT_ELEM((*new_response), float, t, 0);
        if(train) {
            train_total++;
            if(a1!=a2)
                train_error++;
        } else {
            total++;
            if(a1!=a2)
                error++;
        }
    }

    if(type == CV_TRAIN_ERROR)
        return train_error * 100 / train_total;
    else
        return error * 100 / total;
}

static void cvmSetI(CvMat*m, int y, int x, int v)
{
    int*e = (int*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = v;
}
static void cvmSetF(CvMat*m, int y, int x, float f)
{
    float*e = (float*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = f;
}
static int cvmGetI(const CvMat*m, int y, int x)
{
    return CV_MAT_ELEM((*m), int, y, x);
}
static float cvmGetF(const CvMat*m, int y, int x)
{
    return CV_MAT_ELEM((*m), float, y, x);
}

static CvMat cvmat_new_int(int rows, int columns) 
{
    return cvMat(rows, columns, CV_32SC1, calloc(1, sizeof(int)*rows*columns));
}

static CvMat cvmat_new_float(int rows, int columns) 
{
    return cvMat(rows, columns, CV_32FC1, calloc(1, sizeof(int)*rows*columns));
}

CvMat cvmat_make_boolean_class_columns(const CvMat*mat, int num_features)
{
    CvMat columns = cvmat_new_float(mat->rows, num_features);
    int t;
    for(t=0;t<mat->rows;t++) {
        int s;
        for(s=0;s<num_features;s++) {
	    cvmSetF(&columns, t, s, -1);
        }
        int cls = (int)cvmGetF(mat, t, 0);
	assert(cls >= 0 && cls < num_features);
	cvmSetF(&columns, t, cls, 1);
    }
    return columns;
}

CvMat cvmat_remove_column(const CvMat*mat, int column)
{
    assert(column<mat->cols && column>=0);
    assert(mat->cols > 1);

    CvMat new_mat = cvMat(mat->rows, mat->cols-1, mat->type, malloc(mat->rows*mat->cols*8));
    int t;
    int size = CV_ELEM_SIZE(mat->type);
    for(t=0;t<mat->rows;t++) {
        int pos = 0;
        int s;
        for(s=0;s<mat->cols;s++) {
            if(s!=column) {
                memcpy(CV_MAT_ELEM_PTR(new_mat, t, pos), CV_MAT_ELEM_PTR((*mat), t, s), size);
                pos++;
            }
        }
    }
    return new_mat;
}

class CvMySVM: public CvSVM
{
    public:
    float predict2(float*values, int num_values, bool returnDFval)
    {
        return predict(values, num_values, returnDFval);
    }
};
    

double gbt_print_error(CvGBTrees*gbt, const CvMat*values, const CvMat*response, int response_idx, const CvMat*train_sidx)
{
    int count = 0;
    float*tmp = new float[values->cols];
    int t;
    int total = 0;
    int train_total = 0;
    double error = 0;
    double train_error = 0;
    for(t=0;t<values->rows;t++) {
        int s;
        int c=0;
        for(s=0;s<values->cols;s++) {
            tmp[c++] = CV_MAT_ELEM((*values), float, t, s);
        }
        CvMat m = cvMat(1, c, CV_32FC1, tmp);

        float r1 = gbt->predict(&m, 0);
        float r2 = CV_MAT_ELEM((*response), float, t, 0);

        bool train = 0;
        for(s=0;s<train_sidx->cols;s++) {
            if(CV_MAT_ELEM((*train_sidx), unsigned, 0, s) == t) {
                train = 1;
                break;
            }
        }

        if(train) {
            train_total++;
            if(r1!=r2)
                train_error++;
        } else {
            total++;
            if(r1!=r2)
                error++;
        }
    }
    print_result(train_error * 100 / train_total, error * 100 / total, 0);
}

double svm_print_error(CvMySVM*svm, const CvMat*values, const CvMat*response, int response_idx, const CvMat*train_sidx)
{
    int count = 0;
    float*tmp = new float[values->cols];
    int t;
    int total = 0;
    int train_total = 0;
    double error = 0;
    double train_error = 0;
    for(t=0;t<values->rows;t++) {
        int s;
        int c = 0;
        for(s=0;s<values->cols;s++) {
            tmp[c] = CV_MAT_ELEM((*values), float, t, s);
            int r;
            if(s != response_idx) {
                c++;
            }
        }
        float r1 = svm->predict2(tmp, c, true);
        float r2 = CV_MAT_ELEM((*response), float, t, 0);
        //printf("%f %f\n", r1, r2);

        bool train = 0;
        for(s=0;s<train_sidx->cols;s++) {
            if(CV_MAT_ELEM((*train_sidx), unsigned, 0, s) == t) {
                train = 1;
                break;
            }
        }

        if(train) {
            train_total++;
            if(r1!=r2)
                train_error++;
        } else {
            total++;
            if(r1!=r2)
                error++;
        }
    }
    print_result(train_error * 100 / train_total, error * 100 / total, 0);
}

double ann_print_error(CvANN_MLP*ann, const CvMat*values, int num_classes,
                       const CvMat*bool_response, const CvMat*response,
                       int response_idx, const CvMat*train_sidx)
{
    int count = 0;
    float*tmp = new float[values->cols];
    int t;
    int total = 0;
    int train_total = 0;
    double error = 0;
    double train_error = 0;
    float*last_layer = (float*)malloc(sizeof(float)*num_classes);
    for(t=0;t<values->rows;t++) {
        int s;
        int c = 0;
        for(s=0;s<values->cols;s++) {
            if(s != response_idx) {
                tmp[c] = cvmGetF(values, t, s);
                c++;
            }
        }
        CvMat input = cvMat(1, c, CV_32FC1, tmp);
        CvMat output = cvMat(1, num_classes, CV_32FC1, last_layer);

#ifdef PRINT
        for(s=0;s<num_classes;s++) {
            float f = (float)cvmGetF(bool_response, t, s);
            printf("%f ", f);
        }
        printf("| ");
#endif

        int r2 = (int)cvmGetF(response, t, 0);
        ann->predict(&input, &output);

	int r1 = -1;
	float max = -INFINITY;
	for(s=0;s<num_classes;s++) {
#ifdef PRINT
	    printf("%f ", last_layer[s]);
#endif
	    if(last_layer[s] > max) {
		max = last_layer[s];
		r1 = s;
	    }
	}
#ifdef PRINT
        if(r1!=r2)
            printf("E");
        printf("\n");
#endif

        bool train = 0;
        for(s=0;s<train_sidx->cols;s++) {
            if(cvmGetI(train_sidx, 0, s) == t) {
                train = 1;
                break;
            }
        }

        if(train) {
            train_total++;
            if(r1!=r2)
                train_error++;
        } else {
            total++;
            if(r1!=r2)
                error++;
        }
    }
    free(last_layer);
    print_result(train_error * 100 / train_total, error * 100 / total, 0);
}

int main()
{
    const int train_sample_count = 300;
    bool is_regression = false;

    const char* filename = "data/waveform.data";
    int response_idx = 21;

    CvMLData data;

    CvTrainTestSplit spl( train_sample_count );
    
    if(data.read_csv(filename) != 0)
    {
        printf("couldn't read %s\n", filename);
        exit(0);
    }

    data.set_response_idx(response_idx);
    data.change_var_type(response_idx, CV_VAR_CATEGORICAL);
    data.set_train_test_split( &spl );

    const CvMat* values = data.get_values();
    const CvMat* response = data.get_responses();
    const CvMat* missing = data.get_missing();
    const CvMat* var_types = data.get_var_types();
    const CvMat* train_sidx = data.get_train_sample_idx();
    const CvMat* var_idx = data.get_var_idx();
    CvMat*response_map;
    //CvMat*ordered_response = cvPreprocessCategoricalResponses(response, var_idx, response->rows, &response_map, 0);
    int num_classes = response_map->cols;
    
    CvDTree dtree;
    printf("======DTREE=====\n");
    CvDTreeParams cvd_params( 10, 1, 0, false, 16, 0, false, false, 0);
    dtree.train( &data, cvd_params);
    print_result( dtree.calc_error( &data, CV_TRAIN_ERROR), dtree.calc_error( &data, CV_TEST_ERROR ), dtree.get_var_importance() );

#if 0
    /* boosted trees are only implemented for two classes */
    printf("======BOOST=====\n");
    CvBoost boost;
    boost.train( &data, CvBoostParams(CvBoost::DISCRETE, 100, 0.95, 2, false, 0));
    print_result( boost.calc_error( &data, CV_TRAIN_ERROR ), boost.calc_error( &data, CV_TEST_ERROR), 0 );
#endif

    printf("======RTREES=====\n");
    CvRTrees rtrees;
    rtrees.train( &data, CvRTParams( 10, 2, 0, false, 16, 0, true, 0, 100, 0, CV_TERMCRIT_ITER ));
    print_result( rtrees.calc_error( &data, CV_TRAIN_ERROR), rtrees.calc_error( &data, CV_TEST_ERROR ), rtrees.get_var_importance() );

    printf("======ERTREES=====\n");
    CvERTrees ertrees;
    ertrees.train( &data, CvRTParams( 10, 2, 0, false, 16, 0, true, 0, 100, 0, CV_TERMCRIT_ITER ));
    print_result( ertrees.calc_error( &data, CV_TRAIN_ERROR), ertrees.calc_error( &data, CV_TEST_ERROR ), ertrees.get_var_importance() );

    printf("======GBTREES=====\n");
    CvGBTrees gbtrees;
    CvGBTreesParams gbparams;
    gbparams.loss_function_type = CvGBTrees::DEVIANCE_LOSS; // classification, not regression
    gbtrees.train( &data, gbparams);
    
    //gbt_print_error(&gbtrees, values, response, response_idx, train_sidx);
    print_result( gbtrees.calc_error( &data, CV_TRAIN_ERROR), gbtrees.calc_error( &data, CV_TEST_ERROR ), 0);

    printf("======KNEAREST=====\n");
    CvKNearest knearest;
    //bool CvKNearest::train( const Mat& _train_data, const Mat& _responses,
    //                const Mat& _sample_idx, bool _is_regression,
    //                int _max_k, bool _update_base )
    bool is_classifier = var_types->data.ptr[var_types->cols-1] == CV_VAR_CATEGORICAL;
    assert(is_classifier);
    int max_k = 10;
    knearest.train(values, response, train_sidx, is_regression, max_k, false);

    CvMat* new_response = cvCreateMat(response->rows, 1, values->type);
    //print_types();

    //const CvMat* train_sidx = data.get_train_sample_idx();
    knearest.find_nearest(values, max_k, new_response, 0, 0, 0);

    print_result(knearest_calc_error(values, response, new_response, train_sidx, is_regression, CV_TRAIN_ERROR),
                 knearest_calc_error(values, response, new_response, train_sidx, is_regression, CV_TEST_ERROR), 0);

    printf("======== RBF SVM =======\n");
    //printf("indexes: %d / %d, responses: %d\n", train_sidx->cols, var_idx->cols, values->rows);
    CvMySVM svm1;
    CvSVMParams params1 = CvSVMParams(CvSVM::C_SVC, CvSVM::RBF,
                                     /*degree*/0, /*gamma*/1, /*coef0*/0, /*C*/1,
                                     /*nu*/0, /*p*/0, /*class_weights*/0,
                                     cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
    //svm1.train(values, response, train_sidx, var_idx, params1);
    svm1.train_auto(values, response, var_idx, train_sidx, params1);
    svm_print_error(&svm1, values, response, response_idx, train_sidx);

    printf("======== Linear SVM =======\n");
    CvMySVM svm2;
    CvSVMParams params2 = CvSVMParams(CvSVM::C_SVC, CvSVM::LINEAR,
                                     /*degree*/0, /*gamma*/1, /*coef0*/0, /*C*/1,
                                     /*nu*/0, /*p*/0, /*class_weights*/0,
                                     cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
    //svm2.train(values, response, train_sidx, var_idx, params2);
    svm2.train_auto(values, response, var_idx, train_sidx, params2);
    svm_print_error(&svm2, values, response, response_idx, train_sidx);

    printf("======NEURONAL NETWORK=====\n");

    int num_layers = 3;
    CvMat layers = cvMat(1, num_layers, CV_32SC1, calloc(1, sizeof(double)*num_layers*1));
    cvmSetI(&layers, 0, 0, values->cols-1);
    cvmSetI(&layers, 0, 1, num_classes);
    cvmSetI(&layers, 0, 2, num_classes);
    CvANN_MLP ann(&layers, CvANN_MLP::SIGMOID_SYM, 0.0, 0.0);
    CvANN_MLP_TrainParams ann_params;
    //ann_params.train_method = CvANN_MLP_TrainParams::BACKPROP;
    CvMat ann_response = cvmat_make_boolean_class_columns(response, num_classes);

    CvMat values2 = cvmat_remove_column(values, response_idx);
    ann.train(&values2, &ann_response, NULL, train_sidx, ann_params, 0x0000);
    //ann.train(values, &ann_response, NULL, train_sidx, ann_params, 0x0000);

    ann_print_error(&ann, values, num_classes, &ann_response, response, response_idx, train_sidx);

#if 0 /* slow */

    printf("======== Polygonal SVM =======\n");
    //printf("indexes: %d / %d, responses: %d\n", train_sidx->cols, var_idx->cols, values->rows);
    CvMySVM svm3;
    CvSVMParams params3 = CvSVMParams(CvSVM::C_SVC, CvSVM::POLY,
                                     /*degree*/2, /*gamma*/1, /*coef0*/0, /*C*/1,
                                     /*nu*/0, /*p*/0, /*class_weights*/0,
                                     cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
    //svm3.train(values, response, train_sidx, var_idx, params3);
    svm3.train_auto(values, response, var_idx, train_sidx, params3);
    svm_print_error(&svm3, values, response, response_idx, train_sidx);
#endif

    return 0;
}
