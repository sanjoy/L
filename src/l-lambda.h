#ifndef __L__LAMBDA__H
#define __L__LAMBDA__H

#include "l-structures.h"
#include "l-context.h"

void l_adjust_free_variables (LLambda *);
void l_normal_order_reduction (LLambda *);
void l_apply (LLambda *, LTreeNode *);
void l_substitute_assignments (LLambda *, LContext *);

#endif /* __L__LAMBDA__H */
