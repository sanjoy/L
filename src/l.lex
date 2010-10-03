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
	 
%%

[ \t\n]+                        ;

"L"                             return *yytext;

[()<\-]                         return *yytext;

[a-zA-Z0-9*+]+                  {
                                    yylval->raw_token = yytext;
                                    return TOKEN;
                                }

.

%%

int
l_wrap (yyscan_t scanner)
{
	return 1;
}

void
l_error (YYLTYPE *locp, LParserContext *context, const char* err)
{
	/* TODO Have a function pointer in context */
	printf ("ERR");
}
