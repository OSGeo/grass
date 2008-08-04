/* init.c                                                               */

#undef TRACE
#undef DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#undef MAIN
#include "ransurf.h"
#include "local_proto.h"


/* function prototypes */
static void IsLegal(char *Name);


void Init(int argc, char **argv)
{
    struct Option *Output;

    /*    struct Option     *TheoryS; */
    struct Option *range_high_stuff;
    struct Option *SeedStuff;
    struct Cell_head Region;
    int row, col, i, j, NumWeight, NumDist, NumExp;
    char *Name, *Number, String[80];
    char msg[128], msg2[64];
    double MinRes;

    FUNCTION(Init);

    Output = G_define_option();
    Output->key = "output";
    Output->type = TYPE_STRING;
    Output->required = YES;
    Output->multiple = YES;
    Output->description = "Names of the resulting maps";
    Output->gisprompt = "new,cell,raster";

    /*
       TheoryS              = G_define_option() ;
       TheoryS->key         = "theoretical_correlogram";
       TheoryS->type        = TYPE_STRING;
       TheoryS->required    = NO;
       TheoryS->multiple    = NO;
       TheoryS->description = "Name of the resulting theoretical correlogram map";
       TheoryS->gisprompt  = "new,cell,raster" ;
     */

    Distance = G_define_option();
    Distance->key = "distance";
    Distance->type = TYPE_DOUBLE;
    Distance->required = NO;
    Distance->multiple = NO;
    Distance->description =
	"Input value: max. distance of spatial correlation (value >= 0.0, default [0.0])";

    Exponent = G_define_option();
    Exponent->key = "exponent";
    Exponent->type = TYPE_DOUBLE;
    Exponent->multiple = NO;
    Exponent->required = NO;
    Exponent->description =
	"Input value: distance decay exponent (value > 0.0), default [1.0])";

    Weight = G_define_option();
    Weight->key = "flat";
    Weight->type = TYPE_DOUBLE;
    Weight->multiple = NO;
    Weight->required = NO;
    Weight->description =
	"Input value: distance filter remains flat before beginning exponent, default [0.0]";

    SeedStuff = G_define_option();
    SeedStuff->key = "seed";
    SeedStuff->type = TYPE_INTEGER;
    SeedStuff->required = NO;
    SeedStuff->description =
	"Input value: random seed (SEED_MIN >= value >= SEED_MAX), default [random]";

    range_high_stuff = G_define_option();
    range_high_stuff->key = "high";
    range_high_stuff->type = TYPE_INTEGER;
    range_high_stuff->required = NO;
    range_high_stuff->description =
	"Input value: maximum cell value of distribution, default [255]";

    Uniform = G_define_flag();
    Uniform->key = 'u';
    Uniform->description = "Uniformly distributed cell values";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Rs = G_window_rows();
    Cs = G_window_cols();
    Surface = (double **)G_malloc(Rs * sizeof(double *));
    for (row = 0; row < Rs; row++)
	Surface[row] = (double *)G_malloc(Cs * sizeof(double));

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
	if ((FDM = G_open_cell_old("MASK", G_mapset())) < 0) {
	    G_fatal_error(" unable to open MASK");
	}
	else {
	    MapCount = 0;
	    CellBuffer = G_allocate_cell_buf();
	    for (row = 0; row < Rs; row++) {
		G_get_map_row_nomask(FDM, CellBuffer, row);
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
	G_fatal_error("high [%d] must be greater than 1", High);

    CatInfo.NumCat = High;
    NumMaps = 0;
    OutNames = (char **)G_malloc(sizeof(char *));
    for (i = 0; (Name = Output->answers[i]); i++) {
	IsLegal(Name);
	for (j = i - 1; j >= 0; j--) {
	    if (strcmp(OutNames[j], Name) == 0)
		G_fatal_error
		    ("%s: Random map [%s] repeated, maps must be unique",
		     G_program_name(), Name);
	}

	OutNames = (char **)G_realloc(OutNames, (i + 1) * sizeof(char *));
	OutNames[i] = (char *)G_malloc(sizeof(char) * (strlen(Name) + 1));
	strcpy(OutNames[i], Name);
	NumMaps++;
    }
    if (NumMaps == 0)
	G_fatal_error("%s: requires an output map", G_program_name());

    Theory = 0;
    /*
       TheoryName = TheoryS->answer;
       if (G_legal_filename (TheoryName) != -1) {
       Theory++;
       }
     */
    NumSeeds = 0;
    Seeds = (int *)G_malloc(NumMaps * sizeof(int));
    if (!SeedStuff->answers) {
	for (i = 0; i < NumMaps; i++) {
	    Seeds[i] = SEED_MIN - 1;
	    Seed = (int)getpid();
	}
    }
    else {
	for (i = 0; (Number = SeedStuff->answers[i]) && i < NumMaps; i++) {
	    sscanf(Number, "%d", &(Seeds[i]));
	    if (Seeds[i] > SEED_MAX) {

		sprintf(msg, _("Seed [%d] larger than maximum [%d]"),
			Seeds[i], SEED_MAX);
		Seeds[i] = Seeds[i] % SEED_MAX;
		sprintf(msg2, _(" seed is set to %d"), Seeds[i]);
		strcat(msg, msg2);
		G_warning(msg);
	    }
	    else if (Seeds[i] < SEED_MIN) {
		sprintf(msg, _("Seed [%d] smaller than minimum [%d]"),
			Seeds[i], SEED_MIN);
		while (Seeds[i] < SEED_MIN)
		    Seeds[i] += SEED_MAX - SEED_MIN;
		sprintf(msg2, _(" seed is set to %d"), Seeds[i]);
		strcat(msg, msg2);
		G_warning(msg);
	    }
	}			/* /for */
    }				/* /else */

    CellBuffer = G_allocate_cell_buf();
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
	    G_fatal_error("%s: distance value[%d]: [%lf] must be >= 0.0",
			  G_program_name(), NumDist,
			  AllFilters[NumDist].MaxDist);

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
	    G_fatal_error("%s: exponent value [%lf] must be > 0.0",
			  G_program_name(), AllFilters[NumExp].Exp);

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
	    G_fatal_error
		("%s: flat value [%lf] must be less than distance value [%lf]",
		 G_program_name(), AllFilters[NumWeight].Mult,
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
	G_fatal_error("%s: must have a distance value for each filter",
		      G_program_name());

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
	G_fatal_error("%s: must have a exponent value for each filter",
		      G_program_name());

    if (NumWeight > 0) {
	sprintf(String, " flat=");
	strcat(Buf, String);
	for (i = 0; i < NumWeight - 1; i++) {
	    sprintf(String, "%.*lf,",
		    Digits(AllFilters[i].Mult, 6), AllFilters[i].Mult);
	    strcat(Buf, String);
	    DOUBLE(AllFilters[i].Mult);
	}
	sprintf(String, "%.*lf",
		Digits(AllFilters[i].Mult, 6), AllFilters[i].Mult);
	strcat(Buf, String);
    }

    if (NumWeight > 1 && NumWeight < NumFilters)
	G_fatal_error("%s: must have a weight value for each filter",
		      G_program_name());

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
    RETURN;

    AllMaxDist = 0.0;
    for (i = 0; i < NumFilters; i++) {
	if (AllMaxDist < AllFilters[i].MaxDist)
	    AllMaxDist = AllFilters[i].MaxDist;
	AllFilters[i].MaxSq = AllFilters[i].MaxDist * AllFilters[i].MaxDist;
	INT(i);
	DOUBLE(AllFilters[i].Mult);
	DOUBLE(AllFilters[i].MaxDist);
	RETURN;
	DOUBLE(AllFilters[i].MaxSq);
	DOUBLE(AllFilters[i].Exp);
	RETURN;
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


static void IsLegal(char *Name)
{
    if (G_legal_filename(Name) == -1)
	G_fatal_error("%s: map name [%s] not legal for GRASS",
		      G_program_name(), Name);
}
