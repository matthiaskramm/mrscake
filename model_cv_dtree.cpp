#include "cvtools.h"
#include "model.h"
#include "ast.h"

class CodeGeneratingDTree: public CvDTree 
{
    void walk(CvDTreeNode*node, node_t*code_node) const
    {
        const int* vidx;
        // m, mstep: missing

        //sample = _sample->data.fl;
        //step = CV_IS_MAT_CONT(_sample->type) ? 1 : _sample->step/sizeof(sample[0]);

        const int*vtype = data->var_type->data.i;
        const int*cmap = data->cat_map ? data->cat_map->data.i : 0;
        const int*cofs = data->cat_ofs ? data->cat_ofs->data.i : 0;

        while(node->Tn > pruned_tree_idx && node->left )
        {
            CvDTreeSplit* split = node->split;
            int dir = 0;
            for( ; !dir && split != 0; split = split->next )
            {
                int i = split->var_idx;
                int ci = vtype[i];
                //float val = sample[i*step];
                //if( m && m[i*mstep] )
                //    continue;

#if 0
                if( ci < 0 ) // ordered
                    dir = val <= split->ord.c ? -1 : 1;
                else // categorical
                {
                    int c = cvRound(val);
                    c = ( (c == 65535) && data->is_buf_16u ) ? -1 : c;
                    dir = CV_DTREE_CAT_DIR(c, split->subset);
                }
#endif

                if( split->inversed )
                    dir = -dir;
            }

            if( !dir )
            {
                double diff = node->right->sample_count - node->left->sample_count;
                dir = diff < 0 ? -1 : 1;
            }
            node = dir < 0 ? node->left : node->right;
        }
    }

    public:
    
    CodeGeneratingDTree()
        :CvDTree()
    {
    }

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
};

static model_t*dtree_train(example_t**examples, int num_examples)
{
    CvMLDataFromExamples data(examples, num_examples);

    CodeGeneratingDTree dtree;
    CvDTreeParams cvd_params(10, 1, 0, false, 16, 0, false, false, 0);
    dtree.train(&data, cvd_params);

    model_t*m = (model_t*)malloc(sizeof(model_t));
    m->code = dtree.get_program();
    return m;
}

model_factory_t dtree_model_factory = {
    name: "dtree",
    train: dtree_train,
    internal: 0,
};

