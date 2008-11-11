#include <grass/glocale.h>
#include "global.h"
#include "crs.h"		/* CRS HEADER FILE */

int georef_window(struct Cell_head *w1, struct Cell_head *w2, int order)
{
    double n, e;

    CRS_georef(w1->west, w1->north, &e, &n, E12, N12, order);
    w2->north = w2->south = n;
    w2->west = w2->east = e;

    CRS_georef(w1->east, w1->north, &e, &n, E12, N12, order);
    if (n > w2->north)
	w2->north = n;
    if (n < w2->south)
	w2->south = n;
    if (e > w2->east)
	w2->east = e;
    if (e < w2->west)
	w2->west = e;

    CRS_georef(w1->west, w1->south, &e, &n, E12, N12, order);
    if (n > w2->north)
	w2->north = n;
    if (n < w2->south)
	w2->south = n;
    if (e > w2->east)
	w2->east = e;
    if (e < w2->west)
	w2->west = e;

    CRS_georef(w1->east, w1->south, &e, &n, E12, N12, order);
    if (n > w2->north)
	w2->north = n;
    if (n < w2->south)
	w2->south = n;
    if (e > w2->east)
	w2->east = e;
    if (e < w2->west)
	w2->west = e;

    w2->ns_res = (w2->north - w2->south) / w1->rows;
    w2->ew_res = (w2->east - w2->west) / w1->cols;

    G_message(_("Region N=%f S=%f E=%f W=%f"), w2->north, w2->south,
	    w2->east, w2->west);
    G_message(_("Resolution EW=%f NS=%f"), w2->ew_res, w2->ns_res);

    return 0;
}
