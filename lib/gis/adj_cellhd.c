/*!
 * \file lib/gis/adj_cellhd.c
 *
 * \brief GIS Library - CELL header adjustment.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define LL_TOLERANCE 10

/* TODO: find good thresholds */
/* deviation measured in cells */
static double llepsilon = 0.01;
static double fpepsilon = 1.0e-9;

static int ll_wrap(struct Cell_head *cellhd);
static int ll_check_ns(struct Cell_head *cellhd);
static int ll_check_ew(struct Cell_head *cellhd);

/*!
 * \brief Adjust cell header.
 *
 * This function fills in missing parts of the input cell header (or
 * region). It also makes projection-specific adjustments. The
 * <i>cellhd</i> structure must have its <i>north, south, east,
 * west</i>, and <i>proj</i> fields set.
 *
 * If <i>row_flag</i> is true, then the north-south resolution is
 * computed from the number of <i>rows</i> in the <i>cellhd</i>
 * structure. Otherwise the number of <i>rows</i> is computed from the
 * north-south resolution in the structure, similarly for
 * <i>col_flag</i> and the number of columns and the east-west
 * resolution.
 *
 * <b>Note:</b> 3D values are not adjusted.
 *
 * \param[in,out] cellhd pointer to Cell_head structure
 * \param row_flag compute n-s resolution
 * \param col_flag compute e-w resolution
 */
void G_adjust_Cell_head(struct Cell_head *cellhd, int row_flag, int col_flag)
{
    double old_res;

    if (!row_flag) {
	if (cellhd->ns_res <= 0)
	    G_fatal_error(_("Illegal n-s resolution value: %g"),
			  cellhd->ns_res);
    }
    else {
	if (cellhd->rows <= 0)
	    G_fatal_error(_("Illegal number of rows: %d"
			    " (resolution is %g)"),
			  cellhd->rows, cellhd->ns_res);
    }
    if (!col_flag) {
	if (cellhd->ew_res <= 0)
	    G_fatal_error(_("Illegal e-w resolution value: %g"),
			  cellhd->ew_res);
    }
    else {
	if (cellhd->cols <= 0)
	    G_fatal_error(_("Illegal number of columns: %d"
			    " (resolution is %g)"),
			  cellhd->cols, cellhd->ew_res);
    }

    /* check the edge values */
    if (cellhd->north <= cellhd->south) {
	if (cellhd->proj == PROJECTION_LL)
	    G_fatal_error(_("North must be north of South,"
			    " but %g (north) <= %g (south"),
			  cellhd->north, cellhd->south);
	else
	    G_fatal_error(_("North must be larger than South,"
			    " but %g (north) <= %g (south"),
		          cellhd->north, cellhd->south);
    }

    ll_wrap(cellhd);

    if (cellhd->east <= cellhd->west)
	G_fatal_error(_("East must be larger than West,"
			" but %g (east) <= %g (west)"),
		      cellhd->east, cellhd->west);

    /* compute rows and columns, if not set */
    if (!row_flag) {
	cellhd->rows =
	    (cellhd->north - cellhd->south +
	     cellhd->ns_res / 2.0) / cellhd->ns_res;
	if (cellhd->rows == 0)
	    cellhd->rows = 1;
    }
    if (!col_flag) {
	cellhd->cols =
	    (cellhd->east - cellhd->west +
	     cellhd->ew_res / 2.0) / cellhd->ew_res;
	if (cellhd->cols == 0)
	    cellhd->cols = 1;
    }

    if (cellhd->cols < 0) {
	G_fatal_error(_("Invalid coordinates: negative number of columns"));
    }
    if (cellhd->rows < 0) {
	G_fatal_error(_("Invalid coordinates: negative number of rows"));
    }

    /* (re)compute the resolutions */
    old_res = cellhd->ns_res;
    cellhd->ns_res = (cellhd->north - cellhd->south) / cellhd->rows;
    if (old_res > 0 && fabs(old_res - cellhd->ns_res) / old_res > 0.01)
	G_verbose_message(_("NS resolution has been changed"));

    old_res = cellhd->ew_res;
    cellhd->ew_res = (cellhd->east - cellhd->west) / cellhd->cols;
    if (old_res > 0 && fabs(old_res - cellhd->ew_res) / old_res > 0.01)
	G_verbose_message(_("EW resolution has been changed"));

    if (fabs(cellhd->ns_res - cellhd->ew_res) / cellhd->ns_res > 0.01)
	G_verbose_message(_("NS and EW resolutions are different"));

    ll_check_ns(cellhd);
    ll_check_ew(cellhd);
}

/*!
 * \brief Adjust cell header for 3D values.
 *
 * This function fills in missing parts of the input cell header (or
 * region).  It also makes projection-specific adjustments. The
 * <i>cellhd</i> structure must have its <i>north, south, east,
 * west</i>, and <i>proj</i> fields set.
 *
 * If <i>row_flag</i> is true, then the north-south resolution is computed
 * from the number of <i>rows</i> in the <i>cellhd</i> structure.
 * Otherwise the number of <i>rows</i> is computed from the north-south
 * resolution in the structure, similarly for <i>col_flag</i> and the
 * number of columns and the east-west resolution.
 *
 * If <i>depth_flag</i> is true, top-bottom resolution is calculated
 * from depths.
 * If <i>depth_flag</i> are false, number of depths is calculated from
 * top-bottom resolution.
 *
 * \warning This function can cause segmentation fault without any warning
 * when it is called with Cell_head top and bottom set to zero.
 *
 * \param[in,out] cellhd pointer to Cell_head structure
 * \param row_flag compute n-s resolution
 * \param col_flag compute e-w resolution
 * \param depth_flag compute t-b resolution
 */
void G_adjust_Cell_head3(struct Cell_head *cellhd, int row_flag,
			 int col_flag, int depth_flag)
{
    double old_res;

    if (!row_flag) {
	if (cellhd->ns_res <= 0)
	    G_fatal_error(_("Illegal n-s resolution value: %g"),
			  cellhd->ns_res);
	if (cellhd->ns_res3 <= 0)
	    G_fatal_error(_("Illegal n-s resolution value for 3D: %g"),
			  cellhd->ns_res3);
    }
    else {
	if (cellhd->rows <= 0)
	    G_fatal_error(_("Illegal number of rows: %d"
			    " (resolution is %g)"),
			  cellhd->rows, cellhd->ns_res);
	if (cellhd->rows3 <= 0)
	    G_fatal_error(_("Illegal number of rows for 3D: %d"
			    " (resolution is %g)"),
			  cellhd->rows3, cellhd->ns_res3);
    }
    if (!col_flag) {
	if (cellhd->ew_res <= 0)
	    G_fatal_error(_("Illegal e-w resolution value: %g"),
			  cellhd->ew_res);
	if (cellhd->ew_res3 <= 0)
	    G_fatal_error(_("Illegal e-w resolution value for 3D: %g"),
			  cellhd->ew_res3);
    }
    else {
	if (cellhd->cols <= 0)
	    G_fatal_error(_("Illegal number of columns: %d"
			    " (resolution is %g)"),
			  cellhd->cols, cellhd->ew_res);
	if (cellhd->cols3 <= 0)
	    G_fatal_error(_("Illegal number of columns for 3D: %d"
			    " (resolution is %g)"),
			  cellhd->cols3, cellhd->ew_res3);
    }
    if (!depth_flag) {
	if (cellhd->tb_res <= 0)
	    G_fatal_error(_("Illegal t-b resolution value: %g"),
			  cellhd->tb_res);
    }
    else {
	if (cellhd->depths <= 0)
	    G_fatal_error(_("Illegal depths value: %d"), cellhd->depths);
    }

    /* check the edge values */
    if (cellhd->north <= cellhd->south) {
	if (cellhd->proj == PROJECTION_LL)
	    G_fatal_error(_("North must be north of South,"
			    " but %g (north) <= %g (south"),
			  cellhd->north, cellhd->south);
	else
	    G_fatal_error(_("North must be larger than South,"
			    " but %g (north) <= %g (south"),
		          cellhd->north, cellhd->south);
    }

    ll_wrap(cellhd);

    if (cellhd->east <= cellhd->west)
	G_fatal_error(_("East must be larger than West,"
			" but %g (east) <= %g (west)"),
		      cellhd->east, cellhd->west);

    if (cellhd->top <= cellhd->bottom)
	G_fatal_error(_("Top must be larger than Bottom,"
			" but %g (top) <= %g (bottom)"),
		      cellhd->top, cellhd->bottom);

    /* compute rows and columns, if not set */
    if (!row_flag) {
	cellhd->rows =
	    (cellhd->north - cellhd->south +
	     cellhd->ns_res / 2.0) / cellhd->ns_res;
	if (cellhd->rows == 0)
	    cellhd->rows = 1;

	cellhd->rows3 =
	    (cellhd->north - cellhd->south +
	     cellhd->ns_res3 / 2.0) / cellhd->ns_res3;
	if (cellhd->rows3 == 0)
	    cellhd->rows3 = 1;
    }
    if (!col_flag) {
	cellhd->cols =
	    (cellhd->east - cellhd->west +
	     cellhd->ew_res / 2.0) / cellhd->ew_res;
	if (cellhd->cols == 0)
	    cellhd->cols = 1;

	cellhd->cols3 =
	    (cellhd->east - cellhd->west +
	     cellhd->ew_res3 / 2.0) / cellhd->ew_res3;
	if (cellhd->cols3 == 0)
	    cellhd->cols3 = 1;
    }

    if (!depth_flag) {
	cellhd->depths =
	    (cellhd->top - cellhd->bottom +
	     cellhd->tb_res / 2.0) / cellhd->tb_res;
	if (cellhd->depths == 0)
	    cellhd->depths = 1;
    }

    if (cellhd->cols < 0 || cellhd->cols3 < 0) {
	G_fatal_error(_("Invalid coordinates: negative number of columns"));
    }
    if (cellhd->rows < 0 || cellhd->rows3 < 0) {
	G_fatal_error(_("Invalid coordinates: negative number of rows"));
    }
    if (cellhd->depths < 0) {
	G_fatal_error(_("Invalid coordinates: negative number of depths"));
    }

    /* (re)compute the resolutions */
    old_res = cellhd->ns_res;
    cellhd->ns_res = (cellhd->north - cellhd->south) / cellhd->rows;
    if (old_res > 0 && fabs(old_res - cellhd->ns_res) / old_res > 0.01)
	G_verbose_message(_("NS resolution has been changed"));

    old_res = cellhd->ew_res;
    cellhd->ew_res = (cellhd->east - cellhd->west) / cellhd->cols;
    if (old_res > 0 && fabs(old_res - cellhd->ew_res) / old_res > 0.01)
	G_verbose_message(_("EW resolution has been changed"));

    if (fabs(cellhd->ns_res - cellhd->ew_res) / cellhd->ns_res > 0.01)
	G_verbose_message(_("NS and EW resolutions are different"));

    ll_check_ns(cellhd);
    ll_check_ew(cellhd);

    cellhd->ns_res3 = (cellhd->north - cellhd->south) / cellhd->rows3;
    cellhd->ew_res3 = (cellhd->east - cellhd->west) / cellhd->cols3;
    cellhd->tb_res = (cellhd->top - cellhd->bottom) / cellhd->depths;
}

static int ll_wrap(struct Cell_head *cellhd)
{
    double shift;

    /* for lat/lon, force east larger than west, try to wrap to -180, 180 */
    if (cellhd->proj != PROJECTION_LL)
	return 0;

    if (cellhd->east <= cellhd->west) {
	G_warning(_("East (%.15g) is not larger than West (%.15g)"),
	          cellhd->east, cellhd->west);

	while (cellhd->east <= cellhd->west)
	    cellhd->east += 360.0;
    }

    /* with east larger than west,
     * any 360 degree W-E extent can be represented within -360, 360
     * but not within -180, 180 */

    /* try to shift to within -180, 180 */
    shift = 0;
    while (cellhd->west + shift >= 180) {
	shift -= 360.0;
    }
    while (cellhd->east + shift <= -180) {
	shift += 360.0;
    }

    /* try to shift to within -360, 360 */
    while (cellhd->east + shift > 360) {
	shift -= 360.0;
    }
    while (cellhd->west + shift <= -360) {
	shift += 360.0;
    }

    if (shift) {
	cellhd->west += shift;
	cellhd->east += shift;
    }

    /* very liberal thresholds */
    if (cellhd->north > 90.0 + LL_TOLERANCE)
	G_fatal_error(_("Illegal latitude for North: %g"), cellhd->north);
    if (cellhd->south < -90.0 - LL_TOLERANCE)
	G_fatal_error(_("Illegal latitude for South: %g"), cellhd->south);

#if 0
    /* disabled: allow W-E extents larger than 360 degree e.g. for display */
    if (cellhd->west < -360.0 - LL_TOLERANCE) {
	G_debug(1, "East: %g", cellhd->east);
	G_fatal_error(_("Illegal longitude for West: %g"), cellhd->west);
    }
    if (cellhd->east > 360.0 + LL_TOLERANCE) {
	G_debug(1, "West: %g", cellhd->west);
	G_fatal_error(_("Illegal longitude for East: %g"), cellhd->east);
    }
#endif

    return 1;
}

static int ll_check_ns(struct Cell_head *cellhd)
{
    int lladjust;
    double diff;
    int ncells;

    /* lat/lon checks */
    if (cellhd->proj != PROJECTION_LL)
	return 0;

    lladjust = 0;

    G_debug(3, "ll_check_ns: epsilon: %g", llepsilon);

    /* North, South: allow a half cell spill-over */

    diff = (cellhd->north - cellhd->south) / cellhd->ns_res;
    ncells = (int) (diff + 0.5);
    diff -= ncells;
    if ((diff < 0 && diff < -fpepsilon) ||
        (diff > 0 && diff > fpepsilon)) {
	G_verbose_message(_("NS extent does not match NS resolution: %g cells difference"),
	          diff);
    }

    /* north */
    diff = (cellhd->north - 90) / cellhd->ns_res;
    if (diff < 0)
	diff = -diff;
    if (cellhd->north < 90.0 && diff < 1.0 ) {
	G_verbose_message(_("%g cells missing to reach 90 degree north"),
		  diff);
	if (diff < llepsilon && diff > fpepsilon) {
	    G_verbose_message(_("Subtle input data rounding error of north boundary (%g)"),
		      cellhd->north - 90.0);
	    /* check only, do not modify
	    cellhd->north = 90.0;
	    lladjust = 1;
	    */
	}
    }
    if (cellhd->north > 90.0) {
	if (diff <= 0.5 + llepsilon) {
	    G_important_message(_("90 degree north is exceeded by %g cells"),
		      diff);

	    if (diff < llepsilon && diff > fpepsilon) {
		G_verbose_message(_("Subtle input data rounding error of north boundary (%g)"),
			  cellhd->north - 90.0);
		G_debug(1, "North of north in seconds: %g",
			(cellhd->north - 90.0) * 3600);
		/* check only, do not modify
		cellhd->north = 90.0;
		lladjust = 1;
		*/
	    }

	    diff = diff - 0.5;
	    if (diff < 0)
		diff = -diff;
	    if (diff < llepsilon && diff > fpepsilon) {
		G_verbose_message(_("Subtle input data rounding error of north boundary (%g)"),
			  cellhd->north - 90.0 - cellhd->ns_res / 2.0);
		G_debug(1, "North of north + 0.5 cells in seconds: %g",
			(cellhd->north - 90.0 - cellhd->ns_res / 2.0) * 3600);
		/* check only, do not modify
		cellhd->north = 90.0 + cellhd->ns_res / 2.0;
		lladjust = 1;
		*/
	    }
	}
	else
	    G_fatal_error(_("Illegal latitude for North"));
    }

    /* south */
    diff = (cellhd->south + 90) / cellhd->ns_res;
    if (diff < 0)
	diff = -diff;
    if (cellhd->south > -90.0 && diff < 1.0 ) {
	G_verbose_message(_("%g cells missing to reach 90 degree south"),
		  diff);
	if (diff < llepsilon && diff > fpepsilon) {
	    G_verbose_message(_("Subtle input data rounding error of south boundary (%g)"),
		      cellhd->south + 90.0);
	    /* check only, do not modify
	    cellhd->south = -90.0;
	    lladjust = 1;
	    */
	}
    }
    if (cellhd->south < -90.0) {
	if (diff <= 0.5 + llepsilon) {
	    G_important_message(_("90 degree south is exceeded by %g cells"),
		      diff);

	    if (diff < llepsilon && diff > fpepsilon) {
		G_verbose_message(_("Subtle input data rounding error of south boundary (%g)"),
			  cellhd->south + 90);
		G_debug(1, "South of south in seconds: %g",
			(-cellhd->south - 90) * 3600);
		/* check only, do not modify
		cellhd->south = -90.0;
		lladjust = 1;
		*/
	    }

	    diff = diff - 0.5;
	    if (diff < 0)
		diff = -diff;
	    if (diff < llepsilon && diff > fpepsilon) {
		G_verbose_message(_("Subtle input data rounding error of south boundary (%g)"),
			  cellhd->south + 90 + cellhd->ns_res / 2.0);
		G_debug(1, "South of south + 0.5 cells in seconds: %g",
			(-cellhd->south - 90 - cellhd->ns_res / 2.0) * 3600);
		/* check only, do not modify
		cellhd->south = -90.0 - cellhd->ns_res / 2.0;
		lladjust = 1;
		*/
	    }
	}
	else
	    G_fatal_error(_("Illegal latitude for South"));
    }

    if (lladjust)
	cellhd->ns_res = (cellhd->north - cellhd->south) / cellhd->rows;

    return lladjust;
}

static int ll_check_ew(struct Cell_head *cellhd)
{
    int lladjust;
    double diff;
    int ncells;

    /* lat/lon checks */
    if (cellhd->proj != PROJECTION_LL)
	return 0;

    lladjust = 0;

    G_debug(3, "ll_check_ew: epsilon: %g", llepsilon);

    /* west - east, no adjustment */
    diff = (cellhd->east - cellhd->west) / cellhd->ew_res;
    ncells = (int) (diff + 0.5);
    diff -= ncells;
    if ((diff < 0 && diff < -fpepsilon) ||
        (diff > 0 && diff > fpepsilon)) {
	G_verbose_message(_("EW extent does not match EW resolution: %g cells difference"),
	          diff);
    }
    if (cellhd->east - cellhd->west > 360.0) {
	diff = (cellhd->east - cellhd->west - 360.0) / cellhd->ew_res;
	if (diff > fpepsilon)
	    G_important_message(_("360 degree EW extent is exceeded by %g cells"),
		      diff);
    }
    else if (cellhd->east - cellhd->west < 360.0) {
	diff = (360.0 - (cellhd->east - cellhd->west)) / cellhd->ew_res;
	if (diff < 1.0 && diff > fpepsilon)
	    G_verbose_message(_("%g cells missing to cover 360 degree EW extent"),
		  diff);
    }

    return lladjust;
}

/*!
 * \brief Adjust window for lat/lon.
 *
 * This function tries to automatically fix fp precision issues and
 * adjust rounding errors for lat/lon.
 *
 * <b>Note:</b> 3D values are not adjusted.
 *
 * \param[in,out] cellhd pointer to Cell_head structure
 * \return 1 if window was adjusted
 * \return 0 if window was not adjusted
 */
int G_adjust_window_ll(struct Cell_head *cellhd)
{
    int ll_adjust, res_adj;
    double dsec, dsec2;
    char buf[100], buf2[100];
    double diff;
    double old, new;
    struct Cell_head cellhds;	/* everything in seconds, not degrees */

    /* lat/lon checks */
    if (cellhd->proj != PROJECTION_LL)
	return 0;

    /* put everything through ll_format + ll_scan */
    G_llres_format(cellhd->ns_res, buf);
    if (G_llres_scan(buf, &new) != 1)
	G_fatal_error(_("Invalid NS resolution"));
    cellhd->ns_res = new;

    G_llres_format(cellhd->ew_res, buf);
    if (G_llres_scan(buf, &new) != 1)
	G_fatal_error(_("Invalid EW resolution"));
    cellhd->ew_res = new;

    G_lat_format(cellhd->north, buf);
    if (G_lat_scan(buf, &new) != 1)
	G_fatal_error(_("Invalid North"));
    cellhd->north = new;

    G_lat_format(cellhd->south, buf);
    if (G_lat_scan(buf, &new) != 1)
	G_fatal_error(_("Invalid South"));
    cellhd->south = new;

    G_lon_format(cellhd->west, buf);
    if (G_lon_scan(buf, &new) != 1)
	G_fatal_error(_("Invalid West"));
    cellhd->west = new;

    G_lon_format(cellhd->east, buf);
    if (G_lon_scan(buf, &new) != 1)
	G_fatal_error(_("Invalid East"));
    cellhd->east = new;

    /* convert to seconds */
    cellhds = *cellhd;

    old = cellhds.ns_res * 3600;
    sprintf(buf, "%f", old);
    sscanf(buf, "%lf", &new);
    cellhds.ns_res = new;

    old = cellhds.ew_res * 3600;
    sprintf(buf, "%f", old);
    sscanf(buf, "%lf", &new);
    cellhds.ew_res = new;

    old = cellhds.north * 3600;
    sprintf(buf, "%f", old);
    sscanf(buf, "%lf", &new);
    cellhds.north = new;

    old = cellhds.south * 3600;
    sprintf(buf, "%f", old);
    sscanf(buf, "%lf", &new);
    cellhds.south = new;

    old = cellhds.west * 3600;
    sprintf(buf, "%f", old);
    sscanf(buf, "%lf", &new);
    cellhds.west = new;

    old = cellhds.east * 3600;
    sprintf(buf, "%f", old);
    sscanf(buf, "%lf", &new);
    cellhds.east = new;

    ll_adjust = 0;

    /* N - S */
    /* resolution */
    res_adj = 0;
    old = cellhds.ns_res;

    if (old > 0.4) {
	/* round to nearest 0.1 sec */
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	new = dsec2 / 10;
	diff = fabs(dsec2 - dsec) / dsec;
	if (diff > 0 && diff < llepsilon) {
	    G_llres_format(old / 3600, buf);
	    G_llres_format(new / 3600, buf2);
	    if (strcmp(buf, buf2))
		G_verbose_message(_("NS resolution rounded from %s to %s"),
			  buf, buf2);
	    ll_adjust = 1;
	    res_adj = 1;
	    cellhds.ns_res = new;
	}
    }

    if (res_adj) {
	double n_off, s_off;

	old = cellhds.north;
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	diff = fabs(dsec2 - dsec) / (cellhds.ns_res * 10);
	n_off = diff;

	old = cellhds.south;
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	diff = fabs(dsec2 - dsec) / (cellhds.ns_res * 10);
	s_off = diff;

	if (n_off < llepsilon || n_off <= s_off) {
	    old = cellhds.north;
	    dsec = old * 10;
	    dsec2 = floor(dsec + 0.5);
	    new = dsec2 / 10;
	    diff = n_off;
	    if (diff > 0 && diff < llepsilon) {
		G_lat_format(old / 3600, buf);
		G_lat_format(new / 3600, buf2);
		if (strcmp(buf, buf2))
		    G_verbose_message(_("North rounded from %s to %s"),
			      buf, buf2);
		cellhds.north = new;
	    }

	    old = cellhds.south;
	    new = cellhds.north - cellhds.ns_res * cellhds.rows;
	    diff = fabs(new - old) / cellhds.ns_res;
	    if (diff > 0) {
		G_lat_format(old / 3600, buf);
		G_lat_format(new / 3600, buf2);
		if (strcmp(buf, buf2))
		    G_verbose_message(_("South adjusted from %s to %s"),
			      buf, buf2);
	    }
	    cellhds.south = new;
	}
	else {
	    old = cellhds.south;
	    dsec = old * 10;
	    dsec2 = floor(dsec + 0.5);
	    new = dsec2 / 10;
	    diff = s_off;
	    if (diff > 0 && diff < llepsilon) {
		G_lat_format(old / 3600, buf);
		G_lat_format(new / 3600, buf2);
		if (strcmp(buf, buf2))
		    G_verbose_message(_("South rounded from %s to %s"),
			      buf, buf2);
		cellhds.south = new;
	    }

	    old = cellhds.north;
	    new = cellhds.south + cellhds.ns_res * cellhds.rows;
	    diff = fabs(new - old) / cellhds.ns_res;
	    if (diff > 0) {
		G_lat_format(old / 3600, buf);
		G_lat_format(new / 3600, buf2);
		if (strcmp(buf, buf2))
		    G_verbose_message(_("North adjusted from %s to %s"),
			      buf, buf2);
	    }
	    cellhds.north = new;
	}
    }
    else {
	old = cellhds.north;
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	new = dsec2 / 10;
	diff = fabs(dsec2 - dsec) / (cellhds.ns_res * 10);
	if (diff > 0 && diff < llepsilon) {
	    G_lat_format(old / 3600, buf);
	    G_lat_format(new / 3600, buf2);
	    if (strcmp(buf, buf2))
		G_verbose_message(_("North rounded from %s to %s"),
			  buf, buf2);
	    ll_adjust = 1;
	    cellhds.north = new;
	}

	old = cellhds.south;
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	new = dsec2 / 10;
	diff = fabs(dsec2 - dsec) / (cellhds.ns_res * 10);
	if (diff > 0 && diff < llepsilon) {
	    G_lat_format(old / 3600, buf);
	    G_lat_format(new / 3600, buf2);
	    if (strcmp(buf, buf2))
		G_verbose_message(_("South rounded from %s to %s"),
			  buf, buf2);
	    ll_adjust = 1;
	    cellhds.south = new;
	}
    }
    cellhds.ns_res = (cellhds.north - cellhds.south) / cellhds.rows;

    /* E - W */
    /* resolution */
    res_adj = 0;
    old = cellhds.ew_res;

    if (old > 0.4) {
	/* round to nearest 0.1 sec */
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	new = dsec2 / 10;
	diff = fabs(dsec2 - dsec) / dsec;
	if (diff > 0 && diff < llepsilon) {
	    G_llres_format(old / 3600, buf);
	    G_llres_format(new / 3600, buf2);
	    if (strcmp(buf, buf2))
		G_verbose_message(_("EW resolution rounded from %s to %s"),
			  buf, buf2);
	    ll_adjust = 1;
	    res_adj = 1;
	    cellhds.ew_res = new;
	}
    }

    if (res_adj) {
	double w_off, e_off;

	old = cellhds.west;
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	diff = fabs(dsec2 - dsec) / (cellhds.ew_res * 10);
	w_off = diff;

	old = cellhds.east;
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	diff = fabs(dsec2 - dsec) / (cellhds.ew_res * 10);
	e_off = diff;

	if (w_off < llepsilon || w_off <= e_off) {
	    old = cellhds.west;
	    dsec = old * 10;
	    dsec2 = floor(dsec + 0.5);
	    new = dsec2 / 10;
	    diff = w_off;
	    if (diff > 0 && diff < llepsilon) {
		G_lon_format(old / 3600, buf);
		G_lon_format(new / 3600, buf2);
		if (strcmp(buf, buf2))
		    G_verbose_message(_("West rounded from %s to %s"),
			      buf, buf2);
		cellhds.west = new;
	    }

	    old = cellhds.east;
	    new = cellhds.west + cellhds.ew_res * cellhds.cols;
	    diff = fabs(new - old) / cellhds.ew_res;
	    if (diff > 0) {
		G_lon_format(old / 3600, buf);
		G_lon_format(new / 3600, buf2);
		if (strcmp(buf, buf2))
		    G_verbose_message(_("East adjusted from %s to %s"),
			      buf, buf2);
	    }
	    cellhds.east = new;
	}
	else {
	    old = cellhds.east;
	    dsec = old * 10;
	    dsec2 = floor(dsec + 0.5);
	    new = dsec2 / 10;
	    diff = e_off;
	    if (diff > 0 && diff < llepsilon) {
		G_lon_format(old / 3600, buf);
		G_lon_format(new / 3600, buf2);
		if (strcmp(buf, buf2))
		    G_verbose_message(_("East rounded from %s to %s"),
			      buf, buf2);
		cellhds.east = new;
	    }

	    old = cellhds.west;
	    new = cellhds.east - cellhds.ew_res * cellhds.cols;
	    diff = fabs(new - cellhds.west) / cellhds.ew_res;
	    if (diff > 0) {
		G_lon_format(old / 3600, buf);
		G_lon_format(new / 3600, buf2);
		if (strcmp(buf, buf2))
		    G_verbose_message(_("West adjusted from %s to %s"),
			      buf, buf2);
	    }
	    cellhds.west = new;
	}
    }
    else {
	old = cellhds.west;
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	new = dsec2 / 10;
	diff = fabs(dsec2 - dsec) / (cellhds.ew_res * 10);
	if (diff > 0 && diff < llepsilon) {
	    G_lon_format(old / 3600, buf);
	    G_lon_format(new / 3600, buf2);
	    if (strcmp(buf, buf2))
		G_verbose_message(_("West rounded from %s to %s"),
			  buf, buf2);
	    ll_adjust = 1;
	    cellhds.west = new;
	}

	old = cellhds.east;
	dsec = old * 10;
	dsec2 = floor(dsec + 0.5);
	new = dsec2 / 10;
	diff = fabs(dsec2 - dsec) / (cellhds.ew_res * 10);
	if (diff > 0 && diff < llepsilon) {
	    G_lon_format(old / 3600, buf);
	    G_lon_format(new / 3600, buf2);
	    if (strcmp(buf, buf2))
		G_verbose_message(_("East rounded from %s to %s"),
			  buf, buf2);
	    ll_adjust = 1;
	    cellhds.east = new;
	}
    }
    cellhds.ew_res = (cellhds.east - cellhds.west) / cellhds.cols;

    cellhd->ns_res = cellhds.ns_res / 3600;
    cellhd->ew_res = cellhds.ew_res / 3600;
    cellhd->north = cellhds.north / 3600;
    cellhd->south = cellhds.south / 3600;
    cellhd->west = cellhds.west / 3600;
    cellhd->east = cellhds.east / 3600;

    return ll_adjust;
}
