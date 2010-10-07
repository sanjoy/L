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
     char *raw_token;
     char *identifier;
     LListNode *list;
     LToken *token;	
     LTreeNode *tree;
     LLambda *lambda;
     LAssignment *assignment;
};

%type <identifier> IDENTIFIER
%type <raw_token>  TOKEN
%type <token>      parsed_token parsed_identifier
%type <list>       flat_list flat_list_inner
%type <tree>       nested_list nested_list_inner
%type <lambda>     lambda
%type <assignment> assignment

%%

program:
	   program lambda      { l_register_global_node (context->mempool, NODE_LAMBDA, $2, context); }
     | program assignment  { l_register_global_node (context->mempool, NODE_ASSIGNMENT, $2, context); }
     | program END         { return 0; }
     |
     ;

parsed_token:
             TOKEN { $$ = l_token_new (context->hash_table, context->mempool, $1); }
           ;

parsed_identifier:
                    IDENTIFIER { $$ = l_token_new (context->hash_table, context->mempool, $1); }
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
                  nested_list nested_list_inner       { $$ = l_tree_cons_vertical (context->mempool, $1, $2); }
                | parsed_token nested_list_inner      { $$ = l_tree_cons_horizontal (context->mempool, $1, $2); }
                | parsed_identifier nested_list_inner { $$ = l_tree_cons_horizontal (context->mempool, $1, $2); }
                | lambda nested_list_inner            { $$ = l_tree_cons_lambda (context->mempool, $1, $2); }
                |                                     { $$ = NULL; }
				;

lambda:
       '(' 'L' flat_list nested_list ')' { $$ = l_lambda_new (context->mempool, $3, $4, context); }
       ;

assignment:
           parsed_identifier '<' '-' lambda { $$ = l_assignment_new (context->mempool, $1, $4); }
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
