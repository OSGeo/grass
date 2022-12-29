#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "global.h"

int get_ref_window(struct Ref *ref, int *ref_list, struct Cell_head *cellhd)
{
    int i;
    int count;
    struct Cell_head win;

    /* from all the files in the group, get max extends and min resolutions */
    count = 0;
    for (i = 0; i < ref->nfiles; i++) {
	if (!ref_list[i])
	    continue;

	if (count++ == 0) {
	    Rast_get_cellhd(ref->file[i].name,
			    ref->file[i].mapset, cellhd);
	}
	else {
	    Rast_get_cellhd(ref->file[i].name,
			    ref->file[i].mapset, &win);
	    /* max extends */
	    if (cellhd->north < win.north)
		cellhd->north = win.north;
	    if (cellhd->south > win.south)
		cellhd->south = win.south;
	    if (cellhd->west > win.west)
		cellhd->west = win.west;
	    if (cellhd->east < win.east)
		cellhd->east = win.east;
	    /* min resolution */
	    if (cellhd->ns_res > win.ns_res)
		cellhd->ns_res = win.ns_res;
	    if (cellhd->ew_res > win.ew_res)
		cellhd->ew_res = win.ew_res;
	}
    }

    /* if the north-south is not multiple of the resolution,
     *    round the south downward
     */
    cellhd->rows = (cellhd->north - cellhd->south) / cellhd->ns_res + 0.5;
    cellhd->south = cellhd->north - cellhd->rows * cellhd->ns_res;

    /* do the same for the west */
    cellhd->cols = (cellhd->east - cellhd->west) / cellhd->ew_res + 0.5;
    cellhd->west = cellhd->east - cellhd->cols * cellhd->ew_res;

    return 1;
}

int georef_window(struct Image_Group *group, struct Cell_head *w1, struct Cell_head *w2, int order, double res)
{
    double n, e, ad;
    struct _corner {
        double n, e;
    } nw, ne, se, sw;

    /* extents */

    /* compute geo ref of all corners */
    if (order == 0)
	I_georef_tps(w1->west, w1->north, &e, &n, group->E12_t,
	             group->N12_t, &group->control_points, 1);
    else
	I_georef(w1->west, w1->north, &e, &n, group->E12, group->N12, order);
    w2->north = w2->south = n;
    w2->west = w2->east = e;
    nw.n = n;
    nw.e = e;

    if (order == 0)
	I_georef_tps(w1->east, w1->north, &e, &n, group->E12_t,
	             group->N12_t, &group->control_points, 1);
    else
	I_georef(w1->east, w1->north, &e, &n, group->E12, group->N12, order);
    ne.n = n;
    ne.e = e;
    if (n > w2->north)
	w2->north = n;
    if (n < w2->south)
	w2->south = n;
    if (e > w2->east)
	w2->east = e;
    if (e < w2->west)
	w2->west = e;

    if (order == 0)
	I_georef_tps(w1->west, w1->south, &e, &n, group->E12_t,
	             group->N12_t, &group->control_points, 1);
    else
	I_georef(w1->west, w1->south, &e, &n, group->E12, group->N12, order);
    sw.n = n;
    sw.e = e;
    if (n > w2->north)
	w2->north = n;
    if (n < w2->south)
	w2->south = n;
    if (e > w2->east)
	w2->east = e;
    if (e < w2->west)
	w2->west = e;

    if (order == 0)
	I_georef_tps(w1->east, w1->south, &e, &n, group->E12_t,
	             group->N12_t, &group->control_points, 1);
    else
	I_georef(w1->east, w1->south, &e, &n, group->E12, group->N12, order);
    se.n = n;
    se.e = e;
    if (n > w2->north)
	w2->north = n;
    if (n < w2->south)
	w2->south = n;
    if (e > w2->east)
	w2->east = e;
    if (e < w2->west)
	w2->west = e;

    /* resolution */
    if (res > 0)
	w2->ew_res = w2->ns_res = res;
    else {
	/* this results in ugly res values, and ns_res != ew_res */
	/* and is no good for rotation */
	/*
	w2->ns_res = (w2->north - w2->south) / w1->rows;
	w2->ew_res = (w2->east - w2->west) / w1->cols;
	*/

	/* alternative: account for rotation and order > 1 */

	/* N-S extends along western and eastern edge */
	w2->ns_res = (sqrt((nw.n - sw.n) * (nw.n - sw.n) +
			  (nw.e - sw.e) * (nw.e - sw.e)) +
		     sqrt((ne.n - se.n) * (ne.n - se.n) +
			  (ne.e - se.e) * (ne.e - se.e))) / (2.0 * w1->rows);

	/* E-W extends along northern and southern edge */
	w2->ew_res = (sqrt((nw.n - ne.n) * (nw.n - ne.n) +
			  (nw.e - ne.e) * (nw.e - ne.e)) +
		     sqrt((sw.n - se.n) * (sw.n - se.n) +
			  (sw.e - se.e) * (sw.e - se.e))) / (2.0 * w1->cols);

	/* make ew_res = ns_res */
	w2->ns_res = (w2->ns_res + w2->ew_res) / 2.0;
	w2->ew_res = w2->ns_res;

	/* nice round values */
	if (w2->ns_res > 1) {
	    if (w2->ns_res < 10) {
		/* round to first decimal */
		w2->ns_res = (int)(w2->ns_res * 10 + 0.5) / 10.0;
		w2->ew_res = w2->ns_res;
	    }
	    else {
		/* round to integer */
		w2->ns_res = (int)(w2->ns_res + 0.5);
		w2->ew_res = w2->ns_res;
	    }
	}
    }

    /* adjust extents */
    ad = w2->north > 0 ? 0.5 : -0.5;
    w2->north = (int) (ceil(w2->north / w2->ns_res) + ad) * w2->ns_res;
    ad = w2->south > 0 ? 0.5 : -0.5;
    w2->south = (int) (floor(w2->south / w2->ns_res) + ad) * w2->ns_res;
    ad = w2->east > 0 ? 0.5 : -0.5;
    w2->east = (int) (ceil(w2->east / w2->ew_res) + ad) * w2->ew_res;
    ad = w2->west > 0 ? 0.5 : -0.5;
    w2->west = (int) (floor(w2->west / w2->ew_res) + ad) * w2->ew_res;

    w2->rows = (w2->north - w2->south + w2->ns_res / 2.0) / w2->ns_res;
    w2->cols = (w2->east - w2->west + w2->ew_res / 2.0) / w2->ew_res;
    
    G_message(_("Region N=%f S=%f E=%f W=%f"), w2->north, w2->south,
	    w2->east, w2->west);
    G_message(_("Resolution EW=%f NS=%f"), w2->ew_res, w2->ns_res);

    return 0;
}
