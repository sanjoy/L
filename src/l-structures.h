#ifndef __STRUCTURES__H
#define __STRUCTURES__H

#include "l-mempool.h"

#include <stdio.h>

typedef struct {
	char *name;
	int idx;
} LToken;

typedef struct _LLambda LLambda;

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
};

typedef struct {
	LToken *lhs;
	LLambda *rhs;
} LAssignment;

typedef enum {
	NODE_ASSIGNMENT,
	NODE_LAMBDA
} LUniversalNodeType;

struct _LUniversalNode {

	LUniversalNodeType type;

	union {
		LAssignment *assignment;
		LLambda *lambda;
	};
	
	struct _LUniversalNode *next;

};

typedef struct _LUniversalNode LUniversalNode;

LToken *l_token_new (LMempool *, char *);

LTreeNode *l_tree_cons_horizontal (LMempool *, LToken *, LTreeNode *);
LTreeNode *l_tree_cons_vertical (LMempool *, LTreeNode *, LTreeNode *);
LTreeNode *l_tree_cons_lambda (LMempool *, LLambda *, LTreeNode *);

LListNode *l_list_cons (LMempool *, LToken *, LListNode *);

LLambda *l_lambda_new (LMempool *, LListNode *, LTreeNode *);
LAssignment *l_assignment_new (LMempool *, LToken *, LLambda *);

void l_register_universal_node (LMempool *, LUniversalNodeType, void *, void *);

/*
 * These functions are little more than debug tools right now; which means no
 * pretty printing etc.
 */

#define l_print_token(out,token) do { \
		fprintf (out, "%s", (token)->name); \
	} while (0)

void l_print_tree (FILE *, LTreeNode *);
void l_print_list (FILE *, LListNode *);

#define l_print_lambda(out,lambda) do { \
		fprintf (out, "(L \n"); \
		l_print_list (out, (lambda)->args); \
		fprintf (out, "\n"); \
		l_print_tree (out, (lambda)->body); \
		fprintf (out, "\n)\n"); \
	} while (0)

#define l_print_assignment(out, assign) do {	  \
		l_print_token (out, (assign)->lhs); \
		fprintf (out, "\n<- \n"); \
		l_print_lambda (out, (assign)->rhs); \
	} while (0)

#define l_print_universal_node(out, node) do { \
	if ((node)->type == NODE_ASSIGNMENT) \
		l_print_assignment (out, (node)->assignment); \
	else \
		l_print_lambda (out, (node)->lambda); \
	} while (0)

#endif /* __STRUCTURES__H */
