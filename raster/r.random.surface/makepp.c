/* makepp.c                                                             */
#include "ransurf.h"
#include "local_proto.h"


double MakePP(int Row, int Col, int OutRows, int OutCols,
	      double **Randoms, BIGF BigF)
{
    int DRow, DCol;
    int RRow, RCol;
    double Effect, Value;

    G_debug(2, "MakePP()");

    Value = 0.0;
    RRow = Row + BigF.RowPlus;
    RCol = Col + BigF.ColPlus;

    for (DRow = RRow - BigF.RowPlus; DRow <= RRow + BigF.RowPlus; DRow++) {
	/* if( BigF.LowBF  this to speed up function */

	for (DCol = RCol - BigF.ColPlus; DCol <= RCol + BigF.ColPlus; DCol++) {
	    DistDecay(&Effect, RRow - DRow, RCol - DCol);
	    G_debug(3, "(RRow - DRow):%d", RRow - DRow);
	    G_debug(3, "(RCol - DCol):%d", RCol - DCol);
	    G_debug(3, "(Effect):%.12lf", Effect);

	    Value += Effect * Randoms[DRow][DCol];
	}
    }

    return (Value);
}
