/* cpfilter.c                                                           */
#include "ransurf.h"

void CopyFilter(FILTER * FPtr, FILTER Filter)
{
    G_debug(2, "CopyFilter()");

    FPtr->Mult = Filter.Mult;
    FPtr->MaxDist = Filter.MaxDist;
    FPtr->MaxSq = Filter.MaxSq;
    FPtr->Exp = Filter.Exp;
}
