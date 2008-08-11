/* cpfilter.c                                                           */

#undef TRACE
#undef DEBUG

#include "ransurf.h"


void CopyFilter(FILTER * FPtr, FILTER Filter)
{
    FUNCTION(CopyFilter);

    FPtr->Mult = Filter.Mult;
    FPtr->MaxDist = Filter.MaxDist;
    FPtr->MaxSq = Filter.MaxSq;
    FPtr->Exp = Filter.Exp;
}
