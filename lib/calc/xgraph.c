
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
graph(x, x1,y1, x2,y2, ... xn,yn)  returns y value based on graph
described by the x,y pairs.
****************************************************************/

int c_graph(int argc, int *argt)
{
    int i;

    if (argc < 3)
	return E_ARG_LO;

    if (argc % 2 == 0)
	return E_ARG_NUM;

    for (i = 0; i <= argc; i++)
	argt[i] = DCELL_TYPE;

    return 0;
}

int f_graph(int argc, const int *argt, void **args)
{
    DCELL **argz = (DCELL **) args;
    DCELL *res = argz[0];
    int n = (argc - 1) / 2;
    int i, j;

    if (argc < 3)
	return E_ARG_LO;

    if (argc % 2 == 0)
	return E_ARG_NUM;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    for (i = 1; i <= argc; i++)
	if (argt[i] != DCELL_TYPE)
	    return E_ARG_TYPE;

    for (i = 0; i < columns; i++) {
#define X(j) (argz[2 + 2 * (j) + 0][i])
#define Y(j) (argz[2 + 2 * (j) + 1][i])
#define x (argz[1][i])

	if (IS_NULL_D(&x))
	    goto null;

	for (j = 0; j < n; j++)
	    if (IS_NULL_D(&X(j)))
		goto null;

	for (j = 0; j < n - 1; j++)
	    if (X(j + 1) <= X(j))
		goto null;

	if (x <= X(0)) {
	    if (IS_NULL_D(&Y(0)))
		goto null;
	    res[i] = Y(0);
	    continue;
	}

	if (x >= X(n - 1)) {
	    if (IS_NULL_D(&Y(n - 1)))
		goto null;
	    res[i] = Y(n - 1);
	    continue;
	}

	for (j = 0; j < n - 1; j++) {
	    if (x > X(j + 1))
		continue;

	    if (IS_NULL_D(&Y(j)) || IS_NULL_D(&Y(j + 1)))
		goto null;

	    res[i] =
		Y(j) + (x - X(j)) * (Y(j + 1) - Y(j)) / (X(j + 1) - X(j));

	    break;
	}
#undef X
#undef Y
#undef x

	continue;

      null:
	SET_NULL_D(&res[i]);
    }

    return 0;
}

int f_graph2(int argc, const int *argt, void **args)
{
    DCELL **argz = (DCELL **) args;
    DCELL *res = argz[0];
    int n = (argc - 1) / 2;
    int i, j;

    if (argc < 3)
	return E_ARG_LO;

    if (argc % 2 == 0)
	return E_ARG_NUM;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    for (i = 1; i <= argc; i++)
	if (argt[i] != DCELL_TYPE)
	    return E_ARG_TYPE;

    for (i = 0; i < columns; i++) {
#define X(j) (argz[2 + (j) + 0][i])
#define Y(j) (argz[2 + (j) + n][i])
#define x (argz[1][i])

	if (IS_NULL_D(&x))
	    goto null;

	for (j = 0; j < n; j++)
	    if (IS_NULL_D(&X(j)))
		goto null;

	for (j = 0; j < n - 1; j++)
	    if (X(j + 1) <= X(j))
		goto null;

	if (x <= X(0)) {
	    if (IS_NULL_D(&Y(0)))
		goto null;
	    res[i] = Y(0);
	    continue;
	}

	if (x >= X(n - 1)) {
	    if (IS_NULL_D(&Y(n - 1)))
		goto null;
	    res[i] = Y(n - 1);
	    continue;
	}

	for (j = 0; j < n - 1; j++) {
	    if (x > X(j + 1))
		continue;

	    if (IS_NULL_D(&Y(j)) || IS_NULL_D(&Y(j + 1)))
		goto null;

	    res[i] =
		Y(j) + (x - X(j)) * (Y(j + 1) - Y(j)) / (X(j + 1) - X(j));

	    break;
	}
#undef X
#undef Y
#undef x

	continue;

      null:
	SET_NULL_D(&res[i]);
    }

    return 0;
}

