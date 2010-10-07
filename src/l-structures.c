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
l_tree_cons_horizontal (LMempool *pool, LToken *token, LTreeNode *node)
{
	LTreeNode *new_node = l_mempool_alloc (pool, sizeof (LTreeNode));
	new_node->token = token;
	new_node->right_sibling = node;
	return new_node;
}

LTreeNode *
l_tree_cons_vertical (LMempool *pool, LTreeNode *node, LTreeNode *tree)
{
	LTreeNode *new_node = l_mempool_alloc (pool, sizeof (LTreeNode));
	new_node->first_child = node;
	new_node->right_sibling = tree;
	return new_node;
}

LTreeNode *
l_tree_cons_lambda (LMempool *pool, LLambda *lambda, LTreeNode *tree)
{
	LTreeNode *new_node = l_mempool_alloc (pool, sizeof (LTreeNode));
	new_node->lambda = lambda;
	new_node->right_sibling = tree;
	return new_node;
}

LListNode *
l_list_cons (LMempool *pool, LToken *token, LListNode *rest)
{
	LListNode *new_node = l_mempool_alloc (pool, sizeof (LListNode));
	new_node->next = rest;
	new_node->token = token;
	return new_node;
}

LLambda *
l_lambda_new (LMempool *pool, LListNode *args, LTreeNode *body, void *context)
{
	LLambda *new = l_mempool_alloc (pool, sizeof (LLambda));
	LContext *ctx = context;

	new->args = args;
	new->body = body;

	l_substitute_assignments (new, ctx);
	l_adjust_free_variables (new);
	l_normal_order_reduction (new);

	/*
	 * Do something more intelligent in the (L () (L (...) (...))) case.
	 */
	if (new->body->lambda != NULL && new->body->right_sibling == NULL && new->args == NULL)
		return new->body->lambda;
	
	return new;
}

LAssignment *
l_assignment_new (LMempool *pool, LToken *tok, LLambda *lam)
{
	LAssignment *new = l_mempool_alloc (pool, sizeof (LAssignment));
	new->lhs = tok;
	new->rhs = lam;
	return new;
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
		LAssignment *new = data;
		new->next = ctx->global_assignments;
		ctx->global_assignments = new;
		CALL_GLOBAL_NOTIFIER (ctx, NODE_ASSIGNMENT, new);
	}

}
