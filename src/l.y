%pure-parser
%name-prefix "l_"
%error-verbose
%defines
%locations

%parse-param { LContext *context }
%lex-param   { void *scanner }

%{

#include "l-structures.h"
#include "l-context.h"
#include "parser.h"

#define scanner (context->scanner_data)

int l_lex (YYSTYPE *, YYLTYPE *, void *);
void l_error (YYLTYPE *, LContext *, const char *);

%}

%token IDENTIFIER "identifier"
%token TOKEN      "token"
%token END 0      "end of file"

%union {
     LToken *token;	
     LToken *identifier;
     LListNode *list;
     LTreeNode *tree;
     LLambda *lambda;
     LAssignment *assignment;
};

%type <identifier> IDENTIFIER
%type <token>      TOKEN
%type <list>       list
%type <tree>       tree
%type <lambda>     lambda
%type <assignment> assignment

%%

program:
	    program lambda ';'            { l_global_node_new (context, NODE_LAMBDA, $2); }
      | program '[' tree ']' ';'      { l_global_node_new (context, NODE_EXPRESSION, $3); }
      | program assignment ';'        { l_global_node_new (context, NODE_ASSIGNMENT, $2); }
      | program IDENTIFIER ';'        { l_global_node_new (context, NODE_IDENTIFIER, $2); }
      |
      ;

list:
     TOKEN list { $$ = l_list_cons (context->gc_mempool, $1, $2, context); }
   |            { $$ = NULL; }
   ;

tree:
     tree TOKEN             { $$ = l_tree_cons_tree_token  (context->gc_mempool, $1, $2); }
   | tree IDENTIFIER        { $$ = l_tree_cons_tree_token  (context->gc_mempool, $1, $2); }
   | tree '(' tree  ')'     { $$ = l_tree_cons_tree_tree   (context->gc_mempool, $1, $3); }
   | tree '(' lambda ')'    { $$ = l_tree_cons_tree_lambda (context->gc_mempool, $1, $3); }
   |                        { $$ = NULL; }
   ;

lambda:
       'L' list '.' tree  { $$ = l_lambda_new (context, $2, $4); }
     ;

assignment:
           IDENTIFIER '=' tree           { $$ = l_assignment_new_tree (context, $1, $3, 0); }
         | IDENTIFIER '=' lambda         { $$ = l_assignment_new_lambda (context, $1, $3, 0); }
         | IDENTIFIER '=' '[' tree ']'   { $$ = l_assignment_new_tree (context, $1, $4, 1); }
         | IDENTIFIER '=' '[' lambda ']' { $$ = l_assignment_new_lambda (context, $1, $4, 1); }
         ;

%%

#include <stdlib.h>
#include "l-mempool.h"

LContext *
l_context_new_from_file (FILE *file)
{
	LContext *context = calloc (sizeof (LContext), 1);
	context->gc_mempool = l_mempool_new ();
	context->nogc_mempool = l_mempool_new ();
	context->input_file = file;
	context->hash_table = l_token_hashtable_new (context->nogc_mempool, 97);
	return context;
}

LContext *
l_context_new_from_string (char *str, size_t len)
{
	LContext *context = calloc (sizeof (LContext), 1);
	size_t i = 0;
	
	context->gc_mempool = l_mempool_new ();
	context->nogc_mempool = l_mempool_new ();
	context->input_file = NULL;
	context->hash_table = l_token_hashtable_new (context->nogc_mempool, 97);
	
	context->input_string = l_mempool_alloc (context->nogc_mempool, len + 1);
	while (i < len && *str != '\0')
		context->input_string [i++] = *str++;
	context->input_string [i] = '\0';
	return context;
}
