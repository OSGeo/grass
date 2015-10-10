/* indep.c                                                              */
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "ransurf.h"

void Indep(void)
{
    int Count, DRow, DCol;
    int Found, R, C;
    double RowDist, RowDistSq, ColDist;
    struct History history;

    G_debug(2, "indep()");

    Count = 0;
    Found = 0;

    while (CellCount > 0) {
	G_debug(3, "(CellCount):%d", CellCount);
	G_debug(3, "(Count):%d", Count);

        if (Found >= MaxCellsNum) {
            G_debug(1, "Hit maximum number of cells");
            break;
        }

	DRow = DoNext[Count].R;
	DCol = DoNext[Count++].C;

	if (0 != FlagGet(Cells, DRow, DCol)) {
	    /* FLAG_SET( Out, DRow, DCol); */
	    Out[DRow][DCol] = ++Found;
	    for (R = DRow; R < Rs; R++) {
		RowDist = NS * (R - DRow);
		if (RowDist > MaxDistSq) {
		    R = Rs;
		}
		else {
		    RowDistSq = RowDist * RowDist;
		    for (C = DCol; C < Cs; C++) {
			ColDist = EW * (C - DCol);
			G_debug(3, "(RowDistSq):%.12lf", RowDistSq);
			G_debug(3, "(ColDist):%.12lf", ColDist);
			G_debug(3, "(MaxDistSq):%.12lf", MaxDistSq);
			
			if (MaxDistSq >= RowDistSq + ColDist * ColDist) {
			    if (0 != FlagGet(Cells, R, C)) {
				G_debug(2, "unset()");
				FLAG_UNSET(Cells, R, C);
				CellCount--;
			    }
			}
			else {
			    C = Cs;
			}
		    }
		}
	    }

	    G_debug(2, "it1()");
	    for (R = DRow - 1; R >= 0; R--) {
		RowDist = NS * (DRow - R);
		if (RowDist > MaxDistSq) {
		    R = 0;
		}
		else {
		    RowDistSq = RowDist * RowDist;
		    for (C = DCol; C < Cs; C++) {
			ColDist = EW * (C - DCol);
			if (MaxDistSq >= RowDistSq + ColDist * ColDist) {
			    if (0 != FlagGet(Cells, R, C)) {
				G_debug(2, "unset()");
				FLAG_UNSET(Cells, R, C);
				CellCount--;
			    }
			}
			else {
			    C = Cs;
			}
		    }
		}
	    }

	    G_debug(2, "it2()");
	    for (R = DRow; R < Rs; R++) {
		RowDist = NS * (R - DRow);
		if (RowDist > MaxDistSq) {
		    R = Rs;
		}
		else {
		    RowDistSq = RowDist * RowDist;
		    for (C = DCol - 1; C >= 0; C--) {
			ColDist = EW * (DCol - C);
			if (MaxDistSq >= RowDistSq + ColDist * ColDist) {
			    if (0 != FlagGet(Cells, R, C)) {
				G_debug(2, "unset()");
				FLAG_UNSET(Cells, R, C);
				CellCount--;
			    }
			}
			else {
			    C = 0;
			}
		    }
		}
	    }

	    G_debug(2, "it3()");
	    for (R = DRow - 1; R >= 0; R--) {
		RowDist = NS * (DRow - R);
		if (RowDist > MaxDistSq) {
		    R = 0;
		}
		else {
		    RowDistSq = RowDist * RowDist;
		    for (C = DCol - 1; C >= 0; C--) {
			ColDist = EW * (DCol - C);
			if (MaxDistSq >= RowDistSq + ColDist * ColDist) {
			    if (0 != FlagGet(Cells, R, C)) {
				G_debug(2, "unset()");
				FLAG_UNSET(Cells, R, C);
				CellCount--;
			    }
			}
			else {
			    C = 0;
			}
		    }
		}
	    }
	}
    }

    G_debug(2, "outputting()");
    OutFD = Rast_open_c_new(Output->answer);

    G_message(_("Writing raster map <%s>..."),
	      Output->answer);
    for (R = 0; R < Rs; R++) {
	G_percent(R, Rs, 2);
	for (C = 0; C < Cs; C++) {
	    CellBuffer[C] = Out[R][C];
	    if(CellBuffer[C] == 0)
		Rast_set_null_value(&CellBuffer[C], 1, CELL_TYPE);
	}
	Rast_put_row(OutFD, CellBuffer, CELL_TYPE);
    }
    G_percent(1, 1, 1);
    
    Rast_close(OutFD);
    Rast_short_history(Output->answer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(Output->answer, &history);
}
