
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "mapcalc.h"
#include "func_proto.h"

/****************************************************************************/

static expr_list *variables;

/****************************************************************************/

static func_desc *find_func(const char *name)
{
    int i;

    for (i = 0; local_func_descs[i].name; i++) {
	if (strcmp(name, local_func_descs[i].name) == 0)
	    return &local_func_descs[i];
    }
    for (i = 0; calc_func_descs[i].name; i++) {
	if (strcmp(name, calc_func_descs[i].name) == 0)
	    return &calc_func_descs[i];
    }
    return NULL;
}

static expression *find_variable(const char *name)
{
    expr_list *l;

    for (l = variables; l; l = l->next)
	if (strcmp(name, l->exp->data.bind.var) == 0)
	    return l->exp;
    return NULL;
}

static expression *allocate(int type, int res_type)
{
    expression *e = G_malloc(sizeof(expression));

    e->type = type;
    e->res_type = res_type;
    e->buf = NULL;
    e->worker = NULL;
    return e;
}

/****************************************************************************/

static expression *to_int(expression * e1)
{
    expression *e = allocate(expr_type_function, CELL_TYPE);
    expression **args = G_malloc(2 * sizeof(expression *));
    int *argt = G_malloc(2 * sizeof(int));

    argt[0] = CELL_TYPE;

    args[1] = e1;
    argt[1] = e1->res_type;

    e->data.func.name = "";
    e->data.func.oper = NULL;
    e->data.func.func = f_int;
    e->data.func.argc = 1;
    e->data.func.args = args;
    e->data.func.argt = argt;
    e->data.func.argv = NULL;
    return e;
}

static expression *to_float(expression * e1)
{
    expression *e = allocate(expr_type_function, FCELL_TYPE);
    expression **args = G_malloc(2 * sizeof(expression *));
    int *argt = G_malloc(2 * sizeof(int));

    argt[0] = FCELL_TYPE;

    args[1] = e1;
    argt[1] = e1->res_type;

    e->data.func.name = "";
    e->data.func.oper = NULL;
    e->data.func.func = f_float;
    e->data.func.argc = 1;
    e->data.func.args = args;
    e->data.func.argt = argt;
    e->data.func.argv = NULL;
    return e;
}

static expression *to_double(expression * e1)
{
    expression *e = allocate(expr_type_function, DCELL_TYPE);
    expression **args = G_malloc(2 * sizeof(expression *));
    int *argt = G_malloc(2 * sizeof(int));

    argt[0] = DCELL_TYPE;

    args[1] = e1;
    argt[1] = e1->res_type;

    e->data.func.name = "";
    e->data.func.oper = NULL;
    e->data.func.func = f_double;
    e->data.func.argc = 1;
    e->data.func.args = args;
    e->data.func.argt = argt;
    e->data.func.argv = NULL;
    return e;
}

/****************************************************************************/

int is_var(const char *name)
{
    return find_variable(name) ? 1 : 0;
}

/****************************************************************************/

void define_variable(expression * e)
{
    variables = list(e, variables);
}

char *composite(const char *name, const char *mapset)
{
    char *buf = G_malloc(strlen(name) + strlen(mapset) + 2);

    strcpy(buf, name);
    strcat(buf, "@");
    strcat(buf, mapset);
    return buf;
}

expr_list *list(expression * exp, expr_list * next)
{
    expr_list *l = G_malloc(sizeof(struct expr_list));

    l->exp = exp;
    l->next = next;
    return l;
}

int list_length(expr_list * l)
{
    int n = 0;

    for (; l; l = l->next)
	n++;
    return n;
}

expr_list *singleton(expression * e1)
{
    return list(e1, NULL);
}

expr_list *pair(expression * e1, expression * e2)
{
    return list(e1, list(e2, NULL));
}

expr_list *triple(expression * e1, expression * e2, expression * e3)
{
    return list(e1, list(e2, list(e3, NULL)));
}

expression *constant_int(int x)
{
    expression *e = allocate(expr_type_constant, CELL_TYPE);

    e->data.con.ival = x;
    return e;
}

expression *constant_float(float x)
{
    expression *e = allocate(expr_type_constant, FCELL_TYPE);

    e->data.con.fval = x;
    return e;
}

expression *constant_double(double x)
{
    expression *e = allocate(expr_type_constant, DCELL_TYPE);

    e->data.con.fval = x;
    return e;
}

expression *variable(const char *name)
{
    expression *var = find_variable(name);
    expression *e;

    if (!var)
	syntax_error(_("Undefined variable '%s'"), name);

    e = allocate(expr_type_variable, var ? var->res_type : CELL_TYPE);
    e->data.var.name = name;
    e->data.var.bind = var;
    return e;
}

expression *mapname(const char *name, int mod, int row, int col, int depth)
{
    int res_type = map_type(name, mod);
    expression *e = allocate(expr_type_map,
			     res_type >= 0 ? res_type : CELL_TYPE);

    if (res_type < 0)
	syntax_error(_("Invalid map <%s>"), name);

    e->data.map.name = name;
    e->data.map.mod = mod;
    e->data.map.row = row;
    e->data.map.col = col;
    e->data.map.depth = depth;
    return e;
}

expression *binding(const char *var, expression * val)
{
    expression *e = allocate(expr_type_binding, val->res_type);

    e->data.bind.var = var;
    e->data.bind.val = val;
    e->data.bind.fd = -1;
    return e;
}

expression *operator(const char *name, const char *oper, int prec,
		     expr_list * arglist)
{
    func_desc *d = find_func(name);
    int argc = list_length(arglist);
    expression **args = G_malloc((argc + 1) * sizeof(expression *));
    int *argt = G_malloc((argc + 1) * sizeof(int));
    expression *e;
    expr_list *l;
    int i;

    for (l = arglist, i = 1; l; l = l->next, i++)
	args[i] = l->exp;

    for (i = 1; i <= argc; i++)
	argt[i] = args[i]->res_type;

    argt[0] = CELL_TYPE;

    switch (!d ? -1 : d->check_args(argc, argt)) {
    case -1:
	syntax_error(_("Undefined function '%s'"), name);
	break;
    case 0:
	break;
    case E_ARG_LO:
	syntax_error(_("Too few arguments (%d) to function %s()"),
		     argc, name);
	break;
    case E_ARG_HI:
	syntax_error(_("Too many arguments (%d) to function %s()"),
		     argc, name);
	break;
    case E_ARG_TYPE:
	syntax_error(_("Incorrect argument types to function %s()"), name);
	break;
    default:
	G_fatal_error(_("Internal error for function %s()"), name);
	break;
    }

    for (i = 1; i <= argc; i++)
	if (argt[i] != args[i]->res_type) {
	    if (argt[i] == CELL_TYPE)
		args[i] = to_int(args[i]);
	    if (argt[i] == FCELL_TYPE)
		args[i] = to_float(args[i]);
	    if (argt[i] == DCELL_TYPE)
		args[i] = to_double(args[i]);
	}

    e = allocate(expr_type_function, argt[0]);
    e->data.func.name = name;
    e->data.func.oper = oper;
    e->data.func.prec = prec;
    e->data.func.func = d ? d->func : NULL;
    e->data.func.argc = argc;
    e->data.func.args = args;
    e->data.func.argt = argt;
    e->data.func.argv = NULL;
    return e;
}

expression *function(const char *name, expr_list * arglist)
{
    return operator(name, NULL, 0, arglist);
}

/****************************************************************************/

static char *format_expression_prec(const expression * e, int prec);

/****************************************************************************/

static char *format_constant(const expression * e)
{
    char buff[64];

    if (e->res_type == CELL_TYPE)
	sprintf(buff, "%d", e->data.con.ival);
    else
	sprintf(buff, "%.8g", e->data.con.fval);

    return strdup(buff);
}

static char *format_variable(const expression * e)
{
    return strdup(e->data.var.name);
}

static char *format_map(const expression * e)
{
    char buff[1024];
    const char *mod;

    switch (e->data.map.mod) {
    case 'r':
	mod = "r#";
	break;
    case 'g':
	mod = "g#";
	break;
    case 'b':
	mod = "b#";
	break;
    case '#':
	mod = "#";
	break;
    case '@':
	mod = "@";
	break;
    case 'M':
	mod = "";
	break;
    default:
	G_warning(_("Invalid map modifier: '%c'"), e->data.map.mod);
	mod = "?";
	break;
    }

    if (e->data.map.depth)
	sprintf(buff, "%s%s[%d,%d,%d]",
		mod, e->data.map.name,
		e->data.map.row, e->data.map.col, e->data.map.depth);
    else if (e->data.map.row || e->data.map.col)
	sprintf(buff, "%s%s[%d,%d]",
		mod, e->data.map.name, e->data.map.row, e->data.map.col);
    else
	sprintf(buff, "%s%s", mod, e->data.map.name);

    return strdup(buff);
}

static char *format_function(const expression * e, int prec)
{
    char **args = NULL;
    int num_args = 0;
    char *result;
    int len;
    int i;

    if (e->data.func.argc == 1 && !*e->data.func.name)
	return format_expression_prec(e->data.func.args[1], prec);

    len = strlen(e->data.func.name) + 3;

    for (i = 1; i <= e->data.func.argc; i++) {
	if (i >= num_args) {
	    num_args = i + 1000;
	    args = G_realloc(args, num_args * sizeof(char *));
	}
	args[i] = format_expression_prec(e->data.func.args[i], 9);
	if (i > 1)
	    len += 2;
	len += strlen(args[i]);
    }

    result = G_malloc(len);

    strcpy(result, e->data.func.name);
    strcat(result, "(");
    for (i = 1; i <= e->data.func.argc; i++) {
	if (i > 1)
	    strcat(result, ", ");
	strcat(result, args[i]);
	G_free(args[i]);
    }
    strcat(result, ")");

    return result;
}

static char *format_operator(const expression * e, int prec)
{
    int prec2 = e->data.func.prec;
    char *arg1, *arg2, *arg3;
    char *result;

    switch (e->data.func.argc) {
    case 1:
	arg1 = format_expression_prec(e->data.func.args[1], prec2);
	result = G_malloc(strlen(e->data.func.oper) + strlen(arg1) + 3);
	sprintf(result, "%s%s%s%s",
		prec <= prec2 ? "(" : "",
		e->data.func.oper, arg1, prec <= prec2 ? ")" : "");
	G_free(arg1);
	return result;
    case 2:
	arg1 = format_expression_prec(e->data.func.args[1], (prec2 + 1));
	arg2 = format_expression_prec(e->data.func.args[2], prec2);
	result =
	    G_malloc(strlen(e->data.func.oper) + strlen(arg1) + strlen(arg2) +
		     5);
	sprintf(result, "%s%s %s %s%s", prec <= prec2 ? "(" : "", arg1,
		e->data.func.oper, arg2, prec <= prec2 ? ")" : "");
	G_free(arg1);
	G_free(arg2);
	return result;
    case 3:
	arg1 = format_expression_prec(e->data.func.args[1], prec2);
	arg2 = format_expression_prec(e->data.func.args[2], prec2);
	arg3 = format_expression_prec(e->data.func.args[3], (prec2 + 1));
	result = G_malloc(strlen(arg1) + strlen(arg2) + strlen(arg3) + 9);
	sprintf(result, "%s%s ? %s : %s%s",
		prec <= prec2 ? "(" : "",
		arg1, arg2, arg3, prec <= prec2 ? ")" : "");
	G_free(arg1);
	G_free(arg2);
	G_free(arg3);
	return result;
    default:
	G_warning(_("Illegal number of arguments (%d) for operator '%s'"),
		  e->data.func.argc, e->data.func.oper);
	return format_function(e, prec);
    }
}

static char *format_func_op(const expression * e, int prec)
{
    return (e->data.func.oper ? format_operator : format_function) (e, prec);
}

static char *format_binding(const expression * e, int prec)
{
    char *result;
    char *expr = format_expression_prec(e->data.bind.val, 8);

    result = G_malloc(strlen(e->data.bind.var) + strlen(expr) + 6);

    sprintf(result, "%s%s = %s%s",
	    prec < 8 ? "(" : "", e->data.bind.var, expr, prec < 8 ? ")" : "");

    G_free(expr);

    return result;
}

static char *format_expression_prec(const expression * e, int prec)
{
    switch (e->type) {
    case expr_type_constant:
	return format_constant(e);
    case expr_type_variable:
	return format_variable(e);
    case expr_type_map:
	return format_map(e);
    case expr_type_function:
	return format_func_op(e, prec);
    case expr_type_binding:
	return format_binding(e, prec);
    default:
	G_warning(_("Format_expression_prec: unknown type: %d"), e->type);
	return strdup("??");
    }
}

char *format_expression(const expression * e)
{
    return format_expression_prec(e, 9);
}

/****************************************************************************/
