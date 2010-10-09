#include "l-lambda.h"
#include "l-structures.h"

#include <stdlib.h>

static void
replace_nonfree (LTreeNode *node, LLambda *parent, int param_idx, int token_idx)
{
	if (node != NULL) {
		if (node->lambda != NULL) {
			replace_nonfree (node->lambda->body, parent, param_idx, token_idx);
		} else if (node->token != NULL && node->token->parent == NULL && node->token->idx == token_idx) {
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

	for (param_iter = lambda->args, param_idx = 0; param_iter; param_iter = param_iter->next, param_idx++) {
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

static LTreeNode *
copy_tree (LMempool *pool, LTreeNode *node, ReplaceList *repl)
{
	LTreeNode *new;
	
	if (node == NULL)

		return NULL;
	new = l_mempool_alloc (pool, sizeof (LTreeNode));
	if (node->token != NULL)
		new->token = copy_token (pool, node->token, repl);
	if (node->lambda != NULL)
		new->lambda = copy_lambda (pool, node->lambda, repl);
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

static void
subst_assignments (LTreeNode *body, LLambda *value, int token_id, LMempool *pool)
{
	if (body == NULL)
		return;
	
	if (body->token != NULL && body->token->idx == token_id) {
		body->token = NULL;
		body->lambda = copy_lambda (pool, value, NULL);
	} else if (body->lambda != NULL) {
		subst_assignments (body->lambda->body, value, token_id, pool);
	}

	subst_assignments (body->right, value, token_id, pool);
	subst_assignments (body->left, value, token_id, pool);
}

void
l_substitute_assignments (LLambda *lambda, LContext *ctx)
{
	LAssignment *assign_iter;

	for (assign_iter = ctx->global_assignments; assign_iter; assign_iter = assign_iter->next) {
		subst_assignments (lambda->body, assign_iter->rhs, assign_iter->lhs->idx, ctx->mempool);
	}
}
