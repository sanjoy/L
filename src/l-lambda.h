#ifndef __L__LAMBDA__H
#define __L__LAMBDA__H

#include "l-structures.h"
#include "l-context.h"

/*
 * Adjusts the parent pointer of the tokens to point to
 * the correct LLambda.
 */
void l_adjust_bound_variables (LLambda *);

/*
 * Substitutes the previously made assignments to the correct
 * tokens.
 */
LTreeNode *l_substitute_assignments (LTreeNode *, LContext *);

#endif /* __L__LAMBDA__H */
