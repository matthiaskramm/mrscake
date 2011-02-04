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
    int max_trees_divide;
    bool use_surrogate_splits;
} dtree_model_factory_t;

int map_category_back(const CvDTreeTrainData* data, int ci, int i)
{
    const int*cmap = data->cat_map->data.i;
    const int*cofs = data->cat_ofs->data.i;
    int from = cofs[ci];
    int to = (ci+1 >= data->cat_ofs->cols) ? data->cat_map->cols : cofs[ci+1];
    assert(i>=0 && i<to-from);
    return cmap[from+i];
}

int count_categories(const CvDTreeTrainData* data, int ci)
{
    const int*cmap = data->cat_map->data.i;
    const int*cofs = data->cat_ofs->data.i;
    int from = cofs[ci];
    int to = (ci+1 >= data->cat_ofs->cols) ? data->cat_map->cols : cofs[ci+1];
    return to-from;
}

array_t*parse_bitfield(CvDTreeTrainData* data, sanitized_dataset_t*dataset, int column, int ci, int subset[2])
{
    /* FIXME: We have a maximum of 64 categories, seperated into "left" and "right"
              ones. There's the case of an input category not occuring in that
              set, which would require us to treat that input variable as
              "missing" (as opposed to, as we currently do, as "right"). */
    int s,t;
    int count = 0;

    int num_categories = count_categories(data, ci);
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
            int c = map_category_back(data, ci, s);
            a->entries[count++] = dataset->columns[column]->classes[c];
        }
    }
    return a;
}

static void walk_dtree_node(CvDTreeTrainData* data, int pruned_tree_idx, sanitized_dataset_t*dataset, CvDTreeNode*node, node_t*current_node, bool resolve_classes, bool as_float)
{
    const int*vtype = data->var_type->data.i;
    node_t**current_program = 0;

    if(node->Tn <= pruned_tree_idx || !node->left) {
        assert(dataset->desired_response->is_categorical);
        category_t c = (int)floor(node->value+FLT_EPSILON);
        if(resolve_classes) {
            if(c<0 || c>=dataset->desired_response->num_classes) {
                MISSING_CONSTANT;
            } else {
                GENERIC_CONSTANT(dataset->desired_response->classes[c]);
            }
        } else {
            if(as_float) {
                FLOAT_CONSTANT(node->value);
            } else {
                INT_CONSTANT(c);
            }
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
                    PARAM(split->var_idx);
                    FLOAT_CONSTANT(split->ord.c);
                END;
        } else { //categorical
            array_t*a = parse_bitfield(data, dataset, split->var_idx, ci, split->subset);
                IN
                    PARAM(split->var_idx);
                    ARRAY_CONSTANT(a);
                END;
        }
        if(split->inversed) {
            END;
        }
    THEN
        walk_dtree_node(data, pruned_tree_idx, dataset, node->left, current_node, resolve_classes, as_float);
    ELSE
        walk_dtree_node(data, pruned_tree_idx, dataset, node->right, current_node, resolve_classes, as_float);
    END;
}

class CodeGeneratingDTree: public CvDTree
{
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
            walk_dtree_node(data,pruned_tree_idx,dataset,root,code, true, false);
          END;
        END_CODE;
        return code;
    }
    sanitized_dataset_t*dataset;
};

class CodeGeneratingRTrees: public CvRTrees
{
    public:
    CodeGeneratingRTrees(sanitized_dataset_t*dataset)
        :CvRTrees()
    {
        this->dataset = dataset;
    }
    
    node_t* get_program() const
    {
        START_CODE(code);
        BLOCK;

        if(ntrees == 1) {
            // optimization: if it's just a single tree, we don't need voting
            walk_dtree_node(trees[0]->data,trees[0]->pruned_tree_idx,dataset,trees[0]->root,current_node,true,false);
        } else {
            SETLOCAL(0)
                ARRAY_NEW(dataset->desired_response->num_classes);
            END;

            int k;
            for(k=0; k<ntrees; k++)
            {
                SETLOCAL(k+1)
                    walk_dtree_node(trees[k]->data,trees[k]->pruned_tree_idx,dataset,trees[k]->root,current_node,false,false);
                END;
                ARRAY_AT_POS_INC
                    GETLOCAL(0);
                    GETLOCAL(k+1);
                END;
            }

            ARRAY_AT_POS
                ARRAY_CONSTANT(sanitized_dataset_classes_as_array(dataset));
                ARRAY_ARG_MAX_I;
                    GETLOCAL(0);
                END;
            END;
        }

        END;
        END_CODE;
        return code;
    }


    sanitized_dataset_t*dataset;
};

class CodeGeneratingERTrees: public CvERTrees
{
    public:
    CodeGeneratingERTrees(sanitized_dataset_t*dataset)
        :CvERTrees()
    {
        this->dataset = dataset;
    }

    node_t* get_program() const
    {
        START_CODE(code);
        BLOCK;

        if(ntrees == 1) {
            walk_dtree_node(trees[0]->data,trees[0]->pruned_tree_idx,dataset,trees[0]->root,current_node,true,false);
        } else {
            SETLOCAL(0)
                ARRAY_NEW(dataset->desired_response->num_classes);
            END;

            int k;
            for(k=0; k<ntrees; k++)
            {
                SETLOCAL(k+1)
                    walk_dtree_node(trees[k]->data,trees[k]->pruned_tree_idx,dataset,trees[k]->root,current_node,false,false);
                END;
                ARRAY_AT_POS_INC
                    GETLOCAL(0);
                    GETLOCAL(k+1);
                END;
            }

            ARRAY_AT_POS
                ARRAY_CONSTANT(sanitized_dataset_classes_as_array(dataset));
                ARRAY_ARG_MAX_I;
                    GETLOCAL(0);
                END;
            END;
        }
        END;
        END_CODE;
        return code;
    }


    sanitized_dataset_t*dataset;
};

class CodeGeneratingGBTrees: public CvGBTrees
{
    public:
    CodeGeneratingGBTrees(sanitized_dataset_t*dataset)
        :CvGBTrees()
    {
        this->dataset = dataset;
    }

    node_t* get_program() const
    {
        START_CODE(code)
        BLOCK
        assert(weak);

        CvSeqReader reader;
        int weak_count = cvSliceLength( CV_WHOLE_SEQ, weak[class_count-1] );
        CvDTree* tree;
        int var_index = 0;
        for(int i=0; i<class_count; ++i) {
	    int orig_class_label = class_labels->data.i[i];
            SETLOCAL(orig_class_label)
                ADD
                    if ((weak[i]) && (weak_count)) {
                        cvStartReadSeq( weak[i], &reader );
                        cvSetSeqReaderPos( &reader, CV_WHOLE_SEQ.start_index );
                        for (int j=0; j<weak_count; ++j)
                        {
                            CV_READ_SEQ_ELEM( tree, reader );
                            MUL
                                walk_dtree_node(tree->data, tree->pruned_tree_idx, dataset, tree->root, current_node, false, true);
                                FLOAT_CONSTANT(params.shrinkage);
                            END;
                        }
                    }
                END;
            END;
        }
        ARRAY_AT_POS
            ARRAY_CONSTANT(sanitized_dataset_classes_as_array(dataset));
            ARG_MAX_F;
                for(int i=0; i<dataset->desired_response->num_classes; ++i) {
                    GETLOCAL(i);
                }
            END;
        END;

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
        row_t* r = example_to_row(examples[t], m->column_names);
        category_t c1 = tree->predict(r);
        category_t c2 = model_predict(m, r);
        assert(c1 == c2);
        row_destroy(r);
        //printf("%d %d\n", c1, c2);
    }
    free(examples);
}
#endif

static int get_max_trees(dtree_model_factory_t*factory, sanitized_dataset_t*d)
{
    /* TODO: more max_trees parameter tweaking (k-fold cross-validation?) */
    return (int)sqrt(d->num_rows) / factory->max_trees_divide;
}

static model_t*dtree_train(dtree_model_factory_t*factory, sanitized_dataset_t*d)
{
    CvMLDataFromExamples data(d);

    CodeGeneratingDTree dtree(d);
    CvDTreeParams cvd_params(16, 1, 0, factory->use_surrogate_splits, 16, 0, false, false, 0);
    dtree.train(&data, cvd_params);

    model_t*m = model_new(d);
    m->code = dtree.get_program();

#ifdef VERIFY
    verify(dataset, m, &dtree);
#endif
    return m;
}

static model_t*rtrees_train(dtree_model_factory_t*factory, sanitized_dataset_t*d)
{
    CvMLDataFromExamples data(d);
    CodeGeneratingRTrees rtrees(d);
    int max_trees = get_max_trees(factory, d);
    if(!max_trees)
        return 0;
    CvRTParams params(16, 2, 0, false, 16, 0, true, 0, max_trees, 0, CV_TERMCRIT_ITER);
    rtrees.train(&data, params);
    model_t*m = model_new(d);
    m->code = rtrees.get_program();
    return m;
}

static model_t*ertrees_train(dtree_model_factory_t*factory, sanitized_dataset_t*d)
{
    if(d->num_columns == 1 && !d->columns[0]->is_categorical) {
        /* OpenCV has a bug in their special case code for a 
           single ordered predictor (see ertrees.cpp:1827) */
        return 0;
    }
    CvMLDataFromExamples data(d);
    CodeGeneratingERTrees ertrees(d);
    int max_trees = get_max_trees(factory, d);
    if(!max_trees)
        return 0;
    CvRTParams params(16, 2, 0, false, 16, 0, true, 0, max_trees, 0, CV_TERMCRIT_ITER);
    ertrees.train(&data, params);
    model_t*m = model_new(d);
    m->code = ertrees.get_program();
    return m;
}

static model_t*gbtrees_train(dtree_model_factory_t*factory, sanitized_dataset_t*d)
{
    CvMLDataFromExamples data(d);
    CodeGeneratingGBTrees gbtrees(d);

    CvGBTreesParams params;
    params.loss_function_type = CvGBTrees::DEVIANCE_LOSS; // classification, not regression
    gbtrees.train(&data, params);

    model_t*m = model_new(d);
    m->code = gbtrees.get_program();
    return m;
}

static dtree_model_factory_t dtree_model_factory = {
    {
        name: "dtree",
        train: (training_function_t)dtree_train,
    },
    max_trees_divide: 0,
    use_surrogate_splits: 0,
};
static dtree_model_factory_t dtree_s_model_factory = {
    {
        name: "dtree (with surrogate splits)",
        train: (training_function_t)dtree_train,
    },
    max_trees_divide: 0,
    use_surrogate_splits: 1,
};
static dtree_model_factory_t rtrees_model_factory = {
    {
        name: "rtrees",
        train: (training_function_t)rtrees_train,
    },
    max_trees_divide: 1,
};
static dtree_model_factory_t rtrees2_model_factory = {
    {
        name: "rtrees (n/2 trees)",
        train: (training_function_t)rtrees_train,
    },
    max_trees_divide: 2,
};
static dtree_model_factory_t rtrees4_model_factory = {
    {
        name: "rtrees (n/4 trees)",
        train: (training_function_t)rtrees_train,
    },
    max_trees_divide: 4,
};
static dtree_model_factory_t rtrees8_model_factory = {
    {
        name: "rtrees (n/8 trees)",
        train: (training_function_t)rtrees_train,
    },
    max_trees_divide: 8,
};
static dtree_model_factory_t rtrees16_model_factory = {
    {
        name: "rtrees (n/16 trees)",
        train: (training_function_t)rtrees_train,
    },
    max_trees_divide: 16,
};
static dtree_model_factory_t ertrees_model_factory = {
    {
        name: "ertrees",
        train: (training_function_t)ertrees_train,
    },
    max_trees_divide: 1,
};
static dtree_model_factory_t ertrees2_model_factory = {
    {
        name: "ertrees (n/2 trees)",
        train: (training_function_t)ertrees_train,
    },
    max_trees_divide: 2,
};
static dtree_model_factory_t ertrees4_model_factory = {
    {
        name: "ertrees (n/4 trees)",
        train: (training_function_t)ertrees_train,
    },
    max_trees_divide: 4,
};
static dtree_model_factory_t ertrees8_model_factory = {
    {
        name: "ertrees (n/8 trees)",
        train: (training_function_t)ertrees_train,
    },
    max_trees_divide: 8,
};
static dtree_model_factory_t ertrees16_model_factory = {
    {
        name: "ertrees (n/16 trees)",
        train: (training_function_t)ertrees_train,
    },
    max_trees_divide: 16,
};
static dtree_model_factory_t gbtrees_model_factory = {
    {
        name: "gbtrees",
        train: (training_function_t)gbtrees_train,
    },
    max_trees_divide: 0,
    use_surrogate_splits: 0,
};

model_factory_t* dtree_models[] =
{
    (model_factory_t*)&dtree_model_factory,
    (model_factory_t*)&dtree_s_model_factory,
    //(model_factory_t*)&rtrees_model_factory,
    //(model_factory_t*)&rtrees2_model_factory,
    //(model_factory_t*)&rtrees4_model_factory,
    //(model_factory_t*)&rtrees8_model_factory,
    //(model_factory_t*)&rtrees16_model_factory,
    (model_factory_t*)&ertrees_model_factory,
    (model_factory_t*)&ertrees2_model_factory,
    (model_factory_t*)&ertrees4_model_factory,
    (model_factory_t*)&ertrees8_model_factory,
    (model_factory_t*)&ertrees16_model_factory,
    (model_factory_t*)&gbtrees_model_factory,
};
int num_dtree_models = sizeof(dtree_models) / sizeof(dtree_models[0]);
