/* makebigf.c                                                           */

#undef TRACE
#undef DEBUG

#undef MAIN
#include "ransurf.h"
#include "local_proto.h"


void MakeBigF(void)
{
    int R, C;
    double Dist, RDist, CDist;

    FUNCTION(MakeBigF);
    for (R = 0; R < BigF.NumR; R++) {
	BigF.LowBF[R] = BigF.HihBF[R] = -1;
	RDist = (R - BigF.RowPlus) * NS;
	RDist *= RDist;

	for (C = 0; C < BigF.NumC; C++) {
	    INT(R);
	    INT(C);
	    CDist = (C - BigF.ColPlus) * EW;
	    CDist *= CDist;
	    Dist = sqrt(CDist + RDist);

	    if (Dist >= Filter.MaxDist) {
		BigF.F[R][C] = 0.0;
		if (BigF.HihBF[R] == -1)
		    BigF.LowBF[R] = C;
	    }
	    else {
		BigF.F[R][C] = DD(Dist);
		BigF.HihBF[R] = C;
	    }

	    DOUBLE(BigF.F[R][C]);
	    RETURN;
	}

	BigF.LowBF[R] -= BigF.ColPlus;
	BigF.HihBF[R] -= BigF.ColPlus;
    }

    FUNCTION(end MakeBigF);
}
