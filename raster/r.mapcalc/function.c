
#include <stdio.h>
#include <string.h>

#include <grass/glocale.h>

#include "expression.h"
#include "func_proto.h"

func_desc func_descs[] = {
    {"add", c_varop, f_add},
    {"sub", c_binop, f_sub},
    {"mul", c_varop, f_mul},
    {"div", c_binop, f_div},
    {"mod", c_binop, f_mod},
    {"pow", c_binop, f_pow},

    {"neg", c_unop, f_neg},
    {"abs", c_unop, f_abs},

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

    {"rand", c_binop, f_rand},

    {"null", c_int0, f_null},

    {"col", c_int0, f_col},
    {"row", c_int0, f_row},
    {"depth", c_int0, f_depth},

    {"x", c_double0, f_x},
    {"y", c_double0, f_y},
    {"z", c_double0, f_z},
    {"ewres", c_double0, f_ewres},
    {"nsres", c_double0, f_nsres},
    {"tbres", c_double0, f_tbres},
    {NULL, NULL, NULL}
};

void print_function_names(void)
{
    int i;

    fprintf(stderr, _("Known functions:"));
    for (i = 0; func_descs[i].name; i++)
	fprintf(stderr, "%c%-10s", i % 7 ? ' ' : '\n', func_descs[i].name);
    fprintf(stderr, "\n");
}
