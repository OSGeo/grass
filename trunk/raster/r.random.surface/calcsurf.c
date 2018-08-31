/* calcsurf.c                                                           */
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "ransurf.h"
#include "local_proto.h"

void CalcSurface(void)
{
    int Count, OutRows, OutCols;
    int Row, Row2, Col, Col2, RanRows, RanCols;
    int owC, oeC, onR, osR, wC, eC, nR, sR;
    double **Randoms;

    G_debug(2, "CalcSurface()");

    OutRows = BigF.RowPlus;
    OutCols = BigF.ColPlus;
    RanRows = (2 * OutRows + Rs);
    RanCols = (2 * OutCols + Cs);
    owC = osR = 0;
    wC = OutCols;
    sR = OutRows;
    oeC = RanCols - 1;
    onR = RanRows - 1;

    if (OutCols > 0)
	eC = RanCols - (OutCols + 1);
    else
	eC = oeC;

    if (OutRows > 0)
	nR = RanRows - (OutRows + 1);
    else
	nR = onR;

    Randoms = (double **)G_malloc(sizeof(double *) * RanRows);
    for (Row = 0; Row < RanRows; Row++)
	Randoms[Row] = (double *)G_malloc(RanCols * sizeof(double));

    /* OLD
       for( Row = OutRows; Row < RanRows; Row++) {
       for( Col = OutCols; Col < OutCols + Cs; Col++) {
       Randoms[ Row][ Col] = GasDev();
       }
       }

       for( Row = OutRows - 1; Row >= 0; Row--) {
       for( Col = OutCols; Col < OutCols + Cs; Col++) {
       Randoms[ Row][ Col] = GasDev();
       }
       }

       for( Row = 0; Row < RanRows; Row++) {
       for( Col = 0; Col < OutCols; Col++)
       Randoms[ Row][ Col] = GasDev();
       for( Col = OutCols + Cs; Col < RanCols; Col++)
       Randoms[ Row][ Col] = GasDev();
       }
       end OLD */

    for (Row = sR; Row <= nR; Row++) {
	for (Col = wC; Col <= eC; Col++) {
	    Randoms[Row][Col] = GasDev();
	}
    }

    Col = wC - 1;
    Col2 = eC + 1;
    while (Col >= 0) {
	for (Row = sR; Row <= nR; Row++) {
	    Randoms[Row][Col] = GasDev();
	    Randoms[Row][Col2] = GasDev();
	}
	Col--;
	Col2++;
    }

    Row = sR - 1;
    Row2 = nR + 1;
    while (Row >= 0) {
	for (Col = 0; Col < RanCols; Col++) {
	    Randoms[Row][Col] = GasDev();
	    Randoms[Row2][Col] = GasDev();
	}
	Row--;
	Row2++;
    }

    Count = 0;
    if (FDM == -1) {
	for (Row = 0; Row < Rs; Row++) {
	    if (ODD(Row)) {
		for (Col = Cs - 1; Col >= 0; Col--) {
		    G_percent(Count++, MapCount, 1);
		    RSurface[Row][Col] =
			MakePP(Row, Col, OutRows, OutCols, Randoms, BigF);
		}
	    }
	    else {
		for (Col = 0; Col < Cs; Col++) {
		    G_percent(Count++, MapCount, 1);
		    RSurface[Row][Col] =
			MakePP(Row, Col, OutRows, OutCols, Randoms, BigF);
		}
	    }
	}
	G_percent(1, 1, 1);
    }
    else {
	for (Row = 0; Row < Rs; Row++) {
	    Rast_get_c_row_nomask(FDM, CellBuffer, Row);
	    if (ODD(Row)) {
		for (Col = Cs - 1; Col >= 0; Col--) {
		    if (CellBuffer[Col] == 0)
			RSurface[Row][Col] = 0.0;
		    else {
			G_percent(Count++, MapCount, 1);
			RSurface[Row][Col] =
			    MakePP(Row, Col, OutRows, OutCols, Randoms, BigF);
		    }
		}
	    }
	    else {
		for (Col = 0; Col < Cs; Col++) {
		    if (CellBuffer[Col] == 0)
			RSurface[Row][Col] = 0.0;
		    else {
			G_percent(Count++, MapCount, 1);
			RSurface[Row][Col] =
			    MakePP(Row, Col, OutRows, OutCols, Randoms, BigF);
		    }
		}
	    }
	}
	G_percent(1, 1, 1);
    }

    G_free(Randoms);
}
