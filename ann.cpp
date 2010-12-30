#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include "ml.hpp"

static void cvmSetI(CvMat*m, int y, int x, int v)
{
    int*e = (int*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = v;
}
static int cvmGetI(const CvMat*m, int y, int x)
{
    return CV_MAT_ELEM((*m), int, y, x);
}
static float cvmGetF(const CvMat*m, int y, int x)
{
    return CV_MAT_ELEM((*m), float, y, x);
}
static void cvmSetF(CvMat*m, int y, int x, float f)
{
    float*e = (float*)(CV_MAT_ELEM_PTR(*m, y, x));
    *e = f;
}

double ann_print_error(CvANN_MLP*ann, const CvMat*values, int num_classes, const CvMat*response)
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
            tmp[c] = cvmGetF(values, t, s);
            c++;
        }
        CvMat input = cvMat(1, c, CV_32FC1, tmp);
        CvMat output = cvMat(1, num_classes, CV_32FC1, last_layer);

        int r2 = -1;
        float max = -INFINITY;
        for(s=0;s<num_classes;s++) {
            float f = (float)cvmGetF(response, t, s);
            printf("%f ", f);
	    if(f > max) {
                max = f;
                r2 = s;
            }
        }
        printf("| ");

        ann->predict(&input, &output);
       
	int r1 = -1;
	max = -INFINITY;
	//printf("%d: ", r2);
	for(s=0;s<num_classes;s++) {
            printf("%f ", last_layer[s]);
	    //printf("%f ", last_layer[s]);
	    if(last_layer[s] > max) {
		max = last_layer[s];
		r1 = s;
	    }
	}
        printf("\n");
    }
    free(last_layer);
}


int main()
{
    int width = 2;
    CvMat values = cvMat(5, width, 0x05, malloc(sizeof(float)*5*width));
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

    int num_classes = 2;
    CvMat response = cvMat(5, num_classes, CV_32FC1, malloc(sizeof(float)*5*num_classes));
    cvmSetF(&response, 0, 0, -1.0); cvmSetF(&response, 0, 1,  1.0);
    cvmSetF(&response, 1, 0,  1.0); cvmSetF(&response, 1, 1, -1.0);
    cvmSetF(&response, 2, 0,  1.0); cvmSetF(&response, 2, 1, -1.0);
    cvmSetF(&response, 3, 0, -1.0); cvmSetF(&response, 3, 1,  1.0);
    cvmSetF(&response, 4, 0, -1.0); cvmSetF(&response, 4, 1,  1.0);
    printf("------------> %f\n", cvmGetF(&response, 1, 0));

    printf("======NEURONAL NETWORK=====\n");

    int num_layers = 2;
    CvMat layers = cvMat(1, num_layers, CV_32SC1, calloc(1, sizeof(int)*num_layers*1));
    cvmSetI(&layers, 0, 0, values.cols);
    cvmSetI(&layers, 0, 1, num_classes);
    CvANN_MLP ann(&layers, CvANN_MLP::SIGMOID_SYM, 0.0, 0.0);
    CvANN_MLP_TrainParams ann_params;
    ann_params.train_method = CvANN_MLP_TrainParams::BACKPROP;

    ann.train(&values, &response, NULL, NULL, ann_params, 0x0000);

    ann_print_error(&ann, &values, num_classes, &response);
}
