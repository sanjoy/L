#include "l-pretty-printer.h"

struct _LPrettyPrinter {
	FILE *out;
	int current_indent;
	int debug;
};

LPrettyPrinter *
l_pretty_printer_new (LParserContext *ctx)
{
	LPrettyPrinter *new = l_mempool_alloc (ctx->mempool, sizeof (LPrettyPrinter));
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
		fprintf (pprinter->out, "%s [%d %d]", token->name, token->idx, token->non_free_idx);
	else
		fprintf (pprinter->out, "%s", token->name);
}

#define PRINT_N_TIMES(n,text,out) do {	  \
		int ___i; \
		for (___i = 0; ___i < (n); ___i++) \
			fprintf (out, text); \
	} while (0)

static void
print_tree_full (LPrettyPrinter *, LTreeNode *, int, int);

static void
print_lambda_full (LPrettyPrinter *pprinter, LLambda *lambda, int delta)
{
	int prev_current_indent = pprinter->current_indent;
	PRINT_N_TIMES (pprinter->current_indent - delta, " ", pprinter->out);
	pprinter->current_indent += fprintf (pprinter->out, "(L ");
	l_pretty_print_list (pprinter, lambda->args);
	fprintf (pprinter->out, "\n");
	print_tree_full (pprinter, lambda->body, 1, 0);
	fprintf (pprinter->out, ")\n");
	pprinter->current_indent = prev_current_indent;
}

void
l_pretty_print_lambda (LPrettyPrinter *pprinter, LLambda *lambda)
{
	print_lambda_full (pprinter, lambda, 0);
}

static void
print_tree_full (LPrettyPrinter *pprinter, LTreeNode *node, int init_indent, int init_delta)
{
	LTreeNode *iter;
	int prev_current_indent = pprinter->current_indent;

	if (init_indent)
		PRINT_N_TIMES (prev_current_indent - init_delta, " ", pprinter->out);
	
	pprinter->current_indent += fprintf (pprinter->out, "(");
	for (iter = node; iter; iter = iter->right_sibling) {
		if (iter->first_child != NULL) {
			if (node != iter)
				fprintf (pprinter->out, "\n");
			print_tree_full (pprinter, iter->first_child, node != iter, 0);
			if (iter->right_sibling != NULL) {
				fprintf (pprinter->out, "\n");
				PRINT_N_TIMES (prev_current_indent + 1, " ", pprinter->out);
			}
		} else if (iter->token != NULL) {
			l_pretty_print_token (pprinter, iter->token);
			if (iter->right_sibling != NULL)
				fprintf (pprinter->out, " ");
		} else {
			l_pretty_print_lambda (pprinter, iter->lambda);
		}
	}
	
	fprintf (pprinter->out, ")");
	pprinter->current_indent = prev_current_indent;
}

void
l_pretty_print_tree (LPrettyPrinter *pprinter, LTreeNode *tree)
{
	print_tree_full (pprinter, tree, 0, 0);
}

void
l_pretty_print_list (LPrettyPrinter *pprinter, LListNode *list)
{
	LListNode *i;
	fprintf (pprinter->out, "(");
	for (i = list; i; i = i->next) {
		l_pretty_print_token (pprinter, i->token);
		if (i->next != NULL)
			fprintf (pprinter->out, " ");
	}
	fprintf (pprinter->out, ")");
}

void
l_pretty_print_assignment (LPrettyPrinter *pprinter, LAssignment *assign)
{
	int prev_current_indent = pprinter->current_indent;
	int delta = fprintf (pprinter->out, "%s <- ", assign->lhs->name);
	pprinter->current_indent += delta;

	print_lambda_full (pprinter, assign->rhs, delta);

	fprintf (pprinter->out, "\n");
	pprinter->current_indent = prev_current_indent;
}
