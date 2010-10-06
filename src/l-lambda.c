#include "l-lambda.h"

static void
replace_nonfree (LTreeNode *node, LLambda *parent, int param_idx)
{
	if (node->lambda != NULL) {
		replace_nonfree (node->lambda->body, parent, param_idx);
	} else if (node->token != NULL && node->token->parent_lambda == NULL) {
		node->token->parent_lambda = parent;
		node->token->non_free_idx = param_idx;
	}
	if (node->first_child != NULL)
		replace_nonfree (node->first_child, parent, param_idx);
	if (node->right_sibling != NULL)
		replace_nonfree (node->right_sibling, parent, param_idx);
}

void
l_adjust_free_variables (LLambda *lambda)
{
	LListNode *param_iter;
	LTreeNode *body = lambda->body;
	int param_idx;

	for (param_iter = lambda->args, param_idx = 0; param_iter; param_iter = param_iter->next, param_idx++) {
		param_iter->token->non_free_idx = param_idx;
		replace_nonfree (lambda->body, lambda, param_idx);
	}
}
