/* init.c                                                               */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "ransurf.h"
#include "local_proto.h"


void Init(void)
{
    struct Cell_head Region;
    int row, col, i, j, NumWeight, NumDist, NumExp;
    char *Name, *Number, String[80];
    char msg[128], msg2[64];
    double MinRes;

    G_debug(2, "Init");

    Rs = Rast_window_rows();
    Cs = Rast_window_cols();
    RSurface = (double **)G_malloc(Rs * sizeof(double *));
    for (row = 0; row < Rs; row++)
	RSurface[row] = (double *)G_malloc(Cs * sizeof(double));

    G_get_set_window(&Region);
    EW = Region.ew_res;
    NS = Region.ns_res;

    if (EW < NS)
	MinRes = EW;
    else
	MinRes = NS;

    if (NULL == G_find_file("cell", "MASK", G_mapset())) {
	MapCount = Rs * Cs;
	FDM = -1;
    }
    else {
	FDM = Rast_open_old("MASK", G_mapset());
	{
	    MapCount = 0;
	    CellBuffer = Rast_allocate_c_buf();
	    for (row = 0; row < Rs; row++) {
		Rast_get_c_row_nomask(FDM, CellBuffer, row);
		for (col = 0; col < Cs; col++) {
		    if (CellBuffer[col])
			MapCount++;
		}
	    }
	}
    }

    if (Uniform->answer)
	sprintf(Buf, "Uni. R. S.");
    else
	sprintf(Buf, "Dist. R. S.");

    if (!range_high_stuff->answer)
	High = 255;
    else {
	High = atoi(range_high_stuff->answer);
	sprintf(String, " high=%d", High);
	strcat(Buf, String);
    }

    if (1 >= High)
	G_fatal_error(_("High (%d) must be greater than 1"), High);

    CatInfo.NumCat = High;
    NumMaps = 0;
    OutNames = (char **)G_malloc(sizeof(char *));
    for (i = 0; (Name = Output->answers[i]); i++) {
	for (j = i - 1; j >= 0; j--) {
	    if (strcmp(OutNames[j], Name) == 0)
		G_fatal_error
		    (_("Rastar map <%s> repeated, maps must be unique"),
		     Name);
	}

	OutNames = (char **)G_realloc(OutNames, (i + 1) * sizeof(char *));
	OutNames[i] = (char *)G_malloc(sizeof(char) * (strlen(Name) + 1));
	strcpy(OutNames[i], Name);
	NumMaps++;
    }
    if (NumMaps == 0)
	G_fatal_error(_("Output raster map required"));

    Theory = 0;
    NumSeeds = 0;
    Seeds = (int *)G_malloc(NumMaps * sizeof(int));
    Seed = -1;
    if (!SeedStuff->answers) {
	for (i = 0; i < NumMaps; i++) {
	    Seeds[i] = -1;
	}
    }
    else {
	for (i = 0; (Number = SeedStuff->answers[i]) && i < NumMaps; i++) {
	    sscanf(Number, "%d", &(Seeds[i]));
	}			/* /for */
    }				/* /else */

    CellBuffer = Rast_allocate_c_buf();
    CatInfo.NumValue = (int *)G_malloc(CatInfo.NumCat * sizeof(int));
    CatInfo.Average = (double *)G_malloc(CatInfo.NumCat * sizeof(double));
    CatInfo.Min = (double *)G_malloc(CatInfo.NumCat * sizeof(double));
    CatInfo.Max = (double *)G_malloc(CatInfo.NumCat * sizeof(double));
    NumFilters = 1;
    AllFilters = (FILTER *) G_malloc(sizeof(FILTER));
    /*
       if( ! Distance->answers) {
       NumDist = 1;
       AllFilters[ 0].MaxDist = MinRes / 2.0;
       } else {
       NumDist = 0;
       for (i = 0; Number = Distance->answers[i]; i++) {
       if(NumDist == NumFilters) {
       AllFilters = (FILTER *) G_realloc( AllFilters,
       ++NumFilters * sizeof( FILTER));
       }
       sscanf( Number, "%lf", &(AllFilters[NumDist].MaxDist));
       if (AllFilters[NumDist].MaxDist < 0.0)
       G_fatal_error("%s: distance value[%d]: [%lf] must be >= 0.0",
       G_program_name(), NumDist,
       AllFilters[NumDist].MaxDist);

       NumDist++;
       }
       }
     */
    NumDist = 0;
    if (Distance->answer) {
	sscanf(Distance->answer, "%lf", &(AllFilters[NumDist].MaxDist));
	if (AllFilters[NumDist].MaxDist < 0.0)
	    G_fatal_error(_("Distance value (%d): %lf must be >= 0.0"),
			  NumDist, AllFilters[NumDist].MaxDist);

	NumDist++;
    }
    /*
       NumExp = 0;
       if( Exponent->answers) {
       for (i = 0; Number = Exponent->answers[i]; i++) {
       if(NumExp == NumFilters) 
       AllFilters = (FILTER *) G_realloc(
       AllFilters, ++NumFilters * sizeof( FILTER));
       sscanf( Number, "%lf", &(AllFilters[NumExp].Exp));
       DOUBLE(AllFilters[NumExp].Exp);
       if (AllFilters[NumExp].Exp <= 0.0)
       G_fatal_error("%s: exponent value [%lf] must be > 0.0",
       G_program_name(), AllFilters[NumExp].Exp);

       AllFilters[ NumExp].Exp = 1.0 / AllFilters[ NumExp].Exp;
       NumExp++;
       }
       }
       INT(NumExp);
     */
    NumExp = 0;
    if (Exponent->answer) {
	sscanf(Exponent->answer, "%lf", &(AllFilters[NumExp].Exp));
	if (AllFilters[NumExp].Exp <= 0.0)
	    G_fatal_error(_("Exponent value (%lf) must be > 0.0"),
			  AllFilters[NumExp].Exp);

	NumExp++;
    }
    NumWeight = 0;
    /*
       if( Weight->answers) {
       for (i = 0; Number = Weight->answers[i]; i++) {
       if(NumWeight == NumFilters) 
       AllFilters = (FILTER *) G_realloc(
       AllFilters, ++NumFilters * sizeof( FILTER));
       sscanf( Numbers, "%lf", &(AllFilters[NumWeight].Mult));
       if (AllFilters[NumWeight].Mult == 0.0)
       G_fatal_error("%s: weight value [%lf] must not be 0.0",
       G_program_name(), AllFilters[NumWeight].Mult);

       NumWeight++;
       }
       }
     */
    if (Weight->answer) {
	sscanf(Weight->answer, "%lf", &(AllFilters[NumWeight].Mult));
	if (AllFilters[NumWeight].Mult > AllFilters[NumWeight].MaxDist)
	    G_fatal_error(_("Flat value (%lf) must be less than distance value (%lf)"),
			  AllFilters[NumWeight].Mult,
			  AllFilters[NumWeight].MaxDist);

	NumWeight++;
    }

    if (NumDist > 0) {
	sprintf(String, " dist=");
	strcat(Buf, String);
	for (i = 0; i < NumDist - 1; i++) {
	    sprintf(String, "%.*lf,",
		    Digits(AllFilters[i].MaxDist, 6), AllFilters[i].MaxDist);
	    strcat(Buf, String);
	}
	sprintf(String, "%.*lf",
		Digits(AllFilters[i].MaxDist, 6), AllFilters[i].MaxDist);
	strcat(Buf, String);
    }

    if (NumDist > 1 && NumDist < NumFilters)
	G_fatal_error(_("Must have a distance value for each filter"));

    if (NumDist == 0) {
	AllFilters[0].MaxDist = MinRes / 4.0;
    }

    if (NumDist < NumFilters) {
	for (NumDist = 1; NumDist < NumFilters; NumDist++)
	    AllFilters[NumDist].MaxDist = AllFilters[0].MaxDist;
    }

    for (NumDist = 0; NumDist < NumFilters; NumDist++) {
	if (AllFilters[NumDist].MaxDist < MinRes)
	    AllFilters[NumDist].MaxDist = MinRes * 0.5;
	else
	    AllFilters[NumDist].MaxDist *= 0.5;
    }

    if (NumExp > 0) {
	sprintf(String, " exp=");
	strcat(Buf, String);
	for (i = 0; i < NumExp - 1; i++) {
	    sprintf(String, "%.*lf,",
		    Digits(AllFilters[i].Exp, 6), AllFilters[i].Exp);
	    strcat(Buf, String);
	}
	sprintf(String, "%.*lf",
		Digits(AllFilters[i].Exp, 6), AllFilters[i].Exp);
	strcat(Buf, String);
    }

    if (NumExp > 1 && NumExp < NumFilters)
	G_fatal_error(_("Must have a exponent value for each filter"));

    if (NumWeight > 0) {
	sprintf(String, " flat=");
	strcat(Buf, String);
	for (i = 0; i < NumWeight - 1; i++) {
	    sprintf(String, "%.*lf,",
		    Digits(AllFilters[i].Mult, 6), AllFilters[i].Mult);
	    strcat(Buf, String);
	    G_debug(3, "(AllFilters[i].Mult):%.12lf", AllFilters[i].Mult);
	}
	sprintf(String, "%.*lf",
		Digits(AllFilters[i].Mult, 6), AllFilters[i].Mult);
	strcat(Buf, String);
    }

    if (NumWeight > 1 && NumWeight < NumFilters)
	G_fatal_error(_("Must have a weight value for each filter"));

    if (NumExp == 1) {
	for (NumExp = 1; NumExp < NumFilters; NumExp++)
	    AllFilters[NumExp].Exp = AllFilters[0].Exp;
    }

    if (NumExp == 0) {
	for (NumExp = 0; NumExp < NumFilters; NumExp++)
	    AllFilters[NumExp].Exp = 1.0;
    }

    if (NumWeight == 0) {
	for (NumWeight = 0; NumWeight < NumFilters; NumWeight++)
	    AllFilters[NumWeight].Mult = 0.0;
    }

    AllMaxDist = 0.0;
    for (i = 0; i < NumFilters; i++) {
	if (AllMaxDist < AllFilters[i].MaxDist)
	    AllMaxDist = AllFilters[i].MaxDist;
	AllFilters[i].MaxSq = AllFilters[i].MaxDist * AllFilters[i].MaxDist;
	G_debug(3, "(i):%d", i);
	G_debug(3, "(AllFilters[i].Mult):%.12lf", AllFilters[i].Mult);
	G_debug(3, "(AllFilters[i].MaxDist):%.12lf", AllFilters[i].MaxDist);
	G_debug(3, "(AllFilters[i].MaxSq):%.12lf", AllFilters[i].MaxSq);
	G_debug(3, "(AllFilters[i].Exp):%.12lf", AllFilters[i].Exp);
    }

    BigF.RowPlus = AllMaxDist / NS;
    BigF.ColPlus = AllMaxDist / EW;
    BigF.NumR = BigF.RowPlus * 2 + 1;
    BigF.NumC = BigF.ColPlus * 2 + 1;
    BigF.LowBF = (int *)G_malloc(BigF.NumR * sizeof(int));
    BigF.HihBF = (int *)G_malloc(BigF.NumR * sizeof(int));
    BigF.F = (double **)G_malloc(BigF.NumR * sizeof(double *));
    for (i = 0; i < BigF.NumR; i++)
	BigF.F[i] = (double *)G_malloc(BigF.NumC * sizeof(double));

    AllMaxDist *= 2.0;
}
