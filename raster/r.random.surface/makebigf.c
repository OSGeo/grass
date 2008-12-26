/* makebigf.c                                                           */
#include "ransurf.h"
#include "local_proto.h"


void MakeBigF(void)
{
    int R, C;
    double Dist, RDist, CDist;

    G_debug(2, "MakeBigF");

    for (R = 0; R < BigF.NumR; R++) {
	BigF.LowBF[R] = BigF.HihBF[R] = -1;
	RDist = (R - BigF.RowPlus) * NS;
	RDist *= RDist;

	for (C = 0; C < BigF.NumC; C++) {
	    G_debug(3, "(R):%d", R);
	    G_debug(3, "(C):%d", C);
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

	    G_debug(3, "(BigF.F[R][C]):%.12lf", BigF.F[R][C]);
	}

	BigF.LowBF[R] -= BigF.ColPlus;
	BigF.HihBF[R] -= BigF.ColPlus;
    }
}
