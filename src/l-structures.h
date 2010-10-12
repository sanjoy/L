#ifndef __STRUCTURES__H
#define __STRUCTURES__H

#include "l-mempool.h"

#include <stdio.h>

typedef struct _LLambda LLambda;

typedef struct {
	char *name;
	int idx;
	LLambda *parent;
} LToken;

typedef struct _LTokenHashtable LTokenHashtable;

struct _LTreeNode {
	LToken *token;
	LLambda *lambda;
	struct _LTreeNode *left, *right;
};

#define L_TREE_NODE_IS_APPLICATION(node) ((node)->token == NULL && (node)->lambda == NULL)

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
	LTreeNode *rhs;
	struct _LAssignment *next;
};

typedef struct _LAssignment LAssignment;

typedef enum {
	NODE_ASSIGNMENT,
	NODE_LAMBDA,
	NODE_EXPRESSION
} LGlobalNodeType;

/*
 * Various functions called by the parser to construct the AST.
 */

LToken *l_token_new (LTokenHashtable *, LMempool *, char *);

LTreeNode *l_tree_cons_tree_tree (LMempool *, LTreeNode *, LTreeNode *);
LTreeNode *l_tree_cons_tree_token (LMempool *, LTreeNode *, LToken *);
LTreeNode *l_tree_cons_tree_lambda (LMempool *, LTreeNode *, LLambda *);

LListNode *l_list_cons (LMempool *, LToken *, LListNode *, void *);

LLambda *l_lambda_new (void *, LListNode *, LTreeNode *);
LAssignment *l_assignment_new_tree (void *, LToken *, LTreeNode *);
LAssignment *l_assignment_new_lambda (void *, LToken *, LLambda *);

void l_global_node_new (void *, LGlobalNodeType, void *);

#endif /* __STRUCTURES__H */
