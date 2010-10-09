#include "l-lambda.h"
#include "l-structures.h"

#if 0

static void
replace_nonfree (LTreeNode *node, LLambda *parent, int param_idx, int token_idx)
{
	if (node->lambda != NULL) {
		replace_nonfree (node->lambda->body, parent, param_idx, token_idx);
	} else if (node->token != NULL && node->token->parent_lambda == NULL && node->token->idx == token_idx) {
		node->token->parent_lambda = parent;
		node->token->non_free_idx = param_idx;
	}
	if (node->first_child != NULL)
		replace_nonfree (node->first_child, parent, param_idx, token_idx);
	if (node->right_sibling != NULL)
		replace_nonfree (node->right_sibling, parent, param_idx, token_idx);
}

void
l_adjust_free_variables (LLambda *lambda)
{
	LListNode *param_iter;
	int param_idx;

	for (param_iter = lambda->args, param_idx = 0; param_iter; param_iter = param_iter->next, param_idx++) {
		param_iter->token->non_free_idx = param_idx;
		replace_nonfree (lambda->body, lambda, param_idx, param_iter->token->idx);
	}
}

static void
replace_in_body (LTreeNode *node, LLambda *parent, int param_idx, LTreeNode *value)
{
	if (node->lambda != NULL) {
		replace_in_body (node->lambda->body, parent, param_idx, value);
	} else if (node->token != NULL && node->token->parent_lambda == parent && node->token->non_free_idx == param_idx) {
		node->token = value->token;
		node->lambda = value->lambda;
		node->first_child = value->first_child;
	}
	if (node->first_child != NULL)
		replace_in_body (node->first_child, parent, param_idx, value);
	if (node->right_sibling != NULL)
		replace_in_body (node->right_sibling, parent, param_idx, value);
}

void
l_apply (LLambda *function, LTreeNode *param)
{
	if (function->args) {
		replace_in_body (function->body, function, function->args->token->non_free_idx, param);
		function->args = function->args->next;
	}
}

static void
normal_order_reduction_recursive (LTreeNode *body)
{
	if (body == NULL)
		return;
	
	if (body->lambda != NULL) {
		while (body->right_sibling != NULL && body->lambda->args != NULL) {
			l_apply (body->lambda, body->right_sibling);
			body->right_sibling = body->right_sibling->right_sibling;
		}
	}
	if (body->lambda != NULL)
			normal_order_reduction_recursive (body->lambda->body);
	normal_order_reduction_recursive (body->right_sibling);
	normal_order_reduction_recursive (body->first_child);
}

static LTreeNode *
remove_empty_lambdas (LTreeNode *node)
{
	if (node->lambda != NULL)
		node->lambda->body = remove_empty_lambdas (node->lambda->body);
	if (node->first_child != NULL)
		node->first_child = remove_empty_lambdas (node->first_child);
	if (node->right_sibling)
		node->right_sibling = remove_empty_lambdas (node->right_sibling);
	if (node->lambda != NULL && node->lambda->args == NULL)
		return node->lambda->body;
	else
		return node;
}

static LTreeNode *
remove_single_element_lists (LTreeNode *node)
{
	if (node->lambda != NULL)
		node->lambda->body = remove_single_element_lists (node->lambda->body);
	if (node->first_child != NULL) {
		if (node->first_child->right_sibling == NULL) {
			node->first_child->right_sibling = node->right_sibling;
			node = node->first_child;
		} else {
			node->first_child = remove_single_element_lists (node->first_child);
		}
	}
	if (node->right_sibling != NULL)
		node->right_sibling = remove_single_element_lists (node->right_sibling);
	return node;
}

void
l_normal_order_reduction (LLambda *lambda)
{
	normal_order_reduction_recursive (lambda->body);
	lambda->body = remove_empty_lambdas (lambda->body);
	lambda->body = remove_single_element_lists (lambda->body);
}

static void
subst_assignments (LTreeNode *body, LLambda *value, int token_id)
{
	if (body == NULL)
		return;
	
	if (body->token != NULL && body->token->idx == token_id) {
		body->token = NULL;
		body->lambda = value;
	} else if (body->lambda != NULL) {
		subst_assignments (body->lambda->body, value, token_id);
	}

	subst_assignments (body->right_sibling, value, token_id);
	subst_assignments (body->first_child, value, token_id);
}

void
l_substitute_assignments (LLambda *lambda, LContext *ctx)
{
	LAssignment *assign_iter;

	for (assign_iter = ctx->global_assignments; assign_iter; assign_iter = assign_iter->next) {
		subst_assignments (lambda->body, assign_iter->rhs, assign_iter->lhs->idx);
	}
}

#endif // #if 0
