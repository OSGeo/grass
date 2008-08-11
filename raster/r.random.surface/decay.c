/* decay.c                                                              */

#undef TRACE
#undef DEBUG

#include "ransurf.h"

void DistDecay(double *Effect, int R, int C)
{
    FUNCTION(DistDecay);
    INT(R);
    INT(C);

    *Effect = BigF.F[R + BigF.RowPlus][C + BigF.ColPlus];
    FUNCTION(end DistDecay);
}
