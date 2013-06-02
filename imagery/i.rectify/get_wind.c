#include <math.h>
#include <grass/glocale.h>
#include "global.h"

int georef_window(struct Cell_head *w1, struct Cell_head *w2, int order, double res)
{
    double n, e, ad;
    struct _corner {
        double n, e;
    } nw, ne, se, sw;

    /* extends */
    if (order == 0)
	I_georef_tps(w1->west, w1->north, &e, &n, E12_t, N12_t, &cp, 1);
    else
	I_georef(w1->west, w1->north, &e, &n, E12, N12, order);
    w2->north = w2->south = n;
    w2->west = w2->east = e;
    nw.n = n;
    nw.e = e;

    if (order == 0)
	I_georef_tps(w1->east, w1->north, &e, &n, E12_t, N12_t, &cp, 1);
    else
	I_georef(w1->east, w1->north, &e, &n, E12, N12, order);
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
	I_georef_tps(w1->west, w1->south, &e, &n, E12_t, N12_t, &cp, 1);
    else
	I_georef(w1->west, w1->south, &e, &n, E12, N12, order);
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
	I_georef_tps(w1->east, w1->south, &e, &n, E12_t, N12_t, &cp, 1);
    else
	I_georef(w1->east, w1->south, &e, &n, E12, N12, order);
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

    /* adjust extends */
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
