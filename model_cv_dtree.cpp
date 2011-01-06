#include "cvtools.h"
#include "model.h"
#include "dataset.h"
#include "ast.h"

//#define VERIFY 1

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

    array_t*parse_bitfield(int ci, int subset[2]) const
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
        for(s=0;s<num_categories;s++) {
            if(CV_DTREE_CAT_DIR(s,subset)<0) {
                int c = map_category_back(ci, s);
                if(dataset->wordmap && wordmap_find_category(dataset->wordmap, c)) {
                    a->entries[count++] = string_constant(wordmap_find_category(dataset->wordmap, c));
                } else {
                    a->entries[count++] = category_constant(c);
                }
            }
        }
        return a;
    }

    void walk(CvDTreeNode*node, node_t*current_node) const
    {
        const int*vtype = data->var_type->data.i;

        if(node->Tn <= pruned_tree_idx || !node->left) {
            RETURN((int)floor(node->value+FLT_EPSILON));
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
                array_t*a = parse_bitfield(ci, split->subset);
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
#ifdef VERIFY
    category_t predict(row_t*row)
    {
        CvMat* matrix_row = cvmat_from_row(row, 1);
        category_t c = (int)floor(CvDTree::predict(matrix_row, NULL, false)->value + FLT_EPSILON);
        cvReleaseMat(&matrix_row);
        return c;
    }
#endif
    node_t* get_program() const
    {
        if( !root ) {
            assert(!"The tree has not been trained yet");
        }

        START_CODE(code);
        walk(root,code);
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

static model_t*dtree_train(dataset_t*dataset)
{
    sanitized_dataset_t*d = dataset_sanitize(dataset);

    CvMLDataFromExamples data(d);

    CodeGeneratingDTree dtree(d);
    CvDTreeParams cvd_params(10, 1, 0, false, 16, 0, false, false, 0);
    dtree.train(&data, cvd_params);

    model_t*m = (model_t*)malloc(sizeof(model_t));
    m->wordmap = d->wordmap;d->wordmap=0;
    m->code = dtree.get_program();

#ifdef VERIFY
    verify(dataset, m, &dtree);
#endif
    sanitized_dataset_destroy(d);
    return m;
}

model_factory_t dtree_model_factory = {
    name: "dtree",
    train: dtree_train,
    internal: 0,
};

