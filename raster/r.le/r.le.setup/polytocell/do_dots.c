/* @(#)do_dots.c        2.1   6/26/87 */
#include "ply_to_cll.h"

void do_dots(double *xarray, double *yarray, int num_verticies, int category)
{
    int node;

    for (node = 0; node < num_verticies; node++) {
	write_record((int)(yarray[node] + .5),
		     (float)xarray[node], (float)xarray[node], category);
    }
}
