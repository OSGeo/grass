/* dd.c                                                         */
#include <math.h>
#include "ransurf.h"

double DD(double Dist)
{
    double SmallD, SmallDist;

    if (Dist < Filter.Mult)
	return ((double)1.0);

    SmallD = Filter.MaxDist - Filter.Mult;
    SmallDist = Dist - Filter.Mult;

    return (1.0 - pow((SmallDist / SmallD), Filter.Exp));
}
