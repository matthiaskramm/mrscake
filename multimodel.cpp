#include "ml.hpp"
#include <stdio.h>
/*
The sample demonstrates how to use different decision trees.
*/
void print_result(float train_err, float test_err, const CvMat* var_imp)
{
    printf( "train error    %f\n", train_err );
    printf( "test error    %f\n\n", test_err );
    
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
    printf("train data: %d, responses: %d\n", train_sidx->cols, response->rows);

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
        if(is_regression) {
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
        } else {
            float a1 = CV_MAT_ELEM((*response), float, t, 0);
            float a2 = CV_MAT_ELEM((*new_response), float, t, 0);
            if(train) {
                train_total++;
                train_error += (a1-a2)*(a1-a2);
            } else {
                total++;
                error += (a1-a2)*(a1-a2);
            }
        }
    }
    if(!is_regression) {
        error *= 100;
        train_error *= 100;
    }
    if(type == CV_TRAIN_ERROR)
        return train_error / train_total;
    else
        return error / total;
}

class CvMySVM: public CvSVM
{
    public:
    float predict2(float*values, int num_values, bool returnDFval)
    {
        return predict(values, num_values, returnDFval);
    }
};

CVAPI(CvMat*) cvCreateMat( int rows, int cols, int type );

int main()
{
    const int train_sample_count = 300;

//#define LEPIOTA
#ifdef LEPIOTA
    const char* filename = "/usr/share/opencv/samples/c/agaricus-lepiota.data";
#else
    const char* filename = "/usr/share/opencv/samples/c/waveform.data";
#endif

    CvMLData data;

    CvTrainTestSplit spl( train_sample_count );
    
    if(data.read_csv(filename) != 0)
    {
        printf("couldn't read %s\n", filename);
        exit(0);
    }

    bool is_regression = true;
    int response_idx = 0;
#ifdef LEPIOTA
    data.set_response_idx( response_idx = 0 );
#else
    data.set_response_idx( response_idx = 21 );
    data.change_var_type( 21, CV_VAR_CATEGORICAL );
    is_regression = false;
#endif

    data.set_train_test_split( &spl );
    
    CvDTree dtree;
    printf("======DTREE=====\n");
    dtree.train( &data, CvDTreeParams( 10, 2, 0, false, 16, 0, false, false, 0 ));
    print_result( dtree.calc_error( &data, CV_TRAIN_ERROR), dtree.calc_error( &data, CV_TEST_ERROR ), dtree.get_var_importance() );

    if(is_regression) {
        printf("======BOOST=====\n");
        CvBoost boost;
        boost.train( &data, CvBoostParams(CvBoost::DISCRETE, 100, 0.95, 2, false, 0));
        print_result( boost.calc_error( &data, CV_TRAIN_ERROR ), boost.calc_error( &data, CV_TEST_ERROR), 0 );
    }

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
    gbtrees.train( &data, gbparams);
    print_result( gbtrees.calc_error( &data, CV_TRAIN_ERROR), gbtrees.calc_error( &data, CV_TEST_ERROR ), 0);

    printf("======KNEAREST=====\n");
    CvKNearest knearest;
    const CvMat* values = data.get_values();
    const CvMat* response = data.get_responses();
    const CvMat* missing = data.get_missing();
    const CvMat* var_types = data.get_var_types();
    const CvMat* train_sidx = data.get_train_sample_idx();
    const CvMat* var_idx = data.get_var_idx();
    //bool CvKNearest::train( const Mat& _train_data, const Mat& _responses,
    //                const Mat& _sample_idx, bool _is_regression,
    //                int _max_k, bool _update_base )
    bool is_classifier = var_types->data.ptr[var_types->cols-1] == CV_VAR_CATEGORICAL;
    printf("is classifier: %d\n", is_classifier);
    int max_k = 10;
    knearest.train(values, response, train_sidx, is_regression, max_k, false);

    CvMat* new_response = cvCreateMat(response->rows, 1, values->type);
    //print_types();

    //const CvMat* train_sidx = data.get_train_sample_idx();
    knearest.find_nearest(values, max_k, new_response, 0, 0, 0);

    print_result(knearest_calc_error(values, response, new_response, train_sidx, is_regression, CV_TRAIN_ERROR),
                 knearest_calc_error(values, response, new_response, train_sidx, is_regression, CV_TEST_ERROR), 0);

    printf("========SVM=======\n");
    printf("indexes: %d / %d, responses: %d\n", train_sidx->cols, var_idx->cols, values->rows);
    CvMySVM svm;
    CvSVMParams params = CvSVMParams(CvSVM::C_SVC, CvSVM::RBF, /*degree*/0, /*gamma*/1, /*coef0*/0, /*C*/1, /*nu*/0, /*p*/0, /*class_weights*/0, cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
    //svm.train(values, response, train_sidx, var_idx, params);
    printf("%dx%d\n", values->rows, values->cols);
    svm.train(values, response, var_idx, train_sidx, params);

    int count = 0;
    float*tmp = new float[values->cols];
    int t;
    for(t=0;t<values->rows;t++) {
        int s;
        int c = 0;
        for(s=0;s<values->cols;s++) {
            tmp[c] = CV_MAT_ELEM((*values), float, t, s);
            if(response_idx != s)
                c++;
        }
        float r1 = svm.predict2(tmp, c, true);
        float r2 = CV_MAT_ELEM((*response), float, t, 0);
        //printf("%f %f\n", r1, r2);
    }

    return 0;
}
