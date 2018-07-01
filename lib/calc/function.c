
#include <grass/calc.h>

func_desc calc_func_descs[] = {
    {"add", c_varop, f_add},
    {"sub", c_binop, f_sub},
    {"mul", c_varop, f_mul},
    {"div", c_binop, f_div},
    {"mod", c_binop, f_mod},
    {"pow", c_binop, f_pow},

    {"neg", c_unop, f_neg},
    {"abs", c_unop, f_abs},
    {"ceil", c_unop, f_ceil},
    {"floor", c_unop, f_floor},

    {"gt", c_cmpop, f_gt},
    {"ge", c_cmpop, f_ge},
    {"lt", c_cmpop, f_lt},
    {"le", c_cmpop, f_le},
    {"eq", c_cmpop, f_eq},
    {"ne", c_cmpop, f_ne},

    {"and", c_logop, f_and},
    {"or", c_logop, f_or},

    {"and2", c_logop, f_and2},
    {"or2", c_logop, f_or2},

    {"not", c_not, f_not},

    {"bitand", c_logop, f_bitand},
    {"bitor", c_logop, f_bitor},
    {"xor", c_logop, f_bitxor},

    {"shiftl", c_shiftop, f_shiftl},
    {"shiftr", c_shiftop, f_shiftr},
    {"shiftru", c_shiftop, f_shiftru},

    {"bitnot", c_not, f_bitnot},

    {"sqrt", c_double1, f_sqrt},
    {"sin", c_double1, f_sin},
    {"cos", c_double1, f_cos},
    {"tan", c_double1, f_tan},
    {"acos", c_double1, f_acos},
    {"asin", c_double1, f_asin},

    {"exp", c_double12, f_exp},
    {"log", c_double12, f_log},
    {"atan", c_double12, f_atan},

    {"int", c_int, f_int},
    {"float", c_float, f_float},
    {"double", c_double, f_double},
    {"round", c_round, f_round},

    {"eval", c_eval, f_eval},
    {"if", c_if, f_if},
    {"isnull", c_isnull, f_isnull},

    {"max", c_varop, f_max},
    {"min", c_varop, f_min},
    {"median", c_varop, f_median},
    {"mode", c_varop, f_mode},

    {"nmax", c_varop, f_nmax},
    {"nmin", c_varop, f_nmin},
    {"nmedian", c_varop, f_nmedian},
    {"nmode", c_varop, f_nmode},

    {"graph", c_graph, f_graph},
    {"graph2", c_graph, f_graph2},

    {"rand", c_binop, f_rand},

    {"null", c_int0, f_null},

    {NULL, NULL, NULL}
};

