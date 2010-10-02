#ifndef __L__PARSER__CONTEXT__H
#define __L__PARSER__CONTEXT__H

#include "l-structures.h"
#include "l-mempool.h"

struct _LParserContext {
	void *scanner_data;
	LMempool *mempool;
	LUniversalNode *roots;
};

#endif /* __L__PARSER__CONTEXT__H */
