#include "l-pretty-printer.h"

struct _LPrettyPrinter {
	FILE *out;
	int current_indent;
	int debug;
};

LPrettyPrinter *
l_pretty_printer_new (LContext *ctx)
{
	LPrettyPrinter *new = l_mempool_alloc (ctx->nogc_mempool, sizeof (LPrettyPrinter));
	new->out = stdout;
	return new;
}

void
l_pretty_printer_set_output (LPrettyPrinter *pprinter, FILE *out)
{
	pprinter->out = out;
}

void
l_pretty_printer_set_debug_output (LPrettyPrinter *pprinter, int deb)
{
	pprinter->debug = deb;
}

void
l_pretty_print_token (LPrettyPrinter *pprinter, LToken *token)
{
	if (pprinter->debug)
		fprintf (pprinter->out, "%s [%d] {%p}", token->name, token->idx, token->parent);
	else
		fprintf (pprinter->out, "%s", token->name);
}

static int
print_tree_full (LPrettyPrinter *, LTreeNode *);

void
l_pretty_print_lambda (LPrettyPrinter *pprinter, LLambda *lambda)
{
	fprintf (pprinter->out, "L ");
	if (pprinter->debug)
		fprintf (pprinter->out, "{%p} ", lambda);
	l_pretty_print_list (pprinter, lambda->args);
	fprintf (pprinter->out, " . ");
	l_pretty_print_tree (pprinter, lambda->body);
}

static int
print_tree_full (LPrettyPrinter *pprinter, LTreeNode *node)
{
	int print_paren, first_element;
	if (node != NULL)
		first_element = print_tree_full (pprinter, node->left);
	else
		return 1;

	if (!first_element)
		fprintf (pprinter->out, " ");

	if (node->token != NULL) {
		l_pretty_print_token (pprinter, node->token);
	} else if (node->lambda != NULL) {
		fprintf (pprinter->out, "(");
		l_pretty_print_lambda (pprinter, node->lambda);
		fprintf (pprinter->out, ")");
	} else if (node->lazy != NULL) {
		fprintf (pprinter->out, "[");
		print_tree_full (pprinter, node->lazy);
		fprintf (pprinter->out, "]");
	}
	
	print_paren = (node->right != NULL && L_TREE_NODE_IS_APPLICATION (node->right));

	if (print_paren)
		fprintf (pprinter->out, "(");

	print_tree_full (pprinter, node->right);

	if (print_paren)
		fprintf (pprinter->out, ")");

	return 0;
}

void
l_pretty_print_tree (LPrettyPrinter *pprinter, LTreeNode *tree)
{
	print_tree_full (pprinter, tree);
}

void
l_pretty_print_list (LPrettyPrinter *pprinter, LListNode *list)
{
	LListNode *i;
	for (i = list; i; i = i->next) {
		l_pretty_print_token (pprinter, i->token);
		if (i->next != NULL)
			fprintf (pprinter->out, " ");
	}
}

void
l_pretty_print_assignment (LPrettyPrinter *pprinter, LAssignment *assign)
{
	fprintf (pprinter->out, "%s = ", assign->lhs->name);
	l_pretty_print_tree (pprinter, assign->rhs);
}
