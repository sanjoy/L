%{

#include "l-context.h"
#include "l-parser-tokens.h"

#include <assert.h>
	
#define YY_EXTRA_TYPE LContext *

#define YY_USER_ACTION yylloc->first_line = yylineno;

#define YY_INPUT(buf, result, max_size) do { \
		char c; \
        if (yyextra->input_string) { \
            if (*(yyextra->input_string) == '\0') {\
                result = YY_NULL; \
            } else { \
                buf [0] = *(yyextra->input_string++); \
                result = 1; \
            } \
        } else { \
			int ret;						 \
			if (yyextra->to_print_newline) { \
				L_CALL_NEWLINE_CALLBACK (yyextra); \
				yyextra->to_print_newline = 0; \
			} \
			ret = fgetc (yyextra->input_file); \
			if (ret == -1) { \
				if (yyextra->switch_file_callback != NULL) { \
					if (yyextra->switch_file_callback (yyextra->switch_file_callback_data)) { \
						buf [0] = fgetc (yyextra->input_file); \
						result = 1; \
					} else { \
						result = YY_NULL; \
					} \
				} else { \
					result == YY_NULL; \
				} \
			} else {\
				buf [0] = ret; \
				result = 1; \
				if (buf [0] != '\n') \
					yyextra->chars_since_last_global++; \
			} \
			if (result == 1 && buf [0] == '\n')	\
				yyextra->to_print_newline = 1; \
        } \
	} while (0)

#define ECHO

%}

%option reentrant
%option prefix="l_"
%option bison-bridge
%option bison-locations
%option nounput
%option noinput
	 
%%

[ \t\n]+                        ;

[()=.L;<-\[\]]                  return *yytext;

:[A-Z][a-zA-Z0-9]*              {
                                     yylval->identifier = l_token_new (yyextra->hash_table, yyextra->gc_mempool, yytext);
                                     return IDENTIFIER;
                                }

[a-zA-Z0-9*+]+                  {
                                     yylval->token = l_token_new (yyextra->hash_table, yyextra->gc_mempool, yytext);
                                     return TOKEN;
                                }

<<EOF>>                         return END;

%%

int l_parse (void *);

int
l_wrap (yyscan_t scanner)
{
	return 1;
}

void
l_error (YYLTYPE *loc, LContext *context, const char* err)
{
	L_CALL_ERROR_HANDLER (context, err);
}

int
l_parse_using_context (LContext *context)
{
	L_CALL_NEWLINE_CALLBACK (context);
	l_lex_init_extra (context, &(context->scanner_data));
	return l_parse (context);
}

void
l_destroy_context (LContext *context)
{
	l_lex_destroy (context->scanner_data);
	l_mempool_destroy (context->gc_mempool);
	l_mempool_destroy (context->nogc_mempool);
	free (context);
}
