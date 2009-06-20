
#ifndef __EXPRESSION_H_
#define __EXPRESSION_H_

struct expr_list;

typedef int func_t(int argc, const int *argt, void **args);
typedef int args_t(int argc, int *argt);

#define E_ARG_LO	1
#define E_ARG_HI	2
#define E_ARG_TYPE	3
#define E_RES_TYPE	4
#define E_INV_TYPE	5
#define E_ARG_NUM	6
#define E_WTF		99

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

typedef struct func_desc
{
    const char *name;
    args_t *check_args;
    func_t *func;
} func_desc;

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

extern func_desc func_descs[];

#define IS_NULL_C(x) (Rast_is_c_null_value((x)))
#define IS_NULL_F(x) (Rast_is_f_null_value((x)))
#define IS_NULL_D(x) (Rast_is_d_null_value((x)))

#define SET_NULL_C(x) (Rast_set_c_null_value((x),1))
#define SET_NULL_F(x) (Rast_set_f_null_value((x),1))
#define SET_NULL_D(x) (Rast_set_d_null_value((x),1))

#endif /* __EXPRESSION_H_ */
