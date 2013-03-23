
%{
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/config.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <grass/gis.h>

#include "mapcalc.h"

#include "mapcalc.tab.h"

#ifndef YY_NULL
#define YY_NULL 0
#endif
#ifndef yyterminate
#define yyterminate() return YY_NULL
#endif

static const char *input_string;
static int input_length;
static int input_offset;
static FILE *input_stream;

static int get_input_string(char *buf, int max_size)
{
	const char *next = input_string + input_offset;
	int left = input_length - input_offset;
	int result;

	if (left <= 0)
		return YY_NULL;

	result = (left > max_size) ? max_size : left;
	memcpy(buf, next, result);
	input_offset += result;

	return result;
}

static int get_input_stream(char *buf, int max_size)
{
	if (feof(input_stream))
		return YY_NULL;

#ifdef HAVE_READLINE_READLINE_H
	if (isatty(fileno(input_stream)))
	{
		char *line_read;

		line_read = readline("mapcalc> ");
		if (!line_read)
			return YY_NULL;
		if (strlen(line_read) > max_size - 2)
			G_fatal_error("input line too long");
		strcpy(buf, line_read);
		strcat(buf, "\n");
		free(line_read);

		if (!*buf)
			return YY_NULL;
		add_history(buf);
	}
	else
	{
		if (!fgets(buf, max_size, input_stream))
			return YY_NULL;
	}
#else
	if (isatty(fileno(input_stream)))
		fputs("mapcalc> ", stderr);

	if (!fgets(buf, max_size, input_stream))
		return YY_NULL;
#endif

	return strlen(buf);
}

#define YY_INPUT(buf,result,max_size)				\
{								\
	result = input_string					\
		? get_input_string(buf, max_size)		\
		: get_input_stream(buf, max_size);		\
}

%}

W		[ \t\r]+
N		[^ ^#@,"'\000-\037()\[\]+\-*/%><!=&|?:;~]+
I		[0-9]+
X		[0][xX][0-9a-fA-F]+
E		[eE][-+]?[0-9]+
F		[fF]

%%

{W}		;	/* ignore white space */

"\\"{W}?"\n"	;	/* ignore backslash-newline */

{I}"."{I}?{E}?{F}	|
"."{I}{E}?{F}	{
			yylval.fval = atof(yytext);
			return FLOAT;
		}

{I}"."{I}?{E}?	|
"."{I}{E}?	{
			yylval.fval = atof(yytext);
			return DOUBLE;
		}

{I}"."{I}?[eE]	|
{I}"."{I}?[eE][-+]	|
"."{I}?[eE]	|
"."{I}?[eE][-+]	{
			fprintf(stderr, "unterminated FP constant\n");
			yyterminate();
		}

{I}		{
			yylval.ival = atoi(yytext);
			return INTEGER;
		}

{X}		{
			yylval.ival = (int) strtoul(yytext, NULL, 16);
			return INTEGER;
		}

{N}		{
			yylval.sval = strdup(yytext);
			return is_var(yytext) ? VARNAME : NAME; 
		}

\"[^"]+\"	|
\'[^']+\'	{
			yylval.sval = strdup(yytext + 1);
			yylval.sval[yyleng - 2] = '\0';
			return is_var(yytext) ? VARSTRING : STRING; 
		}

\"[^"]+		|
\'[^']+		{
			fprintf(stderr, "unterminated string\n");
			yyterminate();
		}

"+"		{	return '+';	}
"-"		{	return '-';	}
"*"		{	return '*';	}
"/"		{	return '/';	}
"^"		{	return '^';	}
"%"		{	return '%';	}

"!"		{	return '!';	}
"~"		{	return '~';	}

"&&&"		{	return LOGAND2;	}
"&&"		{	return LOGAND;	}
"&"		{	return BITAND;	}

"|||"		{	return LOGOR2;	}
"||"		{	return LOGOR;	}
"|"		{	return BITOR;	}

"<<"		{	return LSH;	}
">>>"		{	return RSHU;	}
">>"		{	return RSH;	}

">"		{	return GT;	}
">="		{	return GE;	}
"<"		{	return LT;	}
"<="		{	return LE;	}

"=="		{	return EQ;	}
"!="		{	return NE;	}

"?"		{	return '?';	}
":"		{	return ':';	}

[rR]"#"		{	return 'r';	}
[gG]"#"		{	return 'g';	}
[bB]"#"		{	return 'b';	}
"#"		{	return '#';	}
[yY]"#"		{	return 'y';	}
[iI]"#"		{	return 'i';	}

"@"		{	return '@';	}

"("		{	return '(';	}
")"		{	return ')';	}

"["		{	return '[';	}
"]"		{	return ']';	}

"="		{	return '=';	}

","		{	return ',';	}

^{W}*\n			{	yyterminate();	}
^{W}*"end"{W}*\n	{	yyterminate();	}
^{W}*"exit"{W}*\n	{	yyterminate();	}

";"		|
"\n"		{	return ';';	}


.		{
			fprintf(stderr, "syntax error: '%*s'\n", yyleng, yytext);
			yyterminate();
		}
%%

int yywrap(void)
{
	return 1;
}

void initialize_scanner_string(const char *s)
{
	input_string = s;
	input_length = strlen(s);
	input_offset = 0;
}

void initialize_scanner_stream(FILE *fp)
{
	input_stream = fp;
	setbuf(fp, NULL);
}

