#ifndef __CVTOOLS_H__
#define __CVTOOLS_H__

#include "ml.hpp"
#include "lib/core_c.h"
#include "model.h"
#include "dataset.h"

/* FIXME- these should come from opencv's include files */
CVAPI(CvMat*) cvCreateMat( int rows, int cols, int type );
CVAPI(CvMat*) cvPreprocessCategoricalResponses( const CvMat* responses, const CvMat* sample_idx, int sample_all, CvMat** out_response_map, CvMat** class_counts);
CVAPI(void) cvSet(void* arr, CvScalar value, const void* maskarr);

class CvMLDataFromExamples: public CvMLData
{
    public:
    CvMLDataFromExamples(sanitized_dataset_t*dataset);
    ~CvMLDataFromExamples();
};

CvMat*cvmat_from_row(row_t*row, bool add_one);

#endif // __CVTOOLS_H__
