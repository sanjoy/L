#include "l-structures.h"
#include "l-context.h"
#include "l-token-hashtable.h"
#include "l-lambda.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

LToken *
l_token_new (LTokenHashtable *hash, LMempool *pool, char *str)
{
	return l_token_hashtable_hash (hash, str);
}

LTreeNode *
l_tree_cons_tree_tree (LMempool *pool, LTreeNode *a, LTreeNode *b)
{
	LTreeNode *new_node = l_mempool_alloc (pool, sizeof (LTreeNode));

	new_node->left = a;
	new_node->right = b;

	return new_node;
}

LTreeNode *
l_tree_cons_tree_token (LMempool *pool, LTreeNode *node, LToken *token)
{
	LTreeNode *new_node = l_mempool_alloc (pool, sizeof (LTreeNode));

	if (node == NULL) {
		new_node->token = token;
	} else {
		new_node->left = node;
		new_node->right = l_mempool_alloc (pool, sizeof (LTreeNode));
		new_node->right->token = token;
	}

	return new_node;
}

LTreeNode *
l_tree_cons_tree_lambda (LMempool *pool, LTreeNode *tree, LLambda *lambda)
{
	LTreeNode *new_node = l_mempool_alloc (pool, sizeof (LTreeNode));

	if (tree == NULL) {
		new_node->lambda = lambda;
	} else {
		new_node->left = tree;
		new_node->right = l_mempool_alloc (pool, sizeof (LTreeNode));
		new_node->right->lambda = lambda;
	}
	
	return new_node;
}

LListNode *
l_list_cons (LMempool *pool, LToken *token, LListNode *list, void *context)
{
	LListNode *new, *iter;
	LContext *ctx = context;

	for (iter = list; iter; iter = iter->next) {
		if (iter->token->idx == token->idx) {
			CALL_ERROR_HANDLER (ctx, "Duplicate argument in lambda paramenters.");
			return NULL;
		}
	}

	new = l_mempool_alloc (pool, sizeof (LListNode));
	new->token = token;
	new->next = list;
	return new;
}

LLambda *
l_lambda_new (LMempool *pool, LListNode *args, LTreeNode *body, void *context)
{
	LLambda *new = l_mempool_alloc (pool, sizeof (LLambda));
	LContext *ctx = context;
	
	if (body == NULL) {
		CALL_ERROR_HANDLER (ctx, "Lambdas with empty bodies not allowed.");
	}

	new->args = args;
	new->body = body;

	l_substitute_assignments (new, ctx);
	l_adjust_bound_variables (new);
	//	l_normal_order_reduction (new);

	/*
	 * Do something more intelligent in the (L () (L (...) (...))) case.
	 */
	//	if (new->body->lambda != NULL && new->body->right_sibling == NULL && new->args == NULL)
	//		return new->body->lambda;
	
	return new;
}

LAssignment *
l_assignment_new_tree (LMempool *pool, LToken *lhs, LTreeNode *rhs)
{
	LAssignment *new = l_mempool_alloc (pool, sizeof (LAssignment));
	new->lhs = lhs;
	new->rhs = rhs;
	return new;
}

LAssignment *
l_assignment_new_lambda (LMempool *pool, LToken *lhs, LLambda *rhs)
{
	LTreeNode *node = l_mempool_alloc (pool, sizeof (LTreeNode));
	node->lambda = rhs;
	return l_assignment_new_tree (pool, lhs, node);
}

void
l_register_global_node (LMempool *pool, LNodeType type, void *data, void *context)
{
	LContext *ctx = context;

	assert (type == NODE_LAMBDA || type == NODE_ASSIGNMENT);

	if (type == NODE_LAMBDA) {
		LLambda *new = data;
		new->next = ctx->global_lambdas;
		ctx->global_lambdas = new;
		CALL_GLOBAL_NOTIFIER (ctx, NODE_LAMBDA, new);
	} else if (type == NODE_ASSIGNMENT) {
		LAssignment *new = data, *iter;
		for (iter = ctx->global_assignments; iter; iter = iter->next) {
			if (iter->lhs->idx == new->lhs->idx) {
				iter->lhs = new->lhs;
				iter->rhs = new->rhs;
			}
		}
		if (iter == NULL) {
			new->next = ctx->global_assignments;
			ctx->global_assignments = new;
		}
		CALL_GLOBAL_NOTIFIER (ctx, NODE_ASSIGNMENT, new);
	}
}
