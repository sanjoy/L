%pure-parser
%name-prefix "l_"
%error-verbose
%defines
%locations

%parse-param { LParserContext *context }
%lex-param { void *scanner  }

%{

#include "l-structures.h"
#include "l-parser-context.h"

#define scanner (context->scanner_data)

%}

%token TOKEN

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
     ;

parsed_token:
             TOKEN { $$ = l_token_new (context->mempool, $1); }
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

