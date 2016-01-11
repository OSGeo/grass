
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>

#include "mapcalc.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

static int syntax_error_occurred;

%}

%union {
	int ival;
	double fval;
	char *sval;
	expression *exp;
	expr_list *list;
}

%token <sval> VARNAME
%token <sval> NAME
%token <sval> VARSTRING
%token <sval> STRING
%token <ival> INTEGER
%token <fval> FLOAT
%token <fval> DOUBLE

%token GT GE LT LE EQ NE LOGAND LOGOR LOGAND2 LOGOR2 BITAND BITOR LSH RSH RSHU

%type <exp> exp
%type <exp> exp_atom
%type <exp> exp_pre
%type <exp> exp_pow
%type <exp> exp_mul
%type <exp> exp_add
%type <exp> exp_sh
%type <exp> exp_cmp
%type <exp> exp_eq
%type <exp> exp_bitand
%type <exp> exp_bitor
%type <exp> exp_logand
%type <exp> exp_logor
%type <exp> exp_cond
%type <exp> exp_let

%type <exp> atom_var
%type <exp> atom_map
%type <exp> atom_func

%type <exp> def

%type <ival> mapmod
%type <ival> index

%type <sval> name
%type <sval> map

%type <list> expr_list

%type <list> defs
%type <list> program

%{

static expr_list *result;

extern int yylex(void);

int yyparse(void);
void yyerror(const char *s);

%}

%%

program		: defs			{ $$ = result = $1;		}
		;

defs		: def			{ $$ = list($1,NULL);		}
		| def ';'		{ $$ = list($1,NULL);		}
		| def ';' defs		{ $$ = list($1,$3);		}
		| error ';' defs	{ $$ = $3;			}
		;

def		: STRING '=' exp	{ $$ = binding($1,$3); define_variable($$);	}
		| NAME '=' exp		{ $$ = binding($1,$3); define_variable($$);	}
		| atom_func
		;

map		: STRING
		| NAME
		| name '@' name		{ $$ = composite($1,$3);	}
		;

mapmod		: '@'			{ $$ = '@';			}
		| 'r'			{ $$ = 'r';			}
		| 'g'			{ $$ = 'g';			}
		| 'b'			{ $$ = 'b';			}
		| '#'			{ $$ = '#';			}
		| 'y'			{ $$ = 'y';			}
		| 'i'			{ $$ = 'i';			}
		;

index		: INTEGER
		| '-' INTEGER		{ $$ = -$2;			}
		;

expr_list	: exp			{ $$ = singleton($1);		}
		| exp ',' expr_list	{ $$ = list($1, $3);		}
		;

atom_var	: VARSTRING		{ $$ = variable($1);		}
		| VARNAME		{ $$ = variable($1);		}
		;

atom_map	: map '[' index ']'	{ $$ = mapname($1,'M',$3,0,0);	}
		| map '[' index ',' index ']'
					{ $$ = mapname($1,'M',$3,$5,0);	}
		| map '[' index ',' index ',' index ']'
					{ $$ = mapname($1,'M',$3,$5,$7);}
		| map			{ $$ = mapname($1,'M',0,0,0);	}
		| mapmod map '[' index ']'
					{ $$ = mapname($2,$1,$4,0,0);	}
		| mapmod map '[' index ',' index ']'
					{ $$ = mapname($2,$1,$4,$6,0);	}
		| mapmod map '[' index ',' index ',' index ']'
					{ $$ = mapname($2,$1,$4,$6,$8);	}
		| mapmod map		{ $$ = mapname($2,$1,0,0,0);	}
		;

atom_func	: name '(' ')'		{ $$ = function($1, NULL);	}
		| name '(' expr_list ')'
					{ $$ = function($1, $3);	}
		;

exp_atom	: '(' exp ')'		{ $$ = $2;			}
		| atom_var
		| atom_map
		| atom_func
		| INTEGER		{ $$ = constant_int($1);	}
		| FLOAT			{ $$ = constant_float($1);	}
		| DOUBLE		{ $$ = constant_double($1);	}
		;

exp_pre		: exp_atom
		| '-' exp_atom		{ $$ = operator("neg","-",1,singleton($2));	}
		| '~' exp_atom		{ $$ = operator("bitnot","~",1,singleton($2));	}
		| '!' exp_atom		{ $$ = operator("not","!",1,singleton($2));	}
		;

exp_pow		: exp_pre
		| exp_pre '^' exp_pow	{ $$ = operator("pow","^",2,pair($1,$3));	}
		;

exp_mul		: exp_pow
		| exp_mul '*' exp_pow	{ $$ = operator("mul","*",3,pair($1,$3));	}
		| exp_mul '/' exp_pow	{ $$ = operator("div","/",3,pair($1,$3));	}
		| exp_mul '%' exp_pow	{ $$ = operator("mod","%",3,pair($1,$3));	}
		;

exp_add		: exp_mul
		| exp_add '+' exp_mul	{ $$ = operator("add","+",4,pair($1,$3));	}
		| exp_add '-' exp_mul	{ $$ = operator("sub","-",4,pair($1,$3));	}
		;

exp_sh		: exp_add
		| exp_sh LSH exp_add	{ $$ = operator("shiftl","<<",5,pair($1,$3));	}
		| exp_sh RSH exp_add	{ $$ = operator("shiftr",">>",5,pair($1,$3));	}
		| exp_sh RSHU exp_add	{ $$ = operator("shiftru",">>>",5,pair($1,$3));	}
		;

exp_cmp		: exp_sh
		| exp_cmp GT exp_sh	{ $$ = operator("gt",">", 6,pair($1,$3));	}
		| exp_cmp GE exp_sh	{ $$ = operator("ge",">=",6,pair($1,$3));	}
		| exp_cmp LT exp_sh	{ $$ = operator("lt","<", 6,pair($1,$3));	}
		| exp_cmp LE exp_sh	{ $$ = operator("le","<=",6,pair($1,$3));	}

exp_eq		: exp_cmp
		| exp_eq EQ exp_cmp	{ $$ = operator("eq","==",7,pair($1,$3));	}
		| exp_eq NE exp_cmp	{ $$ = operator("ne","!=",7,pair($1,$3));	}
		;

exp_bitand	: exp_eq
		| exp_bitand BITAND exp_eq	{ $$ = operator("bitand","&",8,pair($1,$3));	}
		;

exp_bitor	: exp_bitand
		| exp_bitor BITOR exp_bitand	{ $$ = operator("bitor", "|",9,pair($1,$3));	}
		;

exp_logand	: exp_bitor
		| exp_logand LOGAND exp_bitor	{ $$ = operator("and","&&",10,pair($1,$3));	}
		| exp_logand LOGAND2 exp_bitor	{ $$ = operator("and2","&&&",10,pair($1,$3));	}
		;

exp_logor	: exp_logand
		| exp_logor LOGOR exp_logand	{ $$ = operator("or", "||",11,pair($1,$3));	}
		| exp_logor LOGOR2 exp_logand	{ $$ = operator("or2", "|||",11,pair($1,$3));	}
		;

exp_cond	: exp_logor
		| exp_logor '?' exp_cond ':' exp_cond
					{ $$ = operator("if","?:",12,triple($1,$3,$5));	}
		;

exp_let		: exp_cond
		| name '=' exp_let	{ $$ = binding($1,$3); define_variable($$);	}
		;

exp		: exp_let		{ if (syntax_error_occurred) {syntax_error_occurred = 0; YYERROR; } else $$ = $1;	}
		;

name		: NAME
		| VARNAME
		;

%%

void syntax_error(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);

	fprintf(stderr, "\n");

	syntax_error_occurred = 1;
}

void yyerror(const char *s)
{
	fprintf(stderr, "%s\n", s);
	syntax_error_occurred = 0;
}

static expr_list *parse(void)
{
#if 0
	yydebug = 1;
#endif
	syntax_error_occurred = 0;

	if (yyparse() != 0)
	{
		fprintf(stderr, "Parse error\n");
		return NULL;
	}

	if (syntax_error_occurred)
	{
		fprintf(stderr, "Syntax error\n");
		return NULL;
	}

	return result;
}

expr_list *parse_string(const char *s)
{
	initialize_scanner_string(s);
	return parse();
}

expr_list *parse_stream(FILE *fp)
{
	expr_list *e;

	initialize_scanner_stream(fp);
	if (isatty(fileno(fp)))
		fputs("Enter expressions, \"end\" when done.\n", stderr);
	e = parse();
	if (isatty(fileno(fp)))
		fputs("\n", stderr);
	return e;
}

