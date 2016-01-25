
#ifndef __EXPRESSION_H_
#define __EXPRESSION_H_

#include <grass/calc.h>

struct expr_list;

typedef enum expr_t
{
    expr_type_constant,
    expr_type_variable,
    expr_type_map,
    expr_type_function,
    expr_type_binding
} expr_t;

typedef union expr_data_const
{
    int ival;
    double fval;
} expr_data_const;

typedef struct expr_data_var
{
    const char *name;
    struct expression *bind;
} expr_data_var;

typedef struct expr_data_map
{
    const char *name;
    int mod;
    int row, col, depth;
    int idx;
} expr_data_map;

typedef struct expr_data_func
{
    const char *name;
    const char *oper;
    int prec;
    func_t *func;
    int argc;
    struct expression **args;
    int *argt;
    void **argv;
} expr_data_func;

typedef struct expr_data_bind
{
    const char *var;
    struct expression *val;
    int fd;
} expr_data_bind;

typedef struct expression
{
    int type;
    int res_type;
    void *buf;
    union
    {
	expr_data_const con;
	expr_data_var var;
	expr_data_map map;
	expr_data_func func;
	expr_data_bind bind;
    } data;
    void *worker;
} expression;

typedef struct expr_list
{
    expression *exp;
    struct expr_list *next;
} expr_list;

extern int list_length(expr_list * l);
extern void define_variable(expression * e);
extern char *composite(const char *name, const char *mapset);
extern expr_list *list(expression * exp, expr_list * next);
extern expr_list *singleton(expression * e1);
extern expr_list *pair(expression * e1, expression * e2);
extern expr_list *triple(expression * e1, expression * e2, expression * e3);
extern expression *constant_int(int x);
extern expression *constant_float(float x);
extern expression *constant_double(double x);
extern expression *variable(const char *name);
extern expression *mapname(const char *name, int mod, int row, int col,
			   int depth);
extern expression *operator(const char *name, const char *oper, int prec,
			    expr_list * args);
extern expression *function(const char *name, expr_list * args);
extern expression *binding(const char *var, expression * val);

extern func_desc local_func_descs[];

#endif /* __EXPRESSION_H_ */
