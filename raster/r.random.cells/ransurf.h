/* ransurf.h                                                            */

#include <stdio.h>
#include <math.h>
#include <grass/raster.h>
#include "flag.h"

#define ODD(a)	((a) & 1)

#define PI       		M_PI

#define CELLSORTER struct cell_sorter_
CELLSORTER {
    int R, C;
    double Value;
};

extern double NS, EW;
extern int CellCount, Rs, Cs;
extern double MaxDist, MaxDistSq;
extern FLAG *Cells;
extern CELLSORTER *DoNext;
extern CELL **Out, *CellBuffer;
extern int Seed, OutFD;
extern int MaxCellsNum;
extern struct Flag *Verbose;
extern struct Option *Distance;
extern struct Option *Output;
extern struct Option *SeedStuff;
extern struct Option *MaxCells;
