#ifndef __L__PARSER__CONTEXT__H
#define __L__PARSER__CONTEXT__H

#include "l-structures.h"
#include "l-mempool.h"
#include "l-token-hashtable.h"

#include <stdio.h>

typedef void (*LParsingErrorFunc) (int, const char *);
typedef void (*LGlobalNotifier) (void *, LNodeType, void *);

typedef struct {

	void *scanner_data;
	LMempool *mempool;
	LTokenHashtable *hash_table;
	LLambda *global_lambdas;
	LAssignment *global_assignments;

	char *input_string;
	FILE *input_file;

	LParsingErrorFunc error_handler;

	void *global_notifier_data;
	LGlobalNotifier global_notifier ;

} LContext;

#define CALL_GLOBAL_NOTIFIER(ctx,type,data) do { \
		if ((ctx)->global_notifier != NULL) \
			(ctx)->global_notifier ((ctx)->global_notifier_data, type, data); \
	} while (0)

LContext *l_context_new_from_file (FILE *);
LContext *l_context_new_from_string (char *, size_t);

int l_parse_using_context (LContext *);
void l_destroy_context (LContext *);

#endif /* __L__PARSER__CONTEXT__H */
