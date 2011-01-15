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

#include "cvtools.h"
#include "model.h"
#include "dataset.h"
#include "easy_ast.h"
#include "model_select.h"

//#define VERIFY 1

typedef struct _dtree_model_factory {
    model_factory_t head;
} dtree_model_factory_t;

class CodeGeneratingDTree: public CvDTree
{
    int map_category_back(int ci, int i) const
    {
        const int*cmap = data->cat_map->data.i;
        const int*cofs = data->cat_ofs->data.i;
        int from = cofs[ci];
        int to = (ci+1 >= data->cat_ofs->cols) ? data->cat_map->cols : cofs[ci+1];
        assert(i>=0 && i<to-from);
        return cmap[from+i];
    }

    int count_categories(int ci) const
    {
        const int*cmap = data->cat_map->data.i;
        const int*cofs = data->cat_ofs->data.i;
        int from = cofs[ci];
        int to = (ci+1 >= data->cat_ofs->cols) ? data->cat_map->cols : cofs[ci+1];
        return to-from;
    }

    array_t*parse_bitfield(int column, int ci, int subset[2]) const
    {
        /* FIXME: We have a maximum of 64 categories, seperated into "left" and "right"
                  ones. There's the case of an input category not occuring in that
                  set, which would require us to treat that input variable as
                  "missing" (as opposed to, as we currently do, as "right"). */
        int s,t;
        int count = 0;

        int num_categories = count_categories(ci);
        for(s=0;s<num_categories;s++) {
            // CAT_DIR == -1 means "go left"
            if(CV_DTREE_CAT_DIR(s,subset)<0) {
                count++;
            }
        }
        array_t* a = array_new(count);
        count = 0;
        assert(dataset->columns[column]->is_categorical);
        for(s=0;s<num_categories;s++) {
            if(CV_DTREE_CAT_DIR(s,subset)<0) {
                int c = map_category_back(ci, s);
                a->entries[count++] = dataset->columns[column]->classes[c];
            }
        }
        return a;
    }

    void walk(CvDTreeNode*node, node_t*current_node) const
    {
        const int*vtype = data->var_type->data.i;
        node_t**current_program = 0;

        if(node->Tn <= pruned_tree_idx || !node->left) {
            assert(dataset->desired_response->is_categorical);
            category_t c = (int)floor(node->value+FLT_EPSILON);
            if(c<0 || c>=dataset->desired_response->num_classes) {
                MISSING_CONSTANT;
            } else {
                GENERIC_CONSTANT(dataset->desired_response->classes[c]);
            }
            return;
        }

        CvDTreeSplit* split = node->split;
        int ci = vtype[split->var_idx];
        /* TODO: a split contains multiple variables we can use (iterate
                 over a split by using split->next). We could wrap an
                 additional if-statement around the code below which will
                 test all possible variables for existence first. The original
                 dtree code also contained a final fallback (which moves to
                 the node with more samples) in case we don't have *any* of the 
                 variables in this split.
         */
        IF
            if(split->inversed) {
                NOT
            }
            if(ci<0) { // ordered
                    LTE
                        VAR(split->var_idx);
                        FLOAT_CONSTANT(split->ord.c);
                    END;
            } else { //categorical
                array_t*a = parse_bitfield(split->var_idx, ci, split->subset);
                    IN
                        VAR(split->var_idx);
                        ARRAY_CONSTANT(a);
                    END;
            }
            if(split->inversed) {
                END;
            }
        THEN
            walk(node->left, current_node);
        ELSE
            walk(node->right, current_node);
        END;
    }

    public:
    CodeGeneratingDTree(sanitized_dataset_t*dataset)
        :CvDTree()
    {
        this->dataset = dataset;
    }

    constant_t predict(row_t*row)
    {
        CvMat* matrix_row = cvmat_from_row(dataset, row, false, true);
        int i = (int)floor(CvDTree::predict(matrix_row, NULL, false)->value + FLT_EPSILON);
        cvReleaseMat(&matrix_row);
        return sanitized_dataset_map_response_class(dataset, i);
    }

    node_t* get_program() const
    {
        if( !root ) {
            assert(!"The tree has not been trained yet");
        }

        START_CODE(code);
          BLOCK
            walk(root,code);
          END;
        END_CODE;
        return code;
    }
    sanitized_dataset_t*dataset;
};

#ifdef VERIFY
void verify(dataset_t*dataset, model_t*m, CodeGeneratingDTree*tree)
{
    example_t**examples = example_list_to_array(dataset);
    int t;
    for(t=0;t<dataset->num_examples;t++) {
        row_t* r = example_to_row(examples[t]);
        category_t c1 = tree->predict(r);
        category_t c2 = model_predict(m, r);
        assert(c1 == c2);
        row_destroy(r);
        //printf("%d %d\n", c1, c2);
    }
    free(examples);
}
#endif

static model_t*dtree_train(model_factory_t*factory, sanitized_dataset_t*d)
{
    CvMLDataFromExamples data(d);

    CodeGeneratingDTree dtree(d);
    CvDTreeParams cvd_params(10, 1, 0, false, 16, 0, false, false, 0);
    dtree.train(&data, cvd_params);

    model_t*m = model_new(d);
    m->code = dtree.get_program();

#ifdef VERIFY
    verify(dataset, m, &dtree);
#endif
    return m;
}

static dtree_model_factory_t dtree_model_factory = {
    {
        name: "dtree",
        train: dtree_train,
    }
};

model_factory_t* dtree_models[] =
{
    (model_factory_t*)&dtree_model_factory,
};
int num_dtree_models = sizeof(dtree_models) / sizeof(dtree_models[0]);
