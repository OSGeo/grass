#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <grass/gis.h>
#include "list.h"
#include "mapcalc.h"
#include "yylex.h"
#include "v.mapcalc.tab.h"

/*
 * missing features (some might go to bison):
 * - quit/exit/bye
 * - load script
 * - set plugin path
 * - describe a function/variable
 * - get description for function/variable (help)
 * - save current state in a script to be reproducible (vars, descr...)
 */

int yylex(void)
{
    int c;

    while ((c = getchar()) == ' ' || c == '\t' || c == '\n') ;

    if (c == EOF)
	return 0;

    if (c == '.' || isdigit(c)) {
	ungetc(c, stdin);
	scanf("%lf", &yylval.dbl);
	return NUM;
    }

    if (isalpha(c) || c == '_') {
	SYMBOL *s, *sym;
	static char *symbuf = 0;
	static int length = 0;
	int i;

	if (length == 0) {
	    length = 40;
	    symbuf = (char *)G_malloc(length + 1);
	}

	i = 0;
	do {
	    if (i == length) {
		length *= 2;
		symbuf = (char *)G_realloc(symbuf, length + 1);
	    }
	    symbuf[i++] = c;
	    c = getchar();
	} while (c != EOF && (isalnum(c) || c == '_'));

	ungetc(c, stdin);
	symbuf[i] = '\0';

	/*
	 * 1. Check, if it's a known symbol. If so, we know the type.
	 * 2. Check, if it might be an external function. If so load
	 * 3. Check, if there is a map with this name
	 * 4. return as string.
	 */
	sym = (SYMBOL *) listitem(sizeof(SYMBOL));
	s = getsym(symbuf);
	if (!s) {
	    sym->v.p = strdup(symbuf);
	    sym->type = sym->itype = st_str;
	    yylval.ptr = sym;
	    return STRING;
	}
	else {
	    symcpy(sym, s);
	    s = sym;
	}

	yylval.ptr = s;
	switch (s->type) {
	case st_map:
	    return MAPVAR;
	case st_mfunc:
	    return MAPFUNC;
	case st_pnt:
	    return PNTVAR;
	case st_any:
	    return ANYVAR;
	case st_num:
	    return NUMVAR;
	case st_nfunc:
	    return NUMFUNC;
	case st_afunc:
	    return ANYFUNC;
	case st_pfunc:
	    return PNTFUNC;
	default:
	    G_fatal_error("Insert more translations here (%d)", s->type);
	}
    }

    return c;

}

void yyerror(const char *msg)
{
    fprintf(stdout, "%s\n", msg);
}
