/* save.c                                                               */
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "ransurf.h"


void SaveMap(int NumMap, int MapSeed)
{
    int Index, Row, Col, NormIndex;
    int LowColor, HighColor;
    double DownInterval, UpInterval, Value = 0, Ratio, MeanMod;
    struct Categories Cats;
    struct Colors Colr;
    char String[80], Label[240];
    struct History history;
    CELL cat;
    
    G_debug(2, "SaveMap()");

    OutFD = Rast_open_c_new(OutNames[NumMap]);

    MeanMod = 0.0;
    G_debug(3, "(FDM):%d", FDM);
    if (FDM == -1) {
	for (Row = 0; Row < Rs; Row++) {
	    for (Col = 0; Col < Cs; Col++) {
		Value = RSurface[Row][Col];
		MeanMod += Value;
	    }
	}

	MeanMod /= MapCount;
	/* Value = (Value - MeanMod) / FilterSD + MeanMod / FilterSD; */
	Value /= FilterSD;
	DownInterval = UpInterval = Value;
	for (Row = 0; Row < Rs; Row++) {
	    for (Col = 0; Col < Cs; Col++) {
		Value = RSurface[Row][Col];
		/*
		   Value = (Value - MeanMod) / FilterSD
		   + MeanMod / FilterSD;
		 */
		Value /= FilterSD;
		RSurface[Row][Col] = Value;

		if (UpInterval < Value)
		    UpInterval = Value;

		if (DownInterval > Value)
		    DownInterval = Value;
	    }
	}
    }
    else {
	for (Row = 0; Row < Rs; Row++) {
	    Rast_get_c_row_nomask(FDM, CellBuffer, Row);
	    for (Col = 0; Col < Cs; Col++) {
		if (CellBuffer[Col] != 0) {
		    Value = RSurface[Row][Col];
		    MeanMod += Value;
		}
	    }
	}

	MeanMod /= MapCount;
	G_debug(3, "(MeanMod):%.12lf", MeanMod);
	G_debug(3, "(FilterSD):%.12lf", FilterSD);
	/* Value = (Value - MeanMod) / FilterSD + MeanMod / FilterSD; */
	Value /= FilterSD;
	G_debug(3, "(Value):%.12lf", Value);

	DownInterval = UpInterval = Value;

	for (Row = 0; Row < Rs; Row++) {
	    Rast_get_c_row_nomask(FDM, CellBuffer, Row);
	    for (Col = 0; Col < Cs; Col++) {
		if (CellBuffer[Col] != 0) {
		    Value = RSurface[Row][Col];
		    /*
		       Value = (Value - MeanMod) / FilterSD
		       + MeanMod / FilterSD;
		     */
		    Value /= FilterSD;
		    RSurface[Row][Col] = Value;

		    if (UpInterval < Value)
			UpInterval = Value;

		    if (DownInterval > Value)
			DownInterval = Value;
		}
	    }
	}
    }

    G_message(_("Writing raster map <%s>..."), OutNames[NumMap]);

    for (Index = 0; Index < CatInfo.NumCat; Index++) {
	CatInfo.Max[Index] = DownInterval;
	CatInfo.Min[Index] = UpInterval;
	CatInfo.NumValue[Index] = 0;
	CatInfo.Average[Index] = 0.0;
    }

    if (DownInterval == UpInterval)
	UpInterval += .1;

    if (!Uniform->answer) {
	/* normal distribution */
	for (Row = 0; Row < Rs; Row++) {
	    for (Col = 0; Col < Cs; Col++) {
		Value = RSurface[Row][Col];
		if (Value > UpInterval) {
		    Value = UpInterval;
		}
		else if (Value < DownInterval) {
		    Value = DownInterval;
		}

		Ratio = (Value - DownInterval) / (UpInterval - DownInterval);

		/* Ratio in the range of [0..1] */
		Index = (int)((Ratio * CatInfo.NumCat) - .5);
		CatInfo.NumValue[Index]++;
		CatInfo.Average[Index] += Value;

		if (Value > CatInfo.Max[Index])
		    CatInfo.Max[Index] = Value;

		if (Value < CatInfo.Min[Index])
		    CatInfo.Min[Index] = Value;

		RSurface[Row][Col] = 1 + Index;
	    }
	}
    }
    else {
	/* mapping to cumulative normal distribution function */
	for (Row = 0; Row < Rs; Row++) {
	    for (Col = 0; Col < Cs; Col++) {
		Value = RSurface[Row][Col];
		Ratio = (double)(Value - MIN_INTERVAL) /
		    (MAX_INTERVAL - MIN_INTERVAL);
		Index = (int)(Ratio * (SIZE_OF_DISTRIBUTION - 1));
		/* Norm[Index] is always smaller than 1.  */
		NormIndex = (int)(Norm[Index] * CatInfo.NumCat);
		/* record the catogory information */
		CatInfo.NumValue[NormIndex]++;
		CatInfo.Average[NormIndex] += Value;

		if (Value > CatInfo.Max[NormIndex])
		    CatInfo.Max[NormIndex] = Value;

		if (Value < CatInfo.Min[NormIndex])
		    CatInfo.Min[NormIndex] = Value;

		/* NormIndex in range of [0 .. (CatInfo.NumCat-1)] */
		RSurface[Row][Col] = 1 + NormIndex;
	    }
	}
    }

    for (Row = 0; Row < Rs; Row++) {
	G_percent(Row, Rs, 2);
	for (Col = 0; Col < Cs; Col++) {
	    CellBuffer[Col] = (CELL) RSurface[Row][Col];
	}
	Rast_put_row(OutFD, CellBuffer, CELL_TYPE);
    }
    G_percent(1, 1, 1);

    Rast_close(OutFD);
    Rast_short_history(OutNames[NumMap], "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(OutNames[NumMap], &history);

    strcpy(Label, Buf);
    sprintf(String, " seed=%d", MapSeed);
    strcat(Label, String);

    /*
       if( NumMap == 0 && Theory > 0)
       TheoryCovariance( TheoryName, Label);
     */

    Rast_init_cats(Label, &Cats);
    for (Index = 0; Index < CatInfo.NumCat; Index++) {
	if (CatInfo.NumValue[Index] != 0) {
	    CatInfo.Average[Index] /= CatInfo.NumValue[Index];
	    sprintf(Label, "%+lf %+lf to %+lf",
		    CatInfo.Average[Index],
		    CatInfo.Min[Index], CatInfo.Max[Index]);
	    cat = Index + 1;
	    Rast_set_c_cat(&cat, &cat, Label, &Cats);
	}
    }

    Rast_write_cats(OutNames[NumMap], &Cats);
    Rast_init_colors(&Colr);
    LowColor = (int)(127.5 * (CatInfo.Average[0] + 3.5) / 3.5);
    HighColor = (int)(255.0 - 127.5 *
		      (3.5 - CatInfo.Average[CatInfo.NumCat - 1]) / 3.5);

    if (Uniform->answer || LowColor < 0)
	LowColor = 0;
    if (Uniform->answer || HighColor > 255)
	HighColor = 255;
    G_debug(3, "(LowColor):%d", LowColor);
    G_debug(3, "(HighColor):%d", HighColor);

    Rast_add_c_color_rule(&Low, LowColor, LowColor, LowColor,
			  &High, HighColor, HighColor, HighColor, &Colr);

    Rast_write_colors(OutNames[NumMap], G_mapset(), &Colr);
}
