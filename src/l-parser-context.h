#ifndef __L__PARSER__CONTEXT__H
#define __L__PARSER__CONTEXT__H

#include "l-structures.h"
#include "l-mempool.h"
#include "l-token-hashtable.h"

#include <stdio.h>

typedef void (*LParsingErrorFunc) (int, const char *);

typedef struct {

	void *scanner_data;
	LMempool *mempool;
	LUniversalNode *roots;
	LTokenHashtable *hash_table;

	char *input_string;
	FILE *input_file;

	LParsingErrorFunc error_handler;

} LParserContext;

LParserContext *l_parser_context_new_from_file (FILE *);
LParserContext *l_parser_context_new_from_string (char *, size_t);

int l_parse_using_context (LParserContext *);
void l_destroy_parser_context (LParserContext *);

#endif /* __L__PARSER__CONTEXT__H */
