#include <assert.h>
#include "ast.h"

int main()
{
    START_CODE(node)
	IF
	    GT
		ADD
		    VAR(0)
		    VAR(1)
		END;
		VAR(2)
	    END;
	THEN
	    RETURN(1)
	ELSE
	    RETURN(2)
	END;
    END_CODE;

    node_print(node);

    row_t*row = row_new(4);
    row->inputs[0] = variable_make_continuous(5.0);
    row->inputs[1] = variable_make_continuous(3.0);
    row->inputs[2] = variable_make_continuous(3.0);
    row->inputs[3] = variable_make_categorical(5);

    environment_t e;
    e.row = row;
    value_t v = node_eval(node, &e);
    value_print(&v);
    return 0;
}
