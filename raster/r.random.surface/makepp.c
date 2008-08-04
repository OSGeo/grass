/* makepp.c                                                             */

#undef TRACE
#undef DEBUG

#undef MAIN
#include "ransurf.h"
#include "local_proto.h"


double MakePP(int Row, int Col, int OutRows, int OutCols,
	      double **Randoms, BIGF BigF)
{
    int DRow, DCol;
    int RRow, RCol;
    double Effect, Value;

    FUNCTION(MakePP);

    Value = 0.0;
    RRow = Row + BigF.RowPlus;
    RCol = Col + BigF.ColPlus;

    for (DRow = RRow - BigF.RowPlus; DRow <= RRow + BigF.RowPlus; DRow++) {
	/* if( BigF.LowBF  this to speed up function */

	for (DCol = RCol - BigF.ColPlus; DCol <= RCol + BigF.ColPlus; DCol++) {
	    DistDecay(&Effect, RRow - DRow, RCol - DCol);
	    INT(RRow - DRow);
	    INT(RCol - DCol);
	    DOUBLE(Effect);
	    RETURN;
	    Value += Effect * Randoms[DRow][DCol];
	}
    }

    return (Value);
    FUNCTION(end MakePP);
}
