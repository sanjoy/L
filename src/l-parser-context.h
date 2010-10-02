#ifndef __L__PARSER__CONTEXT__H
#define __L__PARSER__CONTEXT__H

#include "l-structures.h"
#include "l-mempool.h"

#include <stdio.h>

typedef struct {

	void *scanner_data;
	LMempool *mempool;
	LUniversalNode *roots;

	char *input_string;
	FILE *input_file;

} LParserContext;

#endif /* __L__PARSER__CONTEXT__H */
