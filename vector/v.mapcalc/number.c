#define _ISOC99_SOURCE		/* to get isfinite() */
#define _BSD_SOURCE		/* to get __USE_MISC */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "list.h"
#include "mapcalc.h"

typedef double (*d_func) (void);
typedef double (*d_func_d) (double d);
typedef double (*d_func_dd) (double d0, double d1);
typedef double (*d_func_di) (double d, int i);
typedef double (*d_func_dai) (double d, int *i);
typedef double (*d_func_p) (void *p0);
typedef double (*d_func_pp) (void *p0, void *p1);
typedef double (*d_func_ppp) (void *p0, void *p1, void *p2);

typedef struct Numfunc
{
    char *name;
    void *func;
    char *proto;
} NUMFUNC;

/*
 * Prototype Types:
 * - all return double or casted to double
 * "d"          (double d)
 * "dd"         (double d, double e)
 * "di"         (double d, int i)
 * "dai"        (double d, int *i)      [a == asterisk]
 *
 * For no argument say NULL or ""
 */

static NUMFUNC nf[] = {
    {"acos", acos, "d"},
    {"asin", asin, "d"},
    {"atan", atan, "d"},
    {"atan2", atan2, "dd"},
    {"cos", cos, "d"},
    {"sin", sin, "d"},
    {"tan", tan, "d"},
    {"cosh", cosh, "d"},
    {"sinh", sinh, "d"},
    {"tanh", tanh, "d"},
    {"acosh", acosh, "d"},
    {"asinh", asinh, "d"},
    {"atanh", atanh, "d"},
    {"exp", exp, "d"},
    {"frexp", frexp, "dai"},
    {"ldexp", ldexp, "di"},
    {"ln", log, "d"},
    {"log10", log10, "d"},
    {"modf", modf, "dai"},
    {"pow", pow, "dd"},
    {"sqrt", sqrt, "d"},
    {"hypot", hypot, "dd"},
    {"cbrt", cbrt, "d"},
    {"ceil", ceil, "d"},
    {"fabs", fabs, "d"},
    {"floor", floor, "d"},
    {"fmod", fmod, "dd"},
    {"drem", drem, "dd"},
    {"j0", j0, "d"},
    {"j1", j1, "d"},
    {"jn", jn, "dd"},
    {"y0", y0, "d"},
    {"y1", y1, "d"},
    {"yn", yn, "dd"},
    {"erf", erf, "d"},
    {"erfc", erfc, "d"},
    {"lgamma", lgamma, "d"},
    {"rint", rint, "d"},
    {NULL, NULL, NULL}
};

void init_num(void);
void shownum(double d);
void setnum(SYMBOL * var, double d);
SYMBOL *mknumvar(SYMBOL * var, double d);
double numfunc(SYMBOL * func, SYMBOL * arglist);
double numop(int op, SYMBOL * opd1, SYMBOL * opd2);
SYMBOL *mknum(double d);

void init_num(void)
{
    SYMBOL *sym;
    int i;

    for (i = 0; nf[i].name; i++) {
	sym = putsym(nf[i].name);
	sym->type = sym->itype = st_nfunc;
	sym->v.p = nf[i].func;
	sym->proto = nf[i].proto;
	sym->rettype = st_num;
    }

    /* add some handy constants */
    sym = putsym("e");
    sym->type = sym->itype = st_num;
    sym->v.d = M_E;

    sym = putsym("pi");
    sym->type = sym->itype = st_num;
    sym->v.d = M_PI;
}

void shownum(double d)
{
    if (!isfinite(d))
	fprintf(stdout, "\t??.??\n");
    else if (d == (int)d)
	fprintf(stdout, "\t%d\n", (int)d);
    else
	fprintf(stdout, "\t%g\n", d);
}

void setnum(SYMBOL * var, double d)
{
    SYMBOL *sym;

    var->v.d = d;
    if (var->name) {
	sym = getsym(var->name);
	if (sym)
	    sym->v.d = d;
    }
    shownum(d);
    freesym(var);
}

SYMBOL *mknumvar(SYMBOL * var, double d)
{
    if (var) {
	var->name = var->v.p;
	var->type = var->itype = st_num;
	var->v.d = d;
	symtab = (SYMBOL *) listadd((LIST *) symtab, (LIST *) var, cmpsymsym);
    }
    else {
	var = (SYMBOL *) listitem(sizeof(SYMBOL));
	var->type = var->itype = st_num;
	var->v.d = d;
    }

    shownum(d);

    return var;
}

double numfunc(SYMBOL * func, SYMBOL * arglist)
{
    double res = 0.0;
    int argc = -1;

    if (!func || !func->v.p || func->type != st_nfunc) {
	parseerror = 1;
	G_warning(_("Can't call bad num-function"));
    }
    else
	argc = listcnt((LIST *) arglist);

    if (argc == 0 && (!func->proto || !*func->proto))
	res = (*(d_func) func->v.p) ();
    else if (argc == 1 && !strcmp(func->proto, "d"))
	res = (*(d_func_d) func->v.p) (arglist->v.d);
    else if (argc == 2 && !strcmp(func->proto, "dd"))
	res = (*(d_func_dd) func->v.p) (arglist->v.d, arglist->next->v.d);
    else if (argc == 2 && !strcmp(func->proto, "di"))
	res = (*(d_func_di) func->v.p) (arglist->v.d,
					(int)arglist->next->v.d);
    else if (argc == 3 && !strcmp(func->proto, "dai")) {
	int iptr;

	iptr = (int)arglist->next->v.d;
	res = (*(d_func_dai) func->v.p) (arglist->v.d, &iptr);
	arglist->next->v.d = (double)iptr;
    }
    else if (argc == 1 && !strcmp(func->proto, "p"))
	res = (*(d_func_p) func->v.p) (arglist->v.p);
    else if (argc == 2 && !strcmp(func->proto, "pp"))
	res = (*(d_func_pp) func->v.p) (arglist->v.p, arglist->next->v.p);
    else if (argc == 3 && !strcmp(func->proto, "ppp"))
	res = (*(d_func_ppp) func->v.p) (arglist->v.p,
					 arglist->next->v.p,
					 arglist->next->next->v.p);
    else {
	G_warning(_("Bad arguments to numfunc %s"), func->name);
	parseerror = 1;
    }

    listdelall((LIST *) func, freesym);
    listdelall((LIST *) arglist, freesym);

    return res;
}

double numop(int op, SYMBOL * opd1, SYMBOL * opd2)
{
    SYMBOL *func, *arglist, *f;
    char buf[32];
    double res = 0.0;

    if (opd1->itype == st_num)
	sprintf(buf, "num_op_func_%c", op);
    else if (opd1->itype == st_pnt)
	sprintf(buf, "pnt_op_func_%c", op);

    func = getsym(buf);
    if (!func) {
	if (opd1->itype == st_num)
	    G_warning(_("No function defined to perform ``number %c number''"),
		      op);
	else if (opd1->itype == st_pnt)
	    G_warning(_("No function defined to perform ``point %c point''"),
		      op);
	parseerror = 1;
    }
    else {
	f = (SYMBOL *) listitem(sizeof(SYMBOL));
	symcpy(f, func);
	f->next = NULL;
	func = f;
	arglist = (SYMBOL *) listapp(NULL, (LIST *) opd1);
	arglist = (SYMBOL *) listapp((LIST *) arglist, (LIST *) opd2);

	res = numfunc(func, arglist);
    }

    return res;
}

SYMBOL *mknum(double d)
{
    SYMBOL *num;

    num = (SYMBOL *) listitem(sizeof(SYMBOL));
    num->type = num->itype = st_num;
    num->v.d = d;

    return num;
}
