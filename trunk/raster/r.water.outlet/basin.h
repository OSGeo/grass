#include <grass/raster.h>
#include "ramseg.h"

#define SHORT int

extern SHORT drain[3][3];
extern int nrows, ncols;
extern char *drain_ptrs;
extern RAMSEG ba_seg, pt_seg;
extern CELL *bas;
