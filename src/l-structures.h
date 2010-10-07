#ifndef __STRUCTURES__H
#define __STRUCTURES__H

#include "l-mempool.h"

#include <stdio.h>

typedef struct _LLambda LLambda;

typedef struct {
	char *name;
	int idx, non_free_idx;
	LLambda *parent_lambda;
} LToken;

typedef struct _LTokenHashtable LTokenHashtable;

struct _LTreeNode {
	LToken *token;
	LLambda *lambda;
	struct _LTreeNode *right_sibling, *first_child;
};

typedef struct _LTreeNode LTreeNode;

struct _LListNode {
	LToken *token;
	struct _LListNode *next;
};

typedef struct _LListNode LListNode;

struct _LLambda {
	LListNode *args;
	LTreeNode *body;
	struct _LLambda *next;
};

struct _LAssignment {
	LToken *lhs;
	LLambda *rhs;
	struct _LAssignment *next;
};

typedef struct _LAssignment LAssignment;

typedef enum {
	NODE_ASSIGNMENT,
	NODE_LAMBDA
} LNodeType;

LToken *l_token_new (LTokenHashtable *, LMempool *, char *);

LTreeNode *l_tree_cons_horizontal (LMempool *, LToken *, LTreeNode *);
LTreeNode *l_tree_cons_vertical (LMempool *, LTreeNode *, LTreeNode *);
LTreeNode *l_tree_cons_lambda (LMempool *, LLambda *, LTreeNode *);

LListNode *l_list_cons (LMempool *, LToken *, LListNode *);

LLambda *l_lambda_new (LMempool *, LListNode *, LTreeNode *, void *);
LAssignment *l_assignment_new (LMempool *, LToken *, LLambda *);

void l_register_global_node (LMempool *, LNodeType, void *, void *);

#endif /* __STRUCTURES__H */
