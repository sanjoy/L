#include <stdio.h>

#include "l-context.h"
#include "l-pretty-printer.h"

void
l_error_handler (int line, const char *err)
{
	fprintf (stderr, "Parsing error: %d : %s\n", line, err);
}

int
main (void)
{
	LContext *ctx = l_context_new_from_file (stdin);
	LUniversalNode *nd;
	LPrettyPrinter *pprinter = l_pretty_printer_new (ctx);
	ctx->error_handler = l_error_handler;
	l_parse_using_context (ctx);
	for (nd = ctx->roots; nd; nd = nd->next) {
		l_pretty_print_universal_node (pprinter, nd);
	}
	l_destroy_context (ctx);
	return 0;
}
