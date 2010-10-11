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
LTreeNode *l_substitute_assignments (LContext *ctx, LTreeNode *);

/*
 * *The* normal order reduction.
 */
LTreeNode *l_normal_order_reduction (LContext *ctx, LTreeNode *);

#define l_substitute_and_reduce(ctx, tree) \
	(l_normal_order_reduction (ctx, \
	                           l_substitute_assignments (ctx, tree)))

#endif /* __L__LAMBDA__H */
