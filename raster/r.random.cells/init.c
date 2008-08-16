/* init.c                                                               */

#undef TRACE
#undef DEBUG

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <grass/gis.h>
#include "ransurf.h"
#include "local_proto.h"


/* function prototypes */
static int comp_array(const void *p1, const void *p2);


void Init(int argc, char **argv)
{
    struct Option *SeedStuff;
    struct Cell_head Region;
    int Count;
    int FD, row, col;
    double MinRes;

    FUNCTION(Init);

    Output = G_define_option();
    Output->key = "output";
    Output->type = TYPE_STRING;
    Output->required = YES;
    Output->multiple = NO;
    Output->description = "Name of indepent cells map";
    Output->gisprompt = "new,cell,raster";

    Distance = G_define_option();
    Distance->key = "distance";
    Distance->type = TYPE_DOUBLE;
    Distance->required = YES;
    Distance->multiple = NO;
    Distance->description =
	"Input value: max. distance of spatial correlation (value(s) >= 0.0)";

    SeedStuff = G_define_option();
    SeedStuff->key = "seed";
    SeedStuff->type = TYPE_INTEGER;
    SeedStuff->required = NO;
    SeedStuff->description =
	"Input value: random seed (SEED_MIN >= value >= SEED_MAX), default [random]";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Rs = G_window_rows();
    Cs = G_window_cols();
    G_get_set_window(&Region);
    EW = Region.ew_res;
    NS = Region.ns_res;
    if (EW < NS)
	MinRes = EW;
    else
	MinRes = NS;
    CellBuffer = G_allocate_cell_buf();

    /* Out = FlagCreate( Rs, Cs); */
    Out = (CELL **) G_malloc(sizeof(CELL *) * Rs);
    for (row = 0; row < Rs; row++) {
	Out[row] = G_allocate_cell_buf();
	G_zero_cell_buf(Out[row]);
    }

    Cells = FlagCreate(Rs, Cs);
    CellCount = 0;
    if (NULL != G_find_file("cell", "MASK", G_mapset())) {
	if ((FD = G_open_cell_old("MASK", G_mapset())) < 0) {
	    G_fatal_error(" unable to open MASK");
	}
	else {
	    for (row = 0; row < Rs; row++) {
		G_get_map_row_nomask(FD, CellBuffer, row);
		for (col = 0; col < Cs; col++) {
		    if (CellBuffer[col]) {
			FLAG_SET(Cells, row, col);
			CellCount++;
		    }
		}
	    }
	    G_close_cell(FD);
	}
    }
    else {
	for (row = 0; row < Rs; row++) {
	    for (col = 0; col < Cs; col++) {
		FLAG_SET(Cells, row, col);
	    }
	}
	CellCount = Rs * Cs;
    }

    sscanf(Distance->answer, "%lf", &MaxDist);
    if (MaxDist < 0.0)
	G_fatal_error("distance must be >= 0.0");

    DOUBLE(MaxDist);
    MaxDistSq = MaxDist * MaxDist;
    if (!SeedStuff->answer) {
	Seed = (int)getpid();
    }
    else {
	sscanf(SeedStuff->answer, "%d", &(Seed));
    }

    if (Seed > SEED_MAX) {
	Seed = Seed % SEED_MAX;
    }
    else if (Seed < SEED_MIN) {
	while (Seed < SEED_MIN)
	    Seed += SEED_MAX - SEED_MIN;
    }

    DoNext = (CELLSORTER *) G_malloc(CellCount * sizeof(CELLSORTER));
    Count = 0;
    for (row = 0; row < Rs; row++) {
	for (col = 0; col < Cs; col++) {
	    if (0 != FlagGet(Cells, row, col)) {
		DoNext[Count].R = row;
		DoNext[Count].C = col;
		DoNext[Count].Value = GasDev();
		if (++Count == CellCount) {
		    row = Rs;
		    col = Cs;
		}
	    }
	}
    }
    qsort(DoNext, CellCount, sizeof(CELLSORTER), comp_array);
}


static int comp_array(const void *q1, const void *q2)
{
    const CELLSORTER *p1 = q1;
    const CELLSORTER *p2 = q2;

    if (p1->Value < p2->Value)
	return (-1);
    if (p2->Value < p1->Value)
	return (1);
    return (0);
}

