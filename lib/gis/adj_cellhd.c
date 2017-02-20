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
	    G_fatal_error(_("Illegal n-s resolution value <%lf>"), cellhd->ns_res);
    }
    else {
	if (cellhd->rows <= 0)
	    G_fatal_error(_("Illegal row value"));
    }
    if (!col_flag) {
	if (cellhd->ew_res <= 0)
	    G_fatal_error(_("Illegal e-w resolution value"));
    }
    else {
	if (cellhd->cols <= 0)
	    G_fatal_error(_("Illegal col value"));
    }

    /* check the edge values */
    if (cellhd->north <= cellhd->south) {
	if (cellhd->proj == PROJECTION_LL)
	    G_fatal_error(_("North must be north of South"));
	else
	    G_fatal_error(_("North must be larger than South"));
    }

    ll_wrap(cellhd);

    if (cellhd->east <= cellhd->west)
	G_fatal_error(_("East must be larger than West"));

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

    if (cellhd->cols < 0 || cellhd->rows < 0) {
	G_fatal_error(_("Invalid coordinates"));
    }

    /* (re)compute the resolutions */
    old_res = cellhd->ns_res;
    cellhd->ns_res = (cellhd->north - cellhd->south) / cellhd->rows;
    if (old_res > 0 && (old_res - cellhd->ns_res) / old_res > 0.001)
	G_message(_("NS resolution has been changed"));

    old_res = cellhd->ew_res;
    cellhd->ew_res = (cellhd->east - cellhd->west) / cellhd->cols;
    if (old_res > 0 && (old_res - cellhd->ew_res) / old_res > 0.001)
	G_message(_("EW resolution has been changed"));

    if ((cellhd->ns_res - cellhd->ew_res) / cellhd->ns_res > 0.001)
	G_message(_("NS and EW resolutions are different"));

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
	    G_fatal_error(_("Illegal n-s resolution value"));
	if (cellhd->ns_res3 <= 0)
	    G_fatal_error(_("Illegal n-s3 resolution value"));
    }
    else {
	if (cellhd->rows <= 0)
	    G_fatal_error(_("Illegal row value"));
	if (cellhd->rows3 <= 0)
	    G_fatal_error(_("Illegal row3 value"));
    }
    if (!col_flag) {
	if (cellhd->ew_res <= 0)
	    G_fatal_error(_("Illegal e-w resolution value"));
	if (cellhd->ew_res3 <= 0)
	    G_fatal_error(_("Illegal e-w3 resolution value"));
    }
    else {
	if (cellhd->cols <= 0)
	    G_fatal_error(_("Illegal col value"));
	if (cellhd->cols3 <= 0)
	    G_fatal_error(_("Illegal col3 value"));
    }
    if (!depth_flag) {
	if (cellhd->tb_res <= 0)
	    G_fatal_error(_("Illegal t-b3 resolution value"));
    }
    else {
	if (cellhd->depths <= 0)
	    G_fatal_error(_("Illegal depths value"));
    }

    /* check the edge values */
    if (cellhd->north <= cellhd->south) {
	if (cellhd->proj == PROJECTION_LL)
	    G_fatal_error(_("North must be north of South"));
	else
	    G_fatal_error(_("North must be larger than South"));
    }

    ll_wrap(cellhd);

    if (cellhd->east <= cellhd->west)
	G_fatal_error(_("East must be larger than West"));

    if (cellhd->top <= cellhd->bottom)
	G_fatal_error(_("Top must be larger than Bottom"));

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

    if (cellhd->cols < 0 || cellhd->rows < 0 || cellhd->cols3 < 0 ||
	cellhd->rows3 < 0 || cellhd->depths < 0) {
	G_fatal_error(_("Invalid coordinates"));
    }

    /* (re)compute the resolutions */
    old_res = cellhd->ns_res;
    cellhd->ns_res = (cellhd->north - cellhd->south) / cellhd->rows;
    if (old_res > 0 && (old_res - cellhd->ns_res) / old_res > 0.001)
	G_message(_("NS resolution has been changed"));

    old_res = cellhd->ew_res;
    cellhd->ew_res = (cellhd->east - cellhd->west) / cellhd->cols;
    if (old_res > 0 && (old_res - cellhd->ew_res) / old_res > 0.001)
	G_message(_("EW resolution has been changed"));

    if ((cellhd->ns_res - cellhd->ew_res) / cellhd->ns_res > 0.001)
	G_message(_("NS and EW resolutions are different"));

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
    if (cellhd->north > 100.0)
	G_fatal_error(_("Illegal latitude for North: %g"), cellhd->north);
    if (cellhd->south < -100.0)
	G_fatal_error(_("Illegal latitude for South: %g"), cellhd->south);

#if 0
    /* disabled: allow W-E extents larger than 360 degree e.g. for display */
    if (cellhd->west < -370.0) {
	G_debug(1, "East: %g", cellhd->east);
	G_fatal_error(_("Illegal longitude for West: %g"), cellhd->west);
    }
    if (cellhd->east > 370.0) {
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
	G_message(_("NS extent does not match NS resolution: %g cells difference"),
	          diff);
    }

    /* north */
    diff = (cellhd->north - 90) / cellhd->ns_res;
    if (diff < 0)
	diff = -diff;
    if (cellhd->north < 90.0 && diff < 1.0 ) {
	G_message(_("%g cells missing to reach 90 degree north"),
		  diff);
	if (diff < llepsilon && diff > fpepsilon) {
	    G_message(_("Subtle input data rounding error of north boundary (%g)"),
		      cellhd->north - 90.0);
	    /* check only, do not modify
	    cellhd->north = 90.0;
	    lladjust = 1;
	    */
	}
    }
    if (cellhd->north > 90.0) {
	if (diff <= 0.5 + llepsilon) {
	    G_message(_("90 degree north is exceeded by %g cells"),
		      diff);
	    
	    if (diff < llepsilon && diff > fpepsilon) {
		G_message(_("Subtle input data rounding error of north boundary (%g)"),
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
		G_message(_("Subtle input data rounding error of north boundary (%g)"),
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
	G_message(_("%g cells missing to reach 90 degree south"),
		  diff);
	if (diff < llepsilon && diff > fpepsilon) {
	    G_message(_("Subtle input data rounding error of south boundary (%g)"),
		      cellhd->south + 90.0);
	    /* check only, do not modify
	    cellhd->south = -90.0;
	    lladjust = 1;
	    */
	}
    }
    if (cellhd->south < -90.0) {
	if (diff <= 0.5 + llepsilon) {
	    G_message(_("90 degree south is exceeded by %g cells"),
		      diff);
	    
	    if (diff < llepsilon && diff > fpepsilon) {
		G_message(_("Subtle input data rounding error of south boundary (%g)"),
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
		G_message(_("Subtle input data rounding error of south boundary (%g)"),
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
	G_message(_("EW extent does not match EW resolution: %g cells difference"),
	          diff);
    }
    if (cellhd->east - cellhd->west > 360.0) {
	diff = (cellhd->east - cellhd->west - 360.0) / cellhd->ew_res;
	if (diff > fpepsilon)
	    G_message(_("360 degree EW extent is exceeded by %g cells"),
		      diff);
    }
    else if (cellhd->east - cellhd->west < 360.0) {
	diff = (360.0 - (cellhd->east - cellhd->west)) / cellhd->ew_res;
	if (diff < 1.0 && diff > fpepsilon)
	    G_message(_("%g cells missing to cover 360 degrees"),
		  diff);
    }

    return lladjust;
}
