%{

#include "l-parser-context.h"
#include "l-parser-tokens.h"

#include <assert.h>
	
#define YY_EXTRA_TYPE LParserContext *

#define YY_USER_ACTION yylloc->first_line = yylineno;

#define YY_INPUT(buf, result, max_size) do {           \
		char c;                                        \
        if (yyextra->input_string) {                   \
            if (*(yyextra->input_string) == '\0')      \
                result = YY_NULL;                      \
            else {                                     \
                buf [0] = *(yyextra->input_string++);  \
                result = 1;                            \
            }                                          \
        } else {                                       \
	        assert (yyextra->input_file);              \
	        if (feof (yyextra->input_file))            \
		        result = YY_NULL;                      \
	        else {                                     \
		        buf [0] = fgetc (yyextra->input_file); \
		        result = 1;                            \
	        }                                          \
        }	                                           \
	} while (0)

%}

%option reentrant
%option prefix="l_"
%option bison-bridge
%option bison-locations
%option nounput
%option noinput
	 
%%

[ \t\n]+                        ;

"L"                             return *yytext;

[()<\-]                         return *yytext;

[a-zA-Z0-9*+]+                  {
                                    yylval->raw_token = yytext;
                                    return TOKEN;
                                }

.                               return ERROR;

%%

int l_parse (void *);

int
l_wrap (yyscan_t scanner)
{
	return 1;
}

void
l_error (YYLTYPE *loc, LParserContext *context, const char* err)
{
	if (context->error_handler != NULL)
		context->error_handler (loc->first_line, err);
}

int
l_parse_using_context (LParserContext *context)
{
	l_lex_init_extra (context, &(context->scanner_data));
	return l_parse (context);
}

void
l_destroy_parser_context (LParserContext *context)
{
	l_lex_destroy (context->scanner_data);
	l_mempool_destroy (context->mempool);
	free (context);
}
