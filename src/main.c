#include <stdio.h>
#include <stdlib.h>

#include "l-context.h"
#include "l-pretty-printer.h"

void
l_error_handler (void *user_data, const char *err)
{
	fprintf (stderr, "ERROR: %s\n", err);
	exit (-1);
}

static void
print_function (void *user_data, LGlobalNodeType node_type, void *data)
{
	printf ("     ");
	if (node_type == NODE_ASSIGNMENT) {
		l_pretty_print_assignment (user_data, data);
	} else if (node_type == NODE_LAMBDA) {
		l_pretty_print_lambda (user_data, data);
	} else if (node_type == NODE_EXPRESSION) {
		l_pretty_print_tree (user_data, data);
	}
	printf ("\n\n");
}

static void
newline_callback (void *data, int complete)
{
	if (complete)
		printf (" L > ");
	else
		printf (" ... ");
}

int
main (void)
{
	LContext *ctx = l_context_new_from_file (stdin);
	int k;

	printf ("Press Ctrl + D to exit.\n");
	
	ctx->global_notifier_data = l_pretty_printer_new (ctx);
	ctx->error_handler = l_error_handler;
	ctx->global_notifier = print_function;
	ctx->newline_callback = newline_callback;

	k = l_parse_using_context (ctx);

	printf ("Return code = %d\n", k);

	l_destroy_context (ctx);
	return 0;
}
