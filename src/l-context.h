#ifndef __L__PARSER__CONTEXT__H
#define __L__PARSER__CONTEXT__H

#include "l-structures.h"
#include "l-mempool.h"
#include "l-token-hashtable.h"

#include <stdio.h>

typedef void (*LParsingErrorFunc) (void *, const char *);

/*
 * The global notifier is invoked whenever a new lambda or a new
 * assignment is completely read.
 */
typedef void (*LGlobalNotifier) (void *, LGlobalNodeType, void *);

/* This is called back everytime a '\n' is read. complete is true if
 * an expression was just completed, otherwise it is false.
 */
typedef void (*LNewlineCallback) (void *, int complete);

/* Called when an EOF is encountered.
 */
typedef int (*LSwitchFileCallback) (void *);

/*
 * Hosts one computation environment.
 */
typedef struct {

	void *scanner_data;

	/* Everything is allocated from this memory pool */
	LMempool *mempool;

	LTokenHashtable *hash_table;

	/* The lambdas and assignments declared at global scope. */
	LLambda *global_lambdas;
	LAssignment *global_assignments;

	/* The last expression evaluated in the REPL. */
	LTreeNode *last_expression;

	/* Only one of the following fields may be non NULL, and that
	 * field shall be used as the input.
	 */
	char *input_string;
	FILE *input_file;

	LParsingErrorFunc error_handler;
	void *error_handler_data;

	void *global_notifier_data;
	LGlobalNotifier global_notifier ;

	void *newline_callback_data;
	LNewlineCallback newline_callback;
	int chars_since_last_global, to_print_newline;

	LSwitchFileCallback switch_file_callback;
	void *switch_file_callback_data;

	/* Used by the REPL. */
	void *repl_data;

} LContext;

/*
 * Macros to easily access (call) the global notifier and the error
 * handler.
 */

#define L_CALL_GLOBAL_NOTIFIER(ctx,type,data) do { \
		if ((ctx)->global_notifier != NULL) \
			(ctx)->global_notifier ((ctx)->global_notifier_data,\
			                        type, data); \
	} while (0)

#define L_CALL_ERROR_HANDLER(ctx,err) do { \
		if ((ctx)->error_handler != NULL) \
			(ctx)->error_handler ((ctx)->error_handler_data, err); \
	} while (0)

#define L_CALL_NEWLINE_CALLBACK(ctx) do {	  \
		if ((ctx)->newline_callback) \
			(ctx)->newline_callback ((ctx)->newline_callback_data,\
			                         (ctx)->chars_since_last_global); \
	} while (0)

/*
 * Use these to create a new context.
 */
LContext *l_context_new_from_file (FILE *);
LContext *l_context_new_from_string (char *, size_t);

/*
 * Call this to parse the context created using one of the l_context_new_*
 * functions. Returns 0 if the parsing was successful, or 1 if it was not.
 */
int l_parse_using_context (LContext *);
void l_destroy_context (LContext *);

#endif /* __L__PARSER__CONTEXT__H */
