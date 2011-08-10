
/* the caps are being built by reading thru the original grid3 file 
 ** and drawn to the screen not being stored at this time
 */

#include <grass/gis.h>
#include "vizual.h"
#include <grass/raster3d.h>
/*
   #define DEBUG1
 */

int init_caps(D_Cap, g3reg)
     struct Cap *D_Cap;
     RASTER3D_Region *g3reg;

/* this subroutine only needs to be called once */
{
    int min;			/*the smallest dimension */
    int dim1, dim2;		/* the two largest dimensions */


    /* In order to determine how much memory to malloc for a D_buff
     ** need to determine the largest 2 dimensions (x,y or z)
     ** this is done by finding the minimum and using the other two */
    if (g3reg->cols < g3reg->rows) {
	dim1 = g3reg->rows;
	min = g3reg->cols;
    }
    else {
	dim1 = g3reg->cols;
	min = g3reg->rows;
    }

    if (g3reg->depths < min)
	dim2 = min;
    else
	dim2 = g3reg->depths;

#ifdef DEBUG1
    fprintf(stderr, "dim1 = %d   dim2 = %d \n", dim1, dim2);
#endif

    /* NOTE: code only written for floats at this time */
    /* malloc memory for buffer that will hold slice of data */
    if ((D_Cap->D_buff =
	 (float *)G_malloc(dim1 * dim2 * sizeof(float))) == NULL) {
	fprintf(stderr, "ERROR: in mallocing memory for D_Cap->D_buff\n");
	return (-1);
    }
    return (1);

}
