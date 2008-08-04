/* digits.c                                                             */

#undef TRACE
#undef DEBUG

#undef MAIN
#include <math.h>
#include "ransurf.h"


int Digits(double Double, int MaxSig)
{
    int I, Round;
    double Check, RD, Right;

    FUNCTION(SigDigits);

    I = 0;
    Double += 1.0;
    while (I < MaxSig) {
	Check = Double * pow(10.0, 1.0 * I);
	Round = (int)Check;
	RD = Round;
	Right = fabs(RD - Check);
	if (Right == 0.0)
	    return (I);
	I++;
    }

    return (MaxSig);
}
