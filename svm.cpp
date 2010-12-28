#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include "ml.hpp"

static void cvmat_dump(CvMat*mat)
{
    int x,y;
    printf("%dx%d\n", mat->cols, mat->rows);
    for(y=0;y<mat->rows;y++) {
        for(x=0;x<mat->cols;x++) {
            printf("%5.2f ", CV_MAT_ELEM((*mat), float, y, x));
        }
        printf("\n");
    }
    printf("\n");
}

void cvmSetI(CvMat*m, int y, int x, int v)
{
    int*e = (int*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = v;
}

CvMat make_counting_matrix(int width, int height)
{
    CvMat m = cvMat(height, width, CV_32S, malloc(sizeof(double)*width*height));
    int x,y;
    for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
            cvmSetI(&m, y, x, x+y);
        }
    }
    return m;
}

int main()
{
    int width = 2;
    CvMat values = cvMat(5, width, 0x05, malloc(sizeof(double)*5*width));
    cvmSet(&values, 0, 0, 1.0);
    cvmSet(&values, 1, 0, 2.0);
    cvmSet(&values, 2, 0, 3.0);
    cvmSet(&values, 3, 0, 4.0);
    cvmSet(&values, 4, 0, 5.0);

    cvmSet(&values, 0, 1, 0.0);
    cvmSet(&values, 1, 1, 0.0);
    cvmSet(&values, 2, 1, 1.0);
    cvmSet(&values, 3, 1, 1.0);
    cvmSet(&values, 4, 1, 1.0);

    CvMat response = cvMat(5, 1, CV_32SC1, malloc(sizeof(double)*5*1));
    cvmSetI(&response, 0, 0, 2);
    cvmSetI(&response, 1, 0, 1);
    cvmSetI(&response, 2, 0, 1);
    cvmSetI(&response, 3, 0, 2);
    cvmSetI(&response, 4, 0, 2);

    CvSVM svm;
    CvSVMParams params = CvSVMParams(CvSVM::C_SVC, CvSVM::LINEAR, /*degree*/1,
                                     /*gamma*/1, /*coef0*/0, /*C*/1, /*nu*/0.5,
                                     /*p*/0, /*class_weights*/0,
                                     cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
    CvMat count_to_width = make_counting_matrix(width,1);
    CvMat count_to_height = make_counting_matrix(1,5);
    
    svm.train_auto(&values, &response, &count_to_width, 0, params, 2);
    //svm.train(&values, &response, &count_to_width, 0, params);

    int count = 0;
    float*tmp = new float[values.cols];
    int t;
    for(t=0;t<values.rows;t++) {
        CvMat row = cvMat(1, width, values.type, CV_MAT_ELEM_PTR(values, t, 0));
        float r1 = svm.predict(&row);
        float r2 = CV_MAT_ELEM(response, int, t, 0);
        printf("%f %f\n", r1, r2);
    }
}
