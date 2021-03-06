#include "l-lambda.h"
#include "l-structures.h"

#include "l-pretty-printer.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

static void
replace_nonfree (LTreeNode *node, LLambda *parent, int token_idx)
{
	if (node != NULL) {
		if (node->lambda != NULL) {
			replace_nonfree (node->lambda->body, parent, token_idx);
		} else if (node->token != NULL && node->token->parent == NULL &&
		           node->token->idx == token_idx) {
			node->token->parent = parent;
		}
		replace_nonfree (node->left, parent, token_idx);
		replace_nonfree (node->right, parent, token_idx);
	}
}

void
l_adjust_bound_variables (LLambda *lambda)
{
	LListNode *param_iter;

	for (param_iter = lambda->args; param_iter; param_iter = param_iter->next)
		replace_nonfree (lambda->body, lambda, param_iter->token->idx);
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
	new->parent = token->parent;

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
	new->lazy = copy_tree (pool, node->lazy, repl);

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
		body->lambda->body = substitute (body->lambda->body, value, predicate,
		                                 user_data, pool);
		l_adjust_bound_variables (body->lambda);
	}

	body->right = substitute (body->right, value, predicate, user_data, pool);
	body->left = substitute (body->left, value, predicate, user_data, pool);
	body->lazy = substitute (body->lazy, value, predicate, user_data, pool);

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

	for (assign_iter = ctx->global_assignments; assign_iter;
	     assign_iter = assign_iter->next) {
		node = substitute (node, assign_iter->rhs, replace_by_token_id_predicate,
		                   &assign_iter->lhs->idx, ctx->gc_mempool);
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
	return (args->parent == token->parent && args->param_idx == token->idx);
}

#ifdef __APPLY__LOG

static LLambda *apply_nodebug (LLambda *, LTreeNode *, LContext *);

static LLambda *
apply (LLambda *lambda, LTreeNode *arg, LContext *ctx)
{
	static LPrettyPrinter *pp = NULL;
	LLambda *ans;

	if (pp == NULL) {
		pp = l_pretty_printer_new (ctx);
		l_pretty_printer_set_debug_output (pp, 1);
	}
	printf ("Function : ");
	l_pretty_print_lambda (pp, lambda);
	printf ("\nFunctor : ");
	l_pretty_print_tree (pp, arg);
	printf ("\n");
	ans = apply_nodebug (lambda, arg, ctx);
	printf ("Answer: ");
	l_pretty_print_lambda (pp, ans);
	printf ("\n");
	return ans;
}

#else
#define apply apply_nodebug
#endif

static LLambda *
apply_nodebug (LLambda *lambda, LTreeNode *arg, LContext *ctx)
{
	ReplaceByParamArgs rbpa;
	assert (lambda->args);
	rbpa.param_idx = lambda->args->token->idx;
	rbpa.parent = lambda;
	lambda->body = substitute (lambda->body, arg, replace_by_param_id_predicate,
	                           &rbpa, ctx->gc_mempool);
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

#ifdef __LAMBDA__LOG

static LTreeNode *normal_order_reduction_inner_nodebug (LContext *, LTreeNode *, int, int);

static LTreeNode *
normal_order_reduction_inner (LContext *ctx, LTreeNode *node, int lazy_right, int depth)
{
	static LPrettyPrinter *pp = NULL;
	static int k = 0;
	LTreeNode *ans;

	if (pp == NULL) {
		pp = l_pretty_printer_new (ctx);
		l_pretty_printer_set_debug_output (pp, 1);
	}
	
	printf ("%d IN  ", k);
	l_pretty_print_tree (pp, node);
	printf ("\n");
	k++;
	ans = normal_order_reduction_inner_nodebug (ctx, node, lazy_right, depth);
	k--;
	printf ("%d OUT  ", k);
	l_pretty_print_tree (pp, ans);
 	printf ("\n");
	return ans;
}

#else
#define normal_order_reduction_inner normal_order_reduction_inner_nodebug
#endif

/*
 * The normal order reduction algorithm
 *
 * While the essence of normal order evaluation, is pretty
 * straightforward, there are certain things that need to be taken care
 * of, most notably that an expression may have non-halting
 * sub-expressions, while the expression may be non-halting as a whole (a
 * trivial example is ((L a . b) ((L x . x x) (L x . x x))).
 * 
 * This problem is solved by evaluating the expressions as lazily as
 * possible. This is done by lazily evaluating the _right_ nodes for each
 * application node, using those lazy (and hence not-yet evaluated)
 * values to evaluate the application node, and then eagerly evaluating
 * the resultant node. This ensures we compute only those expressions which
 * are required, and that the reduction machine does not hang on
 * perfectly computable expressions like
 * ((L a . b) ((L x . x x) (L x . x x))).
 */

#define RECURSION_DEPTH_MAX 128

static LTreeNode *
normal_order_reduction_inner_nodebug (LContext *ctx, LTreeNode *node, int lazy, int depth)
{
	if (depth > 128 || node == NULL)
		return node;

	/* Tokens will not be further reduced. */

	if (node->token != NULL)
		return node;

	/* Lambda bodies shall be reduced. */

	if (node->lambda != NULL) {
		node->lambda->body = normal_order_reduction_inner (ctx, node->lambda->body, lazy, depth + 1);
		return node;
	}

	/* This is a lazy result, which is needed now. */
	
	if (node->lazy != NULL)
		return normal_order_reduction_inner (ctx, node->lazy, lazy, depth + 1);

	/* Neither is node a token nor a lambda, hence the following conditions
	 * must hold. */

	assert (node->left != NULL);
	assert (node->right != NULL);
	
	/* The left child is always reduced. */

	node->left = normal_order_reduction_inner (ctx, node->left, lazy, depth + 1);

	/* Precompute the right branch, if this is not a lazy computation. */

	if (!lazy)
		node->right = normal_order_reduction_inner (ctx, node->right, 0, depth + 1);

	if (L_TREE_NODE_IS_APPLICATION (node)) {
		if (node->left->lambda != NULL) {
			/* If this is a lazy computation, mark the right node to be evaluated later.
			 */
			if (lazy && node->right->token != NULL) {
				LTreeNode *lz = l_mempool_alloc (ctx->gc_mempool, sizeof (LTreeNode));
				lz->lazy = node->right;
				node->right = lz;
			}
			LLambda *answer = apply (node->left->lambda, node->right, ctx);
			LTreeNode *node = l_mempool_alloc (ctx->gc_mempool, sizeof (LTreeNode));
			l_adjust_bound_variables (answer);
			node->lambda = answer;
			node->lambda->body = normal_order_reduction_inner (ctx, node->lambda->body, lazy, depth + 1);
			if (node->lambda->args == NULL)
				return node->lambda->body;
			else
				return node;
		} else {
			return node;
		}
	}
	assert (0);
	return node;
}

static LTreeNode *
replace_parents_in_tokens (LLambda *old_lambda, LLambda *new_lambda, LTreeNode *body)
{
	if (body == NULL)
		return NULL;
	if (body->token != NULL) {
		if (body->token->parent == old_lambda)
			body->token->parent = new_lambda;
		return body;
	} else if (body->lambda != NULL) {
		body->lambda->body = replace_parents_in_tokens
			(old_lambda, new_lambda, body->lambda->body);
		return body;
	}
	body->left = replace_parents_in_tokens (old_lambda, new_lambda, body->left);
	body->right = replace_parents_in_tokens (old_lambda, new_lambda, body->right);
	return body;
}

static LTreeNode *
merge_lambdas (LContext *ctx, LTreeNode *node)
{
	if (node == NULL || node->token != NULL)
		return node;
	if (node->lambda != NULL) {
		if (node->lambda->body->lambda != NULL) {
			LListNode *iter;
			for (iter = node->lambda->args; iter->next; iter = iter->next);
			iter->next = node->lambda->body->lambda->args;
			
			node->lambda->body = replace_parents_in_tokens (node->lambda,
			                                                node->lambda->body->lambda,
			                                                node->lambda->body->lambda->body);
			node->lambda->body->lambda = NULL;
		}
		return node;
	}
	node->left = merge_lambdas (ctx, node->left);
	node->right = merge_lambdas (ctx, node->right);
	return node;
}

struct _StringList {
	char *str;
	struct _StringList *next;
};

typedef struct _StringList StringList;

static StringList *
get_unresolved_variables (LTreeNode *node, StringList *prev, LMempool *pool)
{
	if (node == NULL)
		return prev;

	if (node->token != NULL) {
		if (node->token->name [0] == ':') {
			StringList *n = l_mempool_alloc (pool, sizeof (StringList));
			n->str = node->token->name;
			n->next = prev;
			prev = n;
		}
	}
	if (node->lambda != NULL)
		return get_unresolved_variables (node->lambda->body, prev, pool);
	prev = get_unresolved_variables (node->left, prev, pool);
	prev = get_unresolved_variables (node->right, prev, pool);
	return prev;
}

LTreeNode *
l_normal_order_reduction (LContext *ctx, LTreeNode *node)
{
	StringList *unresolved_symbols = NULL;
	LMempool *err = l_mempool_new (0);
	
	node = remove_empty_lambdas (node);

	node = l_substitute_assignments (ctx, node);

	unresolved_symbols = get_unresolved_variables (node, NULL, err);

	if (unresolved_symbols != NULL) {
		int total_len = 0;
		StringList *iter;
		char *err, *err_i;
		for (iter = unresolved_symbols; iter; iter = iter->next)
			total_len += strlen (iter->str) + 2;
		err = malloc (total_len + 100);
		err_i = err + sprintf (err, "The following symbols could not be resolved: { ");
		for (iter = unresolved_symbols; iter; iter = iter->next)
			err_i += sprintf (err_i, "%s ", iter->str);
		sprintf (err_i, "}");
		L_CALL_ERROR_HANDLER (ctx, err);
		free (err);
	}

	l_mempool_destroy (err);

	/* First evaluate lazily. */
	node = normal_order_reduction_inner (ctx, node, 1, 0);

	/* Now hard-evaluate everything. */
	node = normal_order_reduction_inner (ctx, node, 0, 0);

	/* Change L f . L x . f x to L f x . f x */
	node = merge_lambdas (ctx, node);

	return node;
}
