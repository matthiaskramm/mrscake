/* cvtools.h
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

#ifndef __CVTOOLS_H__
#define __CVTOOLS_H__

#include "ml.hpp"
#include "lib/core_c.h"
#include "model.h"
#include "dataset.h"
#include "lib/internal.hpp"

/* FIXME- these should come from opencv's include files */
CVAPI(CvMat*) cvCreateMat( int rows, int cols, int type );
CVAPI(void) cvSet(void* arr, CvScalar value, const void* maskarr);
CvMat* cv_preprocess_categories( const CvMat* responses,
    const CvMat* sample_idx, int sample_all,
    CvMat** out_response_map, CvMat** class_counts );

class CvMLDataFromExamples: public CvMLData
{
    public:
    CvMLDataFromExamples(sanitized_dataset_t*dataset);
    ~CvMLDataFromExamples();
};

CvMat*cvmat_from_row(row_t*row, bool add_one);

void cvmSetI(CvMat*m, int y, int x, int v);
void cvmSetF(CvMat*m, int y, int x, float f);
int cvmGetI(const CvMat*m, int y, int x);
float cvmGetF(const CvMat*m, int y, int x);

#endif // __CVTOOLS_H__