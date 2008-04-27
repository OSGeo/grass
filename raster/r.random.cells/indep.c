/* indep.c								*/

#undef TRACE
#undef DEBUG

#undef MAIN
#include <grass/gis.h>
#include "ransurf.h"


void Indep(void)
{
	int	Count, DRow, DCol;
	int	Found, R, C;
	double	RowDist, RowDistSq, ColDist;
	struct  History history;

	FUNCTION(indep);
	Count = 0;
	Found = 0;

	while(CellCount > 0) {
	    INT(CellCount);
	    INT(Count);
	    RETURN;
	    DRow = DoNext[ Count].R;
	    DCol = DoNext[ Count++].C;

	    if( 0 != FlagGet( Cells, DRow, DCol)) {
		/* FLAG_SET( Out, DRow, DCol); */
		Out[ DRow][ DCol] = ++Found;
		for( R = DRow; R < Rs; R++) {
		    RowDist = NS * (R - DRow);
		    if( RowDist > MaxDistSq) {
			R = Rs;
		    } else {
			RowDistSq = RowDist * RowDist;
			for( C = DCol; C < Cs; C++) {
			    ColDist = EW * (C - DCol);
			    DOUBLE(RowDistSq);
			    DOUBLE(ColDist);
			    DOUBLE(MaxDistSq);
			    RETURN;
			    if( MaxDistSq >= RowDistSq + ColDist * ColDist) {
				if( 0 != FlagGet( Cells, R, C)) {
					FUNCTION(unset);
					FLAG_UNSET( Cells, R, C);
					CellCount--;
				}
			    } else {
				C = Cs;
			    }
			}
		    }
		}

		FUNCTION(it1);
                for( R = DRow - 1; R >=0; R--) {
                    RowDist = NS * (DRow - R);
                    if( RowDist > MaxDistSq) {
                        R = 0;
                    } else {
                        RowDistSq = RowDist * RowDist;
                        for( C = DCol; C < Cs; C++) {
                            ColDist = EW * (C - DCol);
                            if( MaxDistSq >= RowDistSq + ColDist * ColDist) {
                                if( 0 != FlagGet( Cells, R, C)) {
					FUNCTION(unset);
                                        FLAG_UNSET( Cells, R, C);
                                        CellCount--;
                                }
                            } else {
				C = Cs;
			    }
                        }
                    }
                }

		FUNCTION(it2);
                for( R = DRow; R < Rs; R++) {
                    RowDist = NS * (R - DRow);
                    if( RowDist > MaxDistSq) {
                        R = Rs;
                    } else {
                        RowDistSq = RowDist * RowDist;
                        for( C = DCol - 1; C >= 0; C--) {
                            ColDist = EW * (DCol - C);
                            if( MaxDistSq >= RowDistSq + ColDist * ColDist) {
                                if( 0 != FlagGet( Cells, R, C)) {
					FUNCTION(unset);
                                        FLAG_UNSET( Cells, R, C);
                                        CellCount--;
                                }
                            } else {
                                C = 0;
                            }
                        }
                    }
                }       

		FUNCTION(it3);
                for( R = DRow - 1; R >=0; R--) {
                    RowDist = NS * (DRow - R);
                    if( RowDist > MaxDistSq) {
                        R = 0;
                    } else {
                        RowDistSq = RowDist * RowDist;
                        for( C = DCol - 1; C >= 0; C--) {
                            ColDist = EW * (DCol - C);
                            if( MaxDistSq >= RowDistSq + ColDist * ColDist) {
                                if( 0 != FlagGet( Cells, R, C)) {
					FUNCTION(unset);
                                        FLAG_UNSET( Cells, R, C);
                                        CellCount--;
                                }
                            } else {
                                C = 0;
                            }
                        }
                    }
                }
	    }
	}

	FUNCTION(outputing);
	OutFD = G_open_cell_new( Output->answer);
        if (OutFD < 0)
                G_fatal_error("%s: unable to open new raster map [%s]",
                        G_program_name(), Output->answer);

        for (R = 0; R < Rs; R++) {
                for (C = 0; C < Cs; C++) {
			CellBuffer[C] = Out[ R][ C];
                }
		G_put_raster_row( OutFD, CellBuffer, CELL_TYPE);
        }
	G_close_cell( OutFD);
	G_short_history(Output->answer, "raster", &history);
	G_command_history(&history);
	G_write_history(Output->answer, &history);
}

