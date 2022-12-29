
#include <grass/calc.h>

int c_int0(int argc, int *argt)
{
    if (argc > 0)
	return E_ARG_HI;

    argt[0] = CELL_TYPE;

    return 0;
}

int c_double0(int argc, int *argt)
{
    if (argc > 0)
	return E_ARG_HI;

    argt[0] = DCELL_TYPE;

    return 0;
}

int c_double1(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    argt[0] = DCELL_TYPE;
    argt[1] = DCELL_TYPE;

    return 0;
}

int c_double12(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;

    if (argc > 2)
	return E_ARG_HI;

    argt[0] = DCELL_TYPE;
    argt[1] = DCELL_TYPE;
    if (argc == 2)
	argt[2] = DCELL_TYPE;

    return 0;
}

int c_unop(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    argt[0] = argt[1];

    return 0;
}

int c_binop(int argc, int *argt)
{
    if (argc < 2)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    argt[0] = CELL_TYPE;
    if (argt[1] == FCELL_TYPE || argt[2] == FCELL_TYPE)
	argt[0] = FCELL_TYPE;
    if (argt[1] == DCELL_TYPE || argt[2] == DCELL_TYPE)
	argt[0] = DCELL_TYPE;

    argt[1] = argt[0];
    argt[2] = argt[0];

    return 0;
}

int c_varop(int argc, int *argt)
{
    int i;

    if (argc < 1)
	return E_ARG_LO;

    argt[0] = CELL_TYPE;

    for (i = 1; i <= argc; i++)
	if (argt[i] == FCELL_TYPE)
	    argt[0] = FCELL_TYPE;

    for (i = 1; i <= argc; i++)
	if (argt[i] == DCELL_TYPE)
	    argt[0] = DCELL_TYPE;

    for (i = 1; i <= argc; i++)
	argt[i] = argt[0];

    return 0;
}

int c_cmpop(int argc, int *argt)
{
    int arg_type;

    if (argc < 2)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    argt[0] = CELL_TYPE;

    arg_type = CELL_TYPE;
    if (argt[1] == FCELL_TYPE || argt[2] == FCELL_TYPE)
	arg_type = FCELL_TYPE;
    if (argt[1] == DCELL_TYPE || argt[2] == DCELL_TYPE)
	arg_type = DCELL_TYPE;

    argt[1] = arg_type;
    argt[2] = arg_type;

    return 0;
}

int c_logop(int argc, int *argt)
{
    int i;

    if (argc < 1)
	return E_ARG_LO;

    for (i = 1; i <= argc; i++)
	if (argt[i] != CELL_TYPE)
	    return E_ARG_TYPE;

    argt[0] = CELL_TYPE;

    return 0;
}

int c_shiftop(int argc, int *argt)
{
    if (argc < 2)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    if (argt[1] != CELL_TYPE || argt[2] != CELL_TYPE)
	return E_ARG_TYPE;

    argt[0] = CELL_TYPE;

    return 0;
}
