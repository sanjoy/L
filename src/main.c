#include <stdio.h>

#include "l-parser-context.h"

void
l_error_handler (int line, const char *err)
{
	fprintf (stderr, "Parsing error: %d : %s\n", line, err);
}

int
main (void)
{
	LParserContext *ctx = l_parser_context_new_from_file (stdin);
	ctx->error_handler = l_error_handler;
	l_parse_using_context (ctx);
	l_print_universal_node (stdout, ctx->roots);
	l_destroy_parser_context (ctx);
	return 0;
}
