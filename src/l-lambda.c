#include "l-lambda.h"
#include "l-structures.h"

#include <stdlib.h>
#include <assert.h>

static void
replace_nonfree (LTreeNode *node, LLambda *parent, int param_idx, int token_idx)
{
	if (node != NULL) {
		if (node->lambda != NULL) {
			replace_nonfree (node->lambda->body, parent, param_idx, token_idx);
		} else if (node->token != NULL && node->token->parent == NULL &&
		           node->token->idx == token_idx) {
			node->token->parent = parent;
			node->token->non_free_idx = param_idx;
		}
		replace_nonfree (node->left, parent, param_idx, token_idx);
		replace_nonfree (node->right, parent, param_idx, token_idx);
	}
}

void
l_adjust_bound_variables (LLambda *lambda)
{
	LListNode *param_iter;
	int param_idx;

	for (param_iter = lambda->args, param_idx = 0;
	     param_iter;
	     param_iter = param_iter->next, param_idx++) {
		param_iter->token->non_free_idx = param_idx;
		replace_nonfree (lambda->body, lambda, param_idx, param_iter->token->idx);
	}
}

struct _ReplaceList {
	LLambda *new, *old;
	struct _ReplaceList *next;
};

typedef struct _ReplaceList ReplaceList;

static LLambda *copy_lambda (LMempool *, LLambda *, ReplaceList *);

#define copy_lambda_null(pool, x, repl) \
	(((x) == NULL) ? NULL : copy_lambda ((pool), (x), (repl)))

static LToken *
copy_token (LMempool *pool, LToken *token, ReplaceList *repl)
{
	LToken *new = l_mempool_alloc (pool, sizeof (LToken));
	new->name = token->name;
	new->idx = token->idx;
	new->non_free_idx = token->idx;

	if (token->parent != NULL) {
		ReplaceList *iter;
		for (iter = repl; iter; iter = iter->next) {
			if (iter->old == token->parent)
				new->parent = iter->new;
		}
	}
	return new;
}

#define copy_token_null(pool, x, repl) \
	(((x) == NULL) ? NULL : copy_token ((pool), (x), (repl)))

static LTreeNode *
copy_tree (LMempool *pool, LTreeNode *node, ReplaceList *repl)
{
	LTreeNode *new;
	
	if (node == NULL)
		return NULL;
	
	new = l_mempool_alloc (pool, sizeof (LTreeNode));
	new->token = copy_token_null (pool, node->token, repl);
	new->lambda = copy_lambda_null (pool, node->lambda, repl);
	new->left = copy_tree (pool, node->left, repl);
	new->right = copy_tree (pool, node->right, repl);

	return new;
}

static LListNode *
copy_list (LMempool *pool, LListNode *list, ReplaceList *repl)
{
	LListNode *new;
	if (list == NULL)
		return NULL;
	new = l_mempool_alloc (pool, sizeof (LListNode));
	new->token = copy_token (pool, list->token, repl);
	new->next = copy_list (pool, list->next, repl);
	return new;
}

static LLambda *
copy_lambda (LMempool *pool, LLambda *lambda, ReplaceList *repl)
{
	LLambda *new = l_mempool_alloc (pool, sizeof (LLambda));
	
	ReplaceList *new_repl = calloc (sizeof (ReplaceList), 1);
	new_repl->old = lambda;
	new_repl->new = new;
	new_repl->next = repl;

	new->args = copy_list (pool, lambda->args, new_repl);
	new->body = copy_tree (pool, lambda->body, new_repl);

	free (new_repl);
	return new;
}

typedef int (*ReplaceTokenPredicate) (LToken *, void *);

static LTreeNode *
substitute (LTreeNode *body, LTreeNode *value, ReplaceTokenPredicate predicate,
            void *user_data, LMempool *pool)
{
	if (body == NULL)
		return NULL;
	
	if (body->token != NULL && predicate (body->token, user_data)) {
		return copy_tree (pool, value, NULL);
	} else if (body->lambda != NULL) {
		body->lambda->body = substitute (body->lambda->body, value, predicate, user_data, pool);
	}

	body->right = substitute (body->right, value, predicate, user_data, pool);
	body->left = substitute (body->left, value, predicate, user_data, pool);

	return body;
}

static int
replace_by_token_id_predicate (LToken *token, void *user_data)
{
	int token_id = *((int *) user_data);
	return token->idx == token_id;
}

LTreeNode *
l_substitute_assignments (LContext *ctx, LTreeNode *node)
{
	LAssignment *assign_iter;

	for (assign_iter = ctx->global_assignments; assign_iter; assign_iter = assign_iter->next) {
		node = substitute (node, assign_iter->rhs, replace_by_token_id_predicate,
		                   &assign_iter->lhs->idx, ctx->mempool);
	}
	return node;
}

typedef struct {
	int param_idx;
	LLambda *parent;
} ReplaceByParamArgs;

static int
replace_by_param_id_predicate (LToken *token, void *user_data)
{
	ReplaceByParamArgs *args = user_data;
	return (args->parent == token->parent && args->param_idx == token->non_free_idx);
}

static LLambda *
apply (LLambda *lambda, LTreeNode *arg, LMempool *pool)
{
	ReplaceByParamArgs rbpa;
	assert (lambda->args);
	rbpa.param_idx = lambda->args->token->non_free_idx;
	rbpa.parent = lambda;
	lambda->body = substitute (lambda->body, arg, replace_by_param_id_predicate,
	                           &rbpa, pool);
	lambda->args = lambda->args->next;
	return lambda;
}

static LTreeNode *
remove_empty_lambdas (LTreeNode *node)
{
	if (node == NULL)
		return NULL;
	if (node->lambda != NULL) {
		if (node->lambda->args == NULL) {
			return node->lambda->body;
		} else
			return node;
	}
	node->left = remove_empty_lambdas (node->left);
	node->right = remove_empty_lambdas (node->right);
	
	return node;
}

LTreeNode *
l_normal_order_reduction (LContext *ctx, LTreeNode *node)
{
	if (node == NULL)
		return node;
	node = remove_empty_lambdas (node);
	
	node->left = l_normal_order_reduction (ctx, node->left);
	node->right = l_normal_order_reduction (ctx, node->right);

	if (L_TREE_NODE_IS_APPLICATION (node)) {
		assert (node->left != NULL);
		if (node->left->lambda != NULL) {
			LLambda *ans = apply (node->left->lambda, node->right, ctx->mempool);
			ans->body->left = l_normal_order_reduction (ctx, ans->body->left);
			ans->body->right = l_normal_order_reduction (ctx, ans->body->right);
			if (ans->args == NULL) {
				return ans->body;
			} else {
				LTreeNode *node = l_mempool_alloc (ctx->mempool, sizeof (LTreeNode));
				node->lambda = ans;
				return node;
			}
		} else {
			return node;
		}
	} else if (node->token != NULL) {
		return node;
	} else {
		return node;
	}
}
