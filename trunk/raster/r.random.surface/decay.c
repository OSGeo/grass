/* decay.c                                                              */
#include "ransurf.h"

void DistDecay(double *Effect, int R, int C)
{
    G_debug(2, "DistDecay");
    G_debug(3, "(R):%d", R);
    G_debug(3, "(C):%d", C);

    *Effect = BigF.F[R + BigF.RowPlus][C + BigF.ColPlus];

}
