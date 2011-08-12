#include "vizual.h"
#include <grass/raster3d.h>

#define	XDIMYDIM	(Headfax->xdim)*(Headfax->ydim)

int r3read_level(g3map, Headfax, data, n_lev)
     void *g3map;
     file_info *Headfax;
     float *data;
     int n_lev;
{

    Rast3d_get_block(g3map, 0, 0, n_lev,
		 Headfax->xdim, Headfax->ydim, 1, (char *)data, FCELL_TYPE);

    return 0;
}
