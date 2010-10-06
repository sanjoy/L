#ifndef __L__PRETTY__PRINTER__H
#define __L__PRETTY__PRINTER__H

#include "l-structures.h"
#include "l-parser-context.h"

typedef struct _LPrettyPrinter LPrettyPrinter;

LPrettyPrinter *l_pretty_printer_new (LParserContext *);
void l_pretty_printer_set_output (LPrettyPrinter *, FILE *);
void l_pretty_printer_set_debug_output (LPrettyPrinter *, int);

void l_pretty_print_token (LPrettyPrinter *, LToken *);
void l_pretty_print_tree (LPrettyPrinter *, LTreeNode *);
void l_pretty_print_list (LPrettyPrinter *, LListNode *);
void l_pretty_print_lambda (LPrettyPrinter *, LLambda *);
void l_pretty_print_assignment (LPrettyPrinter *, LAssignment *);

#define l_pretty_print_universal_node(pprinter, node) do { \
	if ((node)->type == NODE_ASSIGNMENT) \
		l_pretty_print_assignment (pprinter, (node)->assignment); \
	else \
		l_pretty_print_lambda (pprinter, (node)->lambda); \
	} while (0)

#endif /* __L__PRETTY__PRINTER__H*/
