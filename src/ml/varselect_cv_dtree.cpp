/* model_cv_dtree.cpp
   Decision tree model

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

#include <stdbool.h>
#include "cvtools.h"
#include "mrscake.h"
#include "dataset.h"
#include "var_selection.h"

class VarSelectingDTree: public CvDTree
{
    public:
    VarSelectingDTree(dataset_t*dataset)
        :CvDTree()
    {
        this->dataset = dataset;
    }

    dataset_t*dataset;
};

class VarSelectingRTrees: public CvRTrees
{
    public:
    VarSelectingRTrees(dataset_t*dataset)
        :CvRTrees()
    {
        this->dataset = dataset;
    }

    dataset_t*dataset;
};

class VarSelectingERTrees: public CvERTrees
{
    public:
    VarSelectingERTrees(dataset_t*dataset)
        :CvERTrees()
    {
        this->dataset = dataset;
    }

    dataset_t*dataset;
};

class VarSelectingGBTrees: public CvGBTrees
{
    public:
    VarSelectingGBTrees(dataset_t*dataset)
        :CvGBTrees()
    {
        this->dataset = dataset;
    }

    dataset_t*dataset;
};

int compare_double_ptr(const void*d1, const void*d2)
{
    double diff = **(double**)d2 - **(double**)d1;
    if(diff<0)
        return -1;
    else if(diff>0)
        return 1;
    else
        return 0;
}

varorder_t*varorder_from_matrix(const CvMat*var_imp, dataset_t*d)
{
    int num = var_imp->cols;
    double*values = (double*)malloc(sizeof(double)*num);
    double**order = (double**)malloc(sizeof(double*)*num);

    int t;
    if(CV_MAT_TYPE( var_imp->type ) == CV_32FC1) {
        for(t=0;t<num;t++) {
            values[t] = var_imp->data.fl[t];
            order[t] = &values[t];
        }
    } else {
        for(t=0;t<num;t++) {
            values[t] = var_imp->data.db[t];
            order[t] = &values[t];
        }
    }
    qsort(order, num, sizeof(order[0]), compare_double_ptr);

    varorder_t*varorder = (varorder_t*)malloc(sizeof(varorder_t));
    varorder->num = num;
    varorder->order = (int*)malloc(sizeof(varorder->order[0])*num);

    for(t=0;t<num;t++) {
        int index = order[t] - values;
        varorder->order[t] = index;
    }
    free(values);
    free(order);
    return varorder;
}

/* TODO: try random perturbation to get different variable 
   orderings (also on individual columns).
*/

extern "C" varorder_t*dtree_var_order(dataset_t*d);

varorder_t*dtree_var_order(dataset_t*d)
{
    CvMLDataFromExamples data(d);

    VarSelectingDTree dtree(d);
    bool use_surrogate_splits = true;
    CvDTreeParams cvd_params(16, 1, 0, use_surrogate_splits, 16, 0, false, false, 0);
    dtree.train(&data, cvd_params);

    const CvMat* var_imp = dtree.get_var_importance();

    return varorder_from_matrix(var_imp, d);
}

