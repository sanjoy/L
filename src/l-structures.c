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
			L_CALL_ERROR_HANDLER (ctx, "Duplicate argument in lambda paramenters.");
			return NULL;
		}
	}

	new = l_mempool_alloc (pool, sizeof (LListNode));
	new->token = token;
	new->next = list;
	return new;
}

LLambda *
l_lambda_new (void *context, LListNode *args, LTreeNode *body)
{
	LContext *ctx = context;
	LLambda *new = l_mempool_alloc (ctx->mempool, sizeof (LLambda));
	
	if (body == NULL) {
		L_CALL_ERROR_HANDLER (ctx, "Lambdas with empty bodies not allowed.");
	}

	new->args = args;
	new->body = body;
	
	l_adjust_bound_variables (new);

	return new;
}

LAssignment *
l_assignment_new_tree (void *context, LToken *lhs, LTreeNode *rhs)
{
	LContext *ctx = context;
	LAssignment *new = l_mempool_alloc (ctx->mempool, sizeof (LAssignment));
	new->lhs = lhs;
	new->rhs = l_substitute_and_reduce (ctx, rhs);
	return new;
}

LAssignment *
l_assignment_new_lambda (void *context, LToken *lhs, LLambda *rhs)
{
	LContext *ctx = context;
	LTreeNode *node = l_mempool_alloc (ctx->mempool, sizeof (LTreeNode));

	rhs->body = l_substitute_assignments (ctx, rhs->body);
	node->lambda = rhs;

	return l_assignment_new_tree (context, lhs, node);
}

void
l_global_node_new (void *context, LGlobalNodeType type, void *data)
{
	LContext *ctx = context;

	assert (type == NODE_LAMBDA || type == NODE_ASSIGNMENT || type == NODE_EXPRESSION);

	if (type == NODE_LAMBDA && ((LLambda *) data)->args != NULL) {
		LLambda *new = data;
		new->next = ctx->global_lambdas;
		new->body = l_substitute_and_reduce (ctx, new->body);
		ctx->global_lambdas = new;
		L_CALL_GLOBAL_NOTIFIER (ctx, NODE_LAMBDA, new);
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
		L_CALL_GLOBAL_NOTIFIER (ctx, NODE_ASSIGNMENT, new);
	} else if (type == NODE_EXPRESSION || (type == NODE_LAMBDA && ((LLambda *) data)->args == NULL)) {
		if (type == NODE_LAMBDA)
			ctx->last_expression = l_substitute_and_reduce (ctx, ((LLambda *) data)->body);
		else
			ctx->last_expression = l_substitute_and_reduce (ctx, data);
		L_CALL_GLOBAL_NOTIFIER (ctx, NODE_EXPRESSION, ctx->last_expression);
	}
}
