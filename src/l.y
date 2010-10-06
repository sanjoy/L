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

%token TOKEN
%token OTHER

%union {
     char *raw_token;
     LListNode *list;
     LToken *token;	
     LTreeNode *tree;
     LLambda *lambda;
     LAssignment *assignment;
     LUniversalNode *universal;
};

%type <raw_token>  TOKEN
%type <token>      parsed_token
%type <list>       flat_list flat_list_inner
%type <tree>       nested_list nested_list_inner
%type <lambda>     lambda
%type <assignment> assignment
%type <universal>  program

%%

program:
	   lambda program      { l_register_universal_node (context->mempool, NODE_LAMBDA, $1, context); }
     | assignment program  { l_register_universal_node (context->mempool, NODE_ASSIGNMENT, $1, context); }
     |                     { context->roots = NULL; }
     | OTHER
     ;

parsed_token:
               TOKEN { $$ = l_token_new (context->hash_table, context->mempool, $1); }
			 ;

flat_list:
          '(' flat_list_inner ')' { $$ = $2; }
		  ;

flat_list_inner:
                parsed_token flat_list_inner { $$ = l_list_cons (context->mempool, $1, $2); }
              |                              { $$ = 0; }
              ;

nested_list:
              '(' nested_list_inner ')' {$$ = $2; }
            ;

nested_list_inner:
                  nested_list nested_list_inner  { $$ = l_tree_cons_vertical (context->mempool, $1, $2); }
                | parsed_token nested_list_inner { $$ = l_tree_cons_horizontal (context->mempool, $1, $2); }
			    | lambda nested_list_inner       { $$ = l_tree_cons_lambda (context->mempool, $1, $2); }
				|                                { $$ = 0; }
				;

lambda:
       '(' 'L' flat_list nested_list ')' { $$ = l_lambda_new (context->mempool, $3, $4); }
       ;

assignment:
           parsed_token '<' '-' lambda { $$ = l_assignment_new (context->mempool, $1, $4); }
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
	context->roots = NULL;
	context->input_file = file;
	context->hash_table = l_token_hashtable_new (context->mempool, 97);
	return context;
}

LContext *
l_context_new_from_string (char *str, size_t len)
{
	LContext *context = malloc (sizeof (LContext));
	size_t i = 0;
	
	context->mempool = l_mempool_new ();
	context->roots = NULL;
	context->input_file = NULL;
	context->hash_table = l_token_hashtable_new (context->mempool, 97);
	
	context->input_string = l_mempool_alloc (context->mempool, len + 1);
	while (i < len && *str != '\0')
		context->input_string [i++] = *str++;
	context->input_string [i] = '\0';
	return context;
}
