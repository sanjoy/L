%pure-parser
%name-prefix "l_"
%error-verbose
%defines
%locations

%parse-param { LContext *context }
%lex-param { void *scanner  }

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
%type <token>  TOKEN
%type <list>       list
%type <tree>       tree
%type <lambda>     lambda
%type <assignment> assignment

%%

program:
	   program lambda      { l_register_global_node (context->mempool, NODE_LAMBDA, $2, context); }
     | program assignment  { l_register_global_node (context->mempool, NODE_ASSIGNMENT, $2, context); }
     | program END         { return 0; }
     |
     ;

list:
       TOKEN flat_list { $$ = l_list_cons (context->mempool, $1, $2); }
     |                        { $$ = NULL; }
     ;

tree:
       '(' tree ')'  { $$ = $2; }
     | tree parsed_token    { $$ = l_tree_cons_vertical (context->mempool, $1, $2); }
     | tree parsed_identifier
     | tree tree
     | tree lambda
     | 
     ;

lambda:
         'L' flat_list '.' nested_list
       ;

assignment:
             IDENTIFIER '=' lambda { $$ = l_assignment_new (context->mempool, $1, $4); }
          ;

%%

#include <stdlib.h>
#include "l-mempool.h"

LContext *
l_context_new_from_file (FILE *file)
{
	LContext *context = malloc (sizeof (LContext));
	context->input_string = NULL;
	context->mempool = l_mempool_new ();
	context->input_file = file;
	context->hash_table = l_token_hashtable_new (context->mempool, 97);
	context->global_notifier = NULL;
	return context;
}

LContext *
l_context_new_from_string (char *str, size_t len)
{
	LContext *context = malloc (sizeof (LContext));
	size_t i = 0;
	
	context->mempool = l_mempool_new ();
	context->input_file = NULL;
	context->hash_table = l_token_hashtable_new (context->mempool, 97);
	
	context->input_string = l_mempool_alloc (context->mempool, len + 1);
	while (i < len && *str != '\0')
		context->input_string [i++] = *str++;
	context->input_string [i] = '\0';
	context->global_notifier = NULL;
	return context;
}
