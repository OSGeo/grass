
#include <stdio.h>
#include <string.h>

#include <grass/glocale.h>

#include "expression.h"
#include "func_proto.h"

func_desc local_func_descs[] = {
    {"col", c_int0, f_col},
    {"row", c_int0, f_row},
    {"depth", c_int0, f_depth},
    {"ncols", c_int0, f_ncols},
    {"nrows", c_int0, f_nrows},
    {"ndepths", c_int0, f_ndepths},

    {"x", c_double0, f_x},
    {"y", c_double0, f_y},
    {"z", c_double0, f_z},

    {"ewres", c_double0, f_ewres},
    {"nsres", c_double0, f_nsres},
    {"tbres", c_double0, f_tbres},

    {"area", c_double0, f_area},

    {NULL, NULL, NULL}
};

void print_function_names(void)
{
    int i;

    fprintf(stderr, _("Known functions:"));
    for (i = 0; calc_func_descs[i].name; i++)
	fprintf(stderr, "%c%-10s", i % 7 ? ' ' : '\n', calc_func_descs[i].name);
    for (i = 0; local_func_descs[i].name; i++)
	fprintf(stderr, "%c%-10s", i % 7 ? ' ' : '\n', local_func_descs[i].name);
    fprintf(stderr, "\n");
}
