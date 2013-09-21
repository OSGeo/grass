#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "list.h"
#include "mapcalc.h"
#include "any.h"

/*
 * This is a backdoor for strange types only. We only handle void pointer
 * references and make no kind of typecheck; lot's of errors can be made
 * by the user. But at least it's possible to deal with minor issues
 * without touching the parser.
 * Memory for "any" should be one single block.
 */

typedef ANY *(*a_func) (void);
typedef ANY *(*a_func_a) (void *a0);
typedef ANY *(*a_func_aa) (void *a0, void *a1);

typedef struct Anyfunc
{
    char *name;
    void *func;
    char *proto;
} ANYFUNC;

static ANY *mkstring(void *a);
void setany(SYMBOL * var, SYMBOL * any);
SYMBOL *mkanyvar(SYMBOL * var, SYMBOL * any);
SYMBOL *anyfunc(SYMBOL * func, SYMBOL * arglist);

static ANYFUNC af[] = {
    {"mkstring", mkstring, "a"},
    {NULL, NULL, NULL}
};

/***************************************************************************
 * Test function only
 */
static ANY *mkstring(void *a)
{
    ANY *str;

    str = (ANY *) listitem(sizeof(ANY));
    str->type = st_str;
    str->any = (char *)strdup(a);
    str->refcnt = 1;
    fprintf(stdout, "\t%s\n", (char *)a);

    return str;
}

/*
 * End test function only
 ***************************************************************************/

void init_any(void)
{
    SYMBOL *sym;
    int i;

    for (i = 0; af[i].name; i++) {
	sym = putsym(af[i].name);
	sym->type = sym->itype = st_afunc;
	sym->v.p = af[i].func;
	sym->proto = af[i].proto;
	sym->rettype = st_any;
    }
}

void setany(SYMBOL * var, SYMBOL * any)
{
    SYMBOL *sym;

    if (var->name) {
	sym = getsym(var->name);
	if (sym) {
	    if (--((ANY *) sym->v.p)->refcnt < 1)
		G_free(sym->v.p);
	    sym->v.p = any->v.p;
	}
    }

    if (--((ANY *) var->v.p)->refcnt < 1)
	G_free(var->v.p);
    var->v.p = NULL;
    freesym(var);

    /* can't show an anyvar */
    any->v.p = NULL;
    freesym(any);
}

SYMBOL *mkanyvar(SYMBOL * var, SYMBOL * any)
{
    var->type = var->itype = st_any;
    var->name = var->v.p;
    var->v.p = any->v.p;
    any->v.p = NULL;
    symtab = (SYMBOL *) listadd((LIST *) symtab, (LIST *) var, cmpsymsym);

    /* can't show any */

    return var;
}

SYMBOL *anyfunc(SYMBOL * func, SYMBOL * arglist)
{
    SYMBOL *sym;
    ANY *res = NULL;
    int argc = -1;

    if (!func || !func->v.p || func->type != st_afunc) {
	parseerror = 1;
	G_warning(_("Can't call bad any-function"));
    }
    else
	argc = listcnt((LIST *) arglist);

    if (argc == 0 && (!func->proto || !*func->proto))
	res = (*(a_func) func->v.p) ();
    else if (argc == 1 && !strcmp(func->proto, "a"))
	res = (*(a_func_a) func->v.p) (arglist->v.p);
    else if (argc == 2 && !strcmp(func->proto, "aa"))
	res = (*(a_func_aa) func->v.p) (arglist->v.p, arglist->next->v.p);
    else {
	G_warning(_("Bad arguments to anyfunc %s (argc = %d)"), func->name,
		  argc);
	parseerror = 1;
    }

    listdelall((LIST *) func, freesym);
    listdelall((LIST *) arglist, freesym);

    sym = (SYMBOL *) listitem(sizeof(SYMBOL));
    sym->type = st_any;
    sym->v.p = res;

    return sym;
}
