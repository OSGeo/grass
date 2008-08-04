
#include <grass/gis.h>
#include "globals.h"
#include "expression.h"
#include "func_proto.h"

/****************************************************************
null() null values
****************************************************************/

int f_null(int argc, const int *argt, void **args)
{
    CELL *res = args[0];
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != CELL_TYPE)
	return E_RES_TYPE;

    for (i = 0; i < columns; i++)
	SET_NULL_C(&res[i]);

    return 0;
}
