#include "vizual.h"
#include <grass/raster3d.h>

int viz_make_header(hf, dmin, dmax, g3reg)
     file_info *hf;
     double dmin, dmax;
     RASTER3D_Region *g3reg;
{

    hf->min = dmin;
    hf->max = dmax;

    hf->xdim = g3reg->cols;
    hf->ydim = g3reg->rows;

    /*
       hf->ydim = g3reg->cols;
       hf->xdim = g3reg->rows;
       TRIAL: */

    hf->zdim = g3reg->depths;

    hf->north = g3reg->north;
    hf->south = g3reg->south;
    hf->east = g3reg->east;
    hf->west = g3reg->west;
    hf->top = g3reg->top;
    hf->bottom = g3reg->bottom;

    hf->ns_res = (g3reg->north - g3reg->south) / g3reg->rows;
    hf->ew_res = (g3reg->east - g3reg->west) / g3reg->cols;
    hf->tb_res = (g3reg->top - g3reg->bottom) / g3reg->depths;

    /*  not needed here ?
       hf->zone
       hf->proj
       hf->type
     */


}
