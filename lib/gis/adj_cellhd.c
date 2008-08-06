
/**
 * \file adj_cellhd.c
 *
 * \brief GIS Library - CELL header adjustment.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <grass/gis.h>
#include <grass/glocale.h>


/**
 * \brief Adjust cell header.
 *
 * This function fills in missing parts of the input
 * cell header (or region).  It also makes projection-specific adjustments. The
 * <b>cellhd</b> structure must have its <i>north, south, east, west</i>,
 * and <i>proj</i> fields set. 
 * 
 * If <b>row_flag</b> is true, then the north-south resolution is computed 
 * from the number of <i>rows</i> in the <b>cellhd</b> structure. Otherwise the number of 
 * <i>rows</i> is computed from the north-south resolution in the structure, similarly for
 * <b>col_flag</b> and the number of columns and the east-west resolution. 
 *
 * <b>Note:</b> 3D values are not adjusted.
 *
 * \param[in,out] cellhd
 * \param[in] row_flag
 * \param[in] col_flag
 * \return NULL on success
 * \return Localized text string on error
 */

char *G_adjust_Cell_head(struct Cell_head *cellhd, int row_flag, int col_flag)
{
    if (!row_flag) {
	if (cellhd->ns_res <= 0)
	    return (_("Illegal n-s resolution value"));
    }
    else {
	if (cellhd->rows <= 0)
	    return (_("Illegal row value"));
    }
    if (!col_flag) {
	if (cellhd->ew_res <= 0)
	    return (_("Illegal e-w resolution value"));
    }
    else {
	if (cellhd->cols <= 0)
	    return (_("Illegal col value"));
    }

    /* for lat/lon, check north,south. force east larger than west */
    if (cellhd->proj == PROJECTION_LL) {
	double epsilon_ns, epsilon_ew;

	/* TODO: find good thresholds */
	epsilon_ns = 1. / cellhd->rows * 0.001;
	epsilon_ew = .000001;	/* epsilon_ew calculation doesn't work due to cellhd->cols update/global wraparound below */

	G_debug(3, "G_adjust_Cell_head: epsilon_ns: %g, epsilon_ew: %g",
		epsilon_ns, epsilon_ew);

	/* TODO: once working, change below G_warning to G_debug */

	/* fix rounding problems if input map slightly exceeds the world definition -180 90 180 -90 */
	if (cellhd->north > 90.0) {
	    if (((cellhd->north - 90.0) < epsilon_ns) &&
		((cellhd->north - 90.0) > GRASS_EPSILON)) {
		G_warning(_("Fixing subtle input data rounding error of north boundary (%g>%g)"),
			  cellhd->north - 90.0, epsilon_ns);
		cellhd->north = 90.0;
	    }
	    else
		return (_("Illegal latitude for North"));
	}

	if (cellhd->south < -90.0) {
	    if (((cellhd->south + 90.0) < epsilon_ns) &&
		((cellhd->south + 90.0) < GRASS_EPSILON)) {
		G_warning(_("Fixing subtle input data rounding error of south boundary (%g>%g)"),
			  cellhd->south + 90.0, epsilon_ns);
		cellhd->south = -90.0;
	    }
	    else
		return (_("Illegal latitude for South"));
	}

#if 0
	/* DISABLED: this breaks global wrap-around */

	G_debug(3,
		"G_adjust_Cell_head()  cellhd->west: %f, devi: %g, eps: %g",
		cellhd->west, cellhd->west + 180.0, epsilon_ew);

	if ((cellhd->west < -180.0) && ((cellhd->west + 180.0) < epsilon_ew)
	    && ((cellhd->west + 180.0) < GRASS_EPSILON)) {
	    G_warning(_("Fixing subtle input data rounding error of west boundary (%g>%g)"),
		      cellhd->west + 180.0, epsilon_ew);
	    cellhd->west = -180.0;
	}

	G_debug(3,
		"G_adjust_Cell_head()  cellhd->east: %f, devi: %g, eps: %g",
		cellhd->east, cellhd->east - 180.0, epsilon_ew);

	if ((cellhd->east > 180.0) && ((cellhd->east - 180.0) > epsilon_ew)
	    && ((cellhd->east - 180.0) > GRASS_EPSILON)) {
	    G_warning(_("Fixing subtle input data rounding error of east boundary (%g>%g)"),
		      cellhd->east - 180.0, epsilon_ew);
	    cellhd->east = 180.0;
	}
#endif

	while (cellhd->east <= cellhd->west)
	    cellhd->east += 360.0;
    }

    /* check the edge values */
    if (cellhd->north <= cellhd->south) {
	if (cellhd->proj == PROJECTION_LL)
	    return (_("North must be north of South"));
	else
	    return (_("North must be larger than South"));
    }
    if (cellhd->east <= cellhd->west)
	return (_("East must be larger than West"));

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
	return (_("Invalid coordinates"));
    }


    /* (re)compute the resolutions */
    cellhd->ns_res = (cellhd->north - cellhd->south) / cellhd->rows;
    cellhd->ew_res = (cellhd->east - cellhd->west) / cellhd->cols;

    return NULL;
}


/**
 * \brief Adjust cell header for 3D values.
 *
 * This function fills in missing parts of the input
 * cell header (or region).  It also makes projection-specific adjustments. The
 * <b>cellhd</b> structure must have its <i>north, south, east, west</i>,
 * and <i>proj</i> fields set. 
 * 
 * If <b>row_flag</b> is true, then the north-south resolution is computed 
 * from the number of <i>rows</i> in the <b>cellhd</b> structure. 
 * Otherwise the number of <i>rows</i> is computed from the north-south 
 * resolution in the structure, similarly for <b>col_flag</b> and the 
 * number of columns and the east-west resolution. 
 *
 * If <b>depth_flag</b> is true, top-bottom resolution is calculated 
 * from depths.
 * If <b>depth_flag</b> are false, number of depths is calculated from 
 * top-bottom resolution.
 *
 * \param[in,out] cellhd
 * \param[in] row_flag
 * \param[in] col_flag
 * \param[in] depth_flag
 * \return NULL on success
 * \return Localized text string on error
 */

char *G_adjust_Cell_head3(struct Cell_head *cellhd, int row_flag,
			  int col_flag, int depth_flag)
{
    if (!row_flag) {
	if (cellhd->ns_res <= 0)
	    return (_("Illegal n-s resolution value"));
	if (cellhd->ns_res3 <= 0)
	    return (_("Illegal n-s3 resolution value"));
    }
    else {
	if (cellhd->rows <= 0)
	    return (_("Illegal row value"));
	if (cellhd->rows3 <= 0)
	    return (_("Illegal row3 value"));
    }
    if (!col_flag) {
	if (cellhd->ew_res <= 0)
	    return (_("Illegal e-w resolution value"));
	if (cellhd->ew_res3 <= 0)
	    return (_("Illegal e-w3 resolution value"));
    }
    else {
	if (cellhd->cols <= 0)
	    return (_("Illegal col value"));
	if (cellhd->cols3 <= 0)
	    return (_("Illegal col3 value"));
    }
    if (!depth_flag) {
	if (cellhd->tb_res <= 0)
	    return (_("Illegal t-b3 resolution value"));
    }
    else {
	if (cellhd->depths <= 0)
	    return (_("Illegal depths value"));
    }

    /* for lat/lon, check north,south. force east larger than west */
    if (cellhd->proj == PROJECTION_LL) {
	double epsilon_ns, epsilon_ew;

	/* TODO: find good thresholds */
	epsilon_ns = 1. / cellhd->rows * 0.001;
	epsilon_ew = .000001;	/* epsilon_ew calculation doesn't work due to cellhd->cols update/global wraparound below */

	G_debug(3, "G_adjust_Cell_head: epsilon_ns: %g, epsilon_ew: %g",
		epsilon_ns, epsilon_ew);

	/* TODO: once working, change below G_warning to G_debug */

	/* fix rounding problems if input map slightly exceeds the world definition -180 90 180 -90 */
	if (cellhd->north > 90.0) {
	    if (((cellhd->north - 90.0) < epsilon_ns) &&
		((cellhd->north - 90.0) > GRASS_EPSILON)) {
		G_warning(_("Fixing subtle input data rounding error of north boundary (%g>%g)"),
			  cellhd->north - 90.0, epsilon_ns);
		cellhd->north = 90.0;
	    }
	    else
		return (_("Illegal latitude for North"));
	}

	if (cellhd->south < -90.0) {
	    if (((cellhd->south + 90.0) < epsilon_ns) &&
		((cellhd->south + 90.0) < GRASS_EPSILON)) {
		G_warning(_("Fixing subtle input data rounding error of south boundary (%g>%g)"),
			  cellhd->south + 90.0, epsilon_ns);
		cellhd->south = -90.0;
	    }
	    else
		return (_("Illegal latitude for South"));
	}

#if 0
	/* DISABLED: this breaks global wrap-around */

	G_debug(3,
		"G_adjust_Cell_head3() cellhd->west: %f, devi: %g, eps: %g",
		cellhd->west, cellhd->west + 180.0, epsilon_ew);

	if ((cellhd->west < -180.0) && ((cellhd->west + 180.0) < epsilon_ew)
	    && ((cellhd->west + 180.0) < GRASS_EPSILON)) {
	    G_warning(_("Fixing subtle input data rounding error of west boundary (%g>%g)"),
		      cellhd->west + 180.0, epsilon_ew);
	    cellhd->west = -180.0;
	}

	G_debug(3,
		"G_adjust_Cell_head3() cellhd->east: %f, devi: %g, eps: %g",
		cellhd->east, cellhd->east - 180.0, epsilon_ew);

	if ((cellhd->east > 180.0) && ((cellhd->east - 180.0) > epsilon_ew)
	    && ((cellhd->east - 180.0) > GRASS_EPSILON)) {
	    G_warning(_("Fixing subtle input data rounding error of east boundary (%g>%g)"),
		      cellhd->east - 180.0, epsilon_ew);
	    cellhd->east = 180.0;
	}
#endif

	while (cellhd->east <= cellhd->west)
	    cellhd->east += 360.0;
    }

    /* check the edge values */
    if (cellhd->north <= cellhd->south) {
	if (cellhd->proj == PROJECTION_LL)
	    return (_("North must be north of South"));
	else
	    return (_("North must be larger than South"));
    }
    if (cellhd->east <= cellhd->west)
	return (_("East must be larger than West"));
    if (cellhd->top <= cellhd->bottom)
	return (_("Top must be larger than Bottom"));


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
	return (_("Invalid coordinates"));
    }

    /* (re)compute the resolutions */
    cellhd->ns_res = (cellhd->north - cellhd->south) / cellhd->rows;
    cellhd->ns_res3 = (cellhd->north - cellhd->south) / cellhd->rows3;
    cellhd->ew_res = (cellhd->east - cellhd->west) / cellhd->cols;
    cellhd->ew_res3 = (cellhd->east - cellhd->west) / cellhd->cols3;
    cellhd->tb_res = (cellhd->top - cellhd->bottom) / cellhd->depths;

    return NULL;
}
