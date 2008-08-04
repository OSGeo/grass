#include <limits.h>
#include <float.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"


static void set_min(struct RASTER_MAP_PTR *, int, struct RASTER_MAP_PTR *);
static void set_max(struct RASTER_MAP_PTR *, int, struct RASTER_MAP_PTR *);


/* get_stats() Find out the number of cells total, number of nulls
 * the min value, the max value and the create the null replacement
 * value.
 */
void get_stats(struct rr_state *theState)
{
    int nrows, ncols, row, col;

    theState->fd_old = G_open_cell_old(theState->inraster, theState->mapset);
    if (theState->fd_old < 0)
	G_fatal_error(_("Unable to open raster map <%s>"),
		      theState->inraster);
    if (theState->docover == 1) {
	theState->fd_cold =
	    G_open_cell_old(theState->inrcover, theState->cmapset);
	if (theState->fd_cold < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  theState->inrcover);
    }
    theState->buf.type = G_get_raster_map_type(theState->fd_old);
    theState->buf.data.v = G_allocate_raster_buf(theState->buf.type);
    if (theState->docover == 1) {
	theState->cover.type = G_get_raster_map_type(theState->fd_cold);
	theState->cover.data.v = G_allocate_raster_buf(theState->cover.type);
    }

    theState->nulls.type = theState->buf.type;
    theState->min.type = theState->buf.type;
    theState->max.type = theState->buf.type;
    theState->nulls.data.v =
	(void *)G_malloc(G_raster_size(theState->nulls.type));
    theState->min.data.v =
	(void *)G_malloc(G_raster_size(theState->min.type));
    theState->max.data.v =
	(void *)G_malloc(G_raster_size(theState->max.type));

    if (theState->docover == 1) {
	theState->cnulls.type = theState->cover.type;
	theState->cmin.type = theState->cover.type;
	theState->cmax.type = theState->cover.type;
	theState->cnulls.data.v =
	    (void *)G_malloc(G_raster_size(theState->cnulls.type));
	theState->cmin.data.v =
	    (void *)G_malloc(G_raster_size(theState->cmin.type));
	theState->cmax.data.v =
	    (void *)G_malloc(G_raster_size(theState->cmax.type));
    }
    nrows = G_window_rows();
    ncols = G_window_cols();

    theState->nCells = nrows * ncols;
    theState->nNulls = 0;
    set_min(NULL, 0, &theState->min);
    set_max(NULL, 0, &theState->max);
    if (theState->docover == 1) {
	theState->cnCells = nrows * ncols;
	theState->cnNulls = 0;
	set_min(NULL, 0, &theState->cmin);
	set_max(NULL, 0, &theState->cmax);
    }
    G_message(_("Collecting Stats..."));
    for (row = 0; row < nrows; row++) {
	if (G_get_raster_row(theState->fd_old, theState->buf.data.v,
			     row, theState->buf.type) < 0)
	    G_fatal_error(_("Cannot read raster row [%d]"), row);
	if (theState->docover == 1) {
	    if (G_get_raster_row(theState->fd_cold, theState->cover.data.v,
				 row, theState->cover.type) < 0)
		G_fatal_error(_("Cannot read cover raster row [%d]"), row);
	}
	for (col = 0; col < ncols; col++) {
	    if (is_null_value(theState->buf, col)) {
		theState->nNulls++;
	    }
	    else {
		set_min(&theState->buf, col, &theState->min);
		set_max(&theState->buf, col, &theState->max);
	    }
	    if (theState->docover == 1) {
		if (is_null_value(theState->cover, col)) {
		    theState->cnNulls++;
		}
		else {
		    set_min(&theState->cover, col, &theState->cmin);
		    set_max(&theState->cover, col, &theState->cmax);
		}
	    }
	}

	G_percent(row, nrows, 2);
    }

    G_percent(1, 1, 1);

    /* rewind the in raster map descriptor for later use */
    lseek(theState->fd_old, 0, SEEK_SET);
    if (theState->docover == 1)
	lseek(theState->fd_cold, 0, SEEK_SET);

    /* Set the NULL value replacement */
    switch (theState->nulls.type) {
    case CELL_TYPE:
	*theState->nulls.data.c = *theState->min.data.c - 1;
	if (theState->docover == 1)
	    *theState->cnulls.data.c = *theState->cmin.data.c - 1;
	break;
    case FCELL_TYPE:
	*theState->nulls.data.f = floor(*theState->min.data.f - 1);
	if (theState->docover == 1)
	    *theState->cnulls.data.f = floor(*theState->cmin.data.f - 1);
	break;
    case DCELL_TYPE:
	*theState->nulls.data.d = floor(*theState->min.data.d - 1);
	if (theState->docover == 1)
	    *theState->cnulls.data.d = floor(*theState->cmin.data.d - 1);
	break;
    default:			/* Huh? */
	G_fatal_error(_("Programmer error in get_stats/switch"));
    }
}				/* get_stats() */


static void set_min(struct RASTER_MAP_PTR *from, int col,
		    struct RASTER_MAP_PTR *to)
{
    if (from == NULL) {
	switch (to->type) {
	case CELL_TYPE:
	    *to->data.c = INT_MAX;
	    break;
	case FCELL_TYPE:
	    *to->data.f = FLT_MAX;
	    break;
	case DCELL_TYPE:
	    *to->data.d = DBL_MAX;
	    break;
	}
    }
    else {
	switch (to->type) {
	case CELL_TYPE:
	    *to->data.c = (*to->data.c < from->data.c[col]) ?
		*to->data.c : from->data.c[col];
	    break;
	case FCELL_TYPE:
	    *to->data.f = (*to->data.f < from->data.f[col]) ?
		*to->data.f : from->data.f[col];
	    break;
	case DCELL_TYPE:
	    *to->data.d = (*to->data.d < from->data.d[col]) ?
		*to->data.d : from->data.d[col];
	    break;
	}
    }
}


static void set_max(struct RASTER_MAP_PTR *from, int col,
		    struct RASTER_MAP_PTR *to)
{
    if (from == NULL) {
	switch (to->type) {
	case CELL_TYPE:
	    *to->data.c = INT_MIN;
	    break;
	case FCELL_TYPE:
	    *to->data.f = FLT_MIN;
	    break;
	case DCELL_TYPE:
	    *to->data.d = DBL_MIN;
	    break;
	}
    }
    else {
	switch (to->type) {
	case CELL_TYPE:
	    *to->data.c = (*to->data.c > from->data.c[col]) ?
		*to->data.c : from->data.c[col];
	    break;
	case FCELL_TYPE:
	    *to->data.f = (*to->data.f > from->data.f[col]) ?
		*to->data.f : from->data.f[col];
	    break;
	case DCELL_TYPE:
	    *to->data.d = (*to->data.d > from->data.d[col]) ?
		*to->data.d : from->data.d[col];
	    break;
	}
    }
}
