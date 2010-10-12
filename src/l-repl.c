#include "l-repl.h"
#include "l-pretty-printer.h"

#include <stdio.h>
#include <stdlib.h>

#define WELCOME_MSG "Welcome to L.\nPress Ctrl + D to exit.\n"

typedef struct {
	int currently_reading_file;
	LContext *ctx;
	LPrettyPrinter *pprinter;
} ReplInfo;

static void
error_handler (void *user_data, const char *err)
{
	fprintf (stderr, "ERROR: %s\n", err);
	exit (-1);
}

static void
print_function (void *user_data, LGlobalNodeType node_type, void *data)
{
	ReplInfo *info = user_data;

	if (info->currently_reading_file)
		return;

	printf ("     ");
	if (node_type == NODE_ASSIGNMENT) {
		l_pretty_print_assignment (info->pprinter, data);
	} else if (node_type == NODE_LAMBDA) {
		l_pretty_print_lambda (info->pprinter, data);
	} else if (node_type == NODE_EXPRESSION) {
		l_pretty_print_tree (info->pprinter, data);
	} else if (node_type == NODE_IDENTIFIER) {
		LToken *ident = data;
		LAssignment *iter;
		for (iter = info->ctx->global_assignments; iter; iter = iter->next) {
			if (iter->lhs->idx == ident->idx) {
				l_pretty_print_tree (info->pprinter, iter->rhs);
				break;
			}
		}
		if (iter == NULL)
			printf ("Could not find identifier %s.", ident->name);
	}
	printf ("\n\n");
}

static void
newline_callback (void *data, int newlines_count)
{
	ReplInfo *info = data;

	if (info->currently_reading_file)
		return;

	if (newlines_count == 0)
		printf (" L > ");
	else
		printf (" ... ");
}

static int
switch_file_callback (void *data)
{
	LContext *ctx = data;
	if (ctx->input_file == stdin) {
		return 0;
	} else {
		ReplInfo *info = ctx->repl_data;
		fclose (ctx->input_file);
		ctx->input_file = stdin;
		info->currently_reading_file = 0;
		ctx->newlines_count = 0;
		L_CALL_NEWLINE_CALLBACK (ctx);
		return 1;
	}
}

void l_start_repl (char *file_name)
{
	LContext *ctx;
	FILE *fil = NULL;
	ReplInfo *info;

	if (file_name != NULL)
		fil = fopen (file_name, "r");

	if (fil)
		ctx = l_context_new_from_file (fil);
	else
		ctx = l_context_new_from_file (stdin);

	printf (WELCOME_MSG);

	ctx->error_handler = error_handler;
	ctx->global_notifier = print_function;
	ctx->newline_callback = newline_callback;
	ctx->switch_file_callback = switch_file_callback;
	ctx->switch_file_callback_data = ctx;
	info = l_mempool_alloc (ctx->mempool, sizeof (ReplInfo));
	info->ctx = ctx;
	ctx->repl_data = info;

	if (fil)
		info->currently_reading_file = 1;

	info->pprinter = l_pretty_printer_new (ctx);
	ctx->newline_callback_data = info;
	ctx->global_notifier_data = info;

	l_parse_using_context (ctx);

	l_destroy_context (ctx);
}
