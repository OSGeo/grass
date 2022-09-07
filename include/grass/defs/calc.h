
#ifndef GRASS_CALCDEFS_H
#define GRASS_CALCDEFS_H

extern void calc_init(int);
extern void pre_exec(void);
extern void post_exec(void);

extern func_t f_add;
extern func_t f_sub;
extern func_t f_mul;
extern func_t f_div;
extern func_t f_mod;
extern func_t f_pow;
extern args_t c_binop;

extern func_t f_neg;
extern func_t f_abs;
extern func_t f_ceil;
extern func_t f_floor;
extern args_t c_unop;

extern func_t f_gt;
extern func_t f_ge;
extern func_t f_lt;
extern func_t f_le;
extern func_t f_eq;
extern func_t f_ne;
extern args_t c_cmpop;

extern func_t f_and;
extern func_t f_or;
extern func_t f_and2;
extern func_t f_or2;
extern func_t f_bitand;
extern func_t f_bitor;
extern func_t f_bitxor;
extern args_t c_logop;

extern func_t f_shiftl;
extern func_t f_shiftr;
extern func_t f_shiftru;
extern args_t c_shiftop;

extern func_t f_not;
extern func_t f_bitnot;
extern args_t c_not;

extern func_t f_sqrt;
extern func_t f_sin;
extern func_t f_cos;
extern func_t f_tan;
extern func_t f_acos;
extern func_t f_asin;
extern args_t c_double1;

extern func_t f_exp;
extern func_t f_log;
extern func_t f_atan;
extern args_t c_double12;

extern func_t f_int;
extern args_t c_int;

extern func_t f_float;
extern args_t c_float;

extern func_t f_double;
extern args_t c_double;

extern func_t f_round;
extern args_t c_round;

extern func_t f_eval;
extern args_t c_eval;

extern func_t f_if;
extern args_t c_if;

extern func_t f_isnull;
extern args_t c_isnull;

extern func_t f_graph;
extern func_t f_graph2;
extern args_t c_graph;

extern func_t f_min;
extern func_t f_max;
extern func_t f_nmin;
extern func_t f_nmax;
extern args_t c_varop;

extern func_t f_median;
extern func_t f_nmedian;
extern args_t c_median;

extern func_t f_mode;
extern func_t f_nmode;
extern args_t c_mode;

extern func_t f_rand;
extern args_t c_binop;

extern func_t f_null;
extern args_t c_int0;

extern args_t c_double0;

#endif
