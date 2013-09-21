%{
#define _ISOC99_SOURCE			/* isfinite() */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#if 0
#include "list.h"
#endif
#include "mapcalc.h"
#include "yylex.h"
#include "number.h"
#include "vector.h"
#include "map.h"
#include "any.h"

/* should print message */
#define ERR		if (parseerror) { yyerror ("\tfunction error\n"); YYERROR; }
#define NOTZERO(a)	if (!isfinite(a) || a == 0.0) { printf ("\tdivision by zero\n"); YYERROR; }
#define NOTFRAC(a,b)	if (a < 0.0 && b != (int)b) { printf ("\toperation doesn't yield real number\n"); YYERROR; }

%}

%union
{
  double dbl;
  SYMBOL *ptr;
}

%token <dbl> NUM
%token <ptr> STRING
%token <ptr> NUMVAR PNTVAR MAPVAR ANYVAR
%token <ptr> NUMFUNC PNTFUNC MAPFUNC ANYFUNC
%type  <dbl> numexp
%type  <ptr> pntexp mapexp anyexp
%type  <ptr> point pntlst _pntlst
%type  <ptr> arg arglst

%right '='
%left  '-' '+'
%left  '*' '/' '%'
%right '^'
%left  NEG

%%

input:    /* empty */
	| input stmt
	;

stmt:     ';'
	| numexp ';'			{ shownum ($1); }
	| pntexp ';'			{ showvec ($1); }
	| MAPVAR ';'			{ showmap ($1); }
	| NUMVAR '=' numexp ';'		{ setnum ($1, $3); }
	| PNTVAR '=' pntexp ';'		{ setpnt ($1, $3); }
	| MAPVAR '=' mapexp ';'		{ setmap ($1, $3); }
	| ANYVAR '=' anyexp ';'		{ setany ($1, $3); }
	| STRING '=' numexp ';'		{ mknumvar ($1, $3); }
	| STRING '=' pntexp ';'		{ mkpntvar ($1, $3); }
	| STRING '=' mapexp ';'		{ mkmapvar ($1, $3); }
	| STRING '=' anyexp ';'		{ mkanyvar ($1, $3); }
	| error ';'			{ parseerror = 0; }
	;

numexp:	  NUM				{ $$ = $1; }
	| NUMVAR			{ $$ = $1->v.d; delsym ($1); }
	| NUMFUNC '(' arglst ')'	{ $$ = numfunc ($1, $3); ERR; }
	| numexp '+' numexp		{ $$ = $1 + $3; }
	| numexp '-' numexp		{ $$ = $1 - $3; }
	| numexp '*' numexp		{ $$ = $1 * $3; }
	| numexp '/' numexp		{ NOTZERO ($3); $$ = $1 / $3; }
	| '-' numexp %prec NEG		{ $$ = -$2; }
	| numexp '^' numexp		{ NOTFRAC ($1, $3); $$ = pow ($1, $3); }
	| numexp '%' numexp		{ NOTZERO ($3); $$ = fmod ($1, $3); }
	| pntexp '%' pntexp		{ $$ = numop ('%', $1, $3); ERR; }
	| '(' numexp ')'		{ $$ = $2; }
	;

pntexp:   PNTVAR			{ $$ = $1; }
	| point				{ $$ = $1; }
	| pntlst			{ $$ = $1; }
	| PNTFUNC '(' arglst ')'	{ $$ = pntfunc ($1, $3); ERR; }
	| pntexp '+' pntexp		{ $$ = pntop ('+', $1, $3); ERR; }
	| pntexp '-' pntexp		{ $$ = pntop ('-', $1, $3); ERR; }
	| pntexp '*' numexp		{ $$ = pntop ('*', $1, mknum($3)); ERR;}
	| numexp '*' pntexp		{ $$ = pntop ('*', $3, mknum($1)); ERR;}
	| pntexp '/' numexp		{ $$ = pntop ('/', $1, mknum($3)); ERR;}
	| '-' pntexp %prec NEG		{ $$ = pntop ('_', $2, NULL); ERR; }
	| pntexp '^' pntexp		{ $$ = pntop ('^', $1, $3); ERR; }
	| '(' pntexp ')'		{ $$ = $2; }
	;

point:	  '(' numexp ',' numexp ')'	{ $$ = mkpnt ($2, $4, nanval); ERR; }
	| '(' numexp ',' numexp ',' numexp ')' { $$ = mkpnt ($2, $4, $6); ERR; }
	;

pntlst:   '(' _pntlst ')'		{ $$ = $2; }
	;

_pntlst:  pntexp ',' pntexp		{ $$ = $1; }
	| _pntlst ',' pntexp		{ $$ = pntapp ($1, $3); }
	;

mapexp:   MAPVAR			{ $$ = $1; }
	| MAPFUNC '(' arglst ')'	{ $$ = mapfunc ($1, $3); ERR; }
	| mapexp '+' mapexp		{ $$ = mapop ('+', $1, $3); ERR; }
	| mapexp '-' mapexp		{ $$ = mapop ('-', $1, $3); ERR; }
	| mapexp '*' mapexp		{ $$ = mapop ('*', $1, $3); ERR; }
	| mapexp '/' mapexp		{ $$ = mapop ('/', $1, $3); ERR; }
	| '-' mapexp %prec NEG		{ $$ = mapop ('_', $2, NULL); ERR; }
	| mapexp '^' mapexp		{ $$ = mapop ('^', $1, $3); ERR; }
	| mapexp '%' mapexp		{ $$ = mapop ('%', $1, $3); ERR; }
	| '(' mapexp ')'		{ $$ = $2; }
	;

anyexp:   ANYVAR			{ $$ = $1; }
	| ANYFUNC '(' arglst ')'	{ $$ = anyfunc ($1, $3); ERR; }
	;

arglst:  /* empty */			{ $$ = NULL; }
	| arg				{ $$ = $1; }
	| arglst ',' arg		{ $$ = argapp ($1, $3); }
	;

/*
 * The member itype continues the same. After using the argument list,
 * the function must either free the argument if it's not in symtab,
 * or reset type to what is in itype.
 */

arg:	  STRING			{ $$ = $1; $$->type = st_arg; }
	| numexp			{ $$ = mknum ($1); }
	| pntexp			{ $$ = $1; $$->type = st_arg; }
	| mapexp			{ $$ = $1; $$->type = st_arg; }
	| anyexp			{ $$ = $1; $$->type = st_arg; }
	;
