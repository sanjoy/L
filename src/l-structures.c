#include "l-structures.h"
#include "l-parser-context.h"
#include "l-token-hashtable.h"

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
l_lambda_new (LMempool *pool, LListNode *args, LTreeNode *body)
{
	LLambda *new = l_mempool_alloc (pool, sizeof (LLambda));
	new->args = args;
	new->body = body;
	l_adjust_free_variables (new);
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
l_register_universal_node (LMempool *pool, LUniversalNodeType type, void *data, void *ctx)
{
	LUniversalNode *new = l_mempool_alloc (pool, sizeof (LUniversalNode));
	LParserContext *context = ctx;
	new->type = type;

	if (type == NODE_ASSIGNMENT) {
		new->assignment = data;
	} else {
		new->lambda = data;
	}

	new->next = context->roots;
	context->roots = new;
}

/* TODO Add pretty printing. */

static void
print_tree_recur (FILE *out, LTreeNode *node)
{
	if (node->first_child != NULL) {
		fprintf (out, "(");
		print_tree_recur (out, node->first_child);
		fprintf (out, ")");
	} else {
		if (node->token != NULL) {
			l_print_token (out, node->token);
			fprintf (out, " ");
		} else {
			assert (node->lambda);
			l_print_lambda (out, node->lambda);
		}
	}

	if (node->right_sibling != NULL)
		print_tree_recur (out, node->right_sibling);
}

void
l_print_tree (FILE *out, LTreeNode *tree)
{
	fprintf (out, "(");
	print_tree_recur (out, tree);
	fprintf (out, ")");
}

void
l_print_list (FILE *out, LListNode *list)
{
	LListNode *i;
	fprintf (out, "(");
	for (i = list; i != NULL; i = i->next) {
		if (list->token != NULL) {
			l_print_token (out, i->token);
			fprintf (out, " ");
		}
	}
	fprintf (out, ")");
}
