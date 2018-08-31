
/****************************************************************************
 *
 * MODULE:       r.carve
 *
 * AUTHOR(S):    Original author Bill Brown, UIUC GIS Laboratory
 *               Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:      Takes vector stream data, converts it to 3D raster and
 *               subtracts a specified depth
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "enforce.h"


#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif


/* function prototypes */
static void clear_bitmap(struct BM *bm);
static int process_line(struct Map_info *Map, struct Map_info *outMap,
			void *rbuf, const int line, const struct parms *parm);
static void traverse_line_flat(Point2 * pgpts, const int pt, const int npts);
static void traverse_line_noflat(Point2 * pgpts, const double depth,
				 const int pt, const int npts);
static void set_min_point(void *buf, const int col, const int row,
			  const double elev, const double depth,
			  const RASTER_MAP_TYPE rtype);
static double lowest_cell_near_point(void *data, const RASTER_MAP_TYPE rtype,
				     const double px, const double py,
				     const double rad);
static void process_line_segment(const int npts, void *rbuf, Point2 * pgxypts,
				 Point2 * pgpts, struct BM *bm,
				 struct Map_info *outMap,
				 const struct parms *parm);


/******************************************************************
 * Returns 0 on success, -1 on error, 1 on warning, writing message
 * to errbuf for -1 or 1 */

int enforce_downstream(int infd, int outfd,
		       struct Map_info *Map, struct Map_info *outMap,
		       struct parms *parm)
{
    struct Cell_head wind;
    int retval = 0;
    int line, nlines;
    void *rbuf = NULL;

    /* width used here is actually distance to center of stream */
    parm->swidth /= 2;

    G_get_window(&wind);

    Vect_set_constraint_region(Map, wind.north, wind.south, wind.east,
			       wind.west, wind.top, wind.bottom);

    /* allocate and clear memory for entire raster */
    rbuf =
	G_calloc(Rast_window_rows() * Rast_window_cols(),
		 Rast_cell_size(parm->raster_type));

    /* first read whole elevation file into buf */
    read_raster(rbuf, infd, parm->raster_type);

    G_message(_("Processing lines... "));

    nlines = Vect_get_num_lines(Map);
    for (line = 1; line <= nlines; line++)
	retval = process_line(Map, outMap, rbuf, line, parm);

    /* write output raster map */
    write_raster(rbuf, outfd, parm->raster_type);

    G_free(rbuf);

    return retval;
}


static int process_line(struct Map_info *Map, struct Map_info *outMap,
			void *rbuf, const int line, const struct parms *parm)
{
    int i, retval = 0;
    int do_warn = 0;
    int npts = 0;
    int in_out = 0;
    int first_in = -1;
    double totdist = 0.;
    struct Cell_head wind;
    static struct line_pnts *points = NULL;
    static struct line_cats *cats = NULL;
    static struct BM *bm = NULL;
    Point2 *pgpts, *pgxypts;
    PointGrp pg;
    PointGrp pgxy;		/* copy any points in region to this one */

    G_get_window(&wind);

    if (!points)
	points = Vect_new_line_struct();
    if (!cats)
	cats = Vect_new_cats_struct();

    if (!(Vect_read_line(Map, points, cats, line) & GV_LINE))
	return 0;

    if (!bm)
	bm = BM_create(Rast_window_cols(), Rast_window_rows());
    clear_bitmap(bm);

    pg_init(&pg);
    pg_init(&pgxy);

    G_percent(line, Vect_get_num_lines(Map), 10);

    for (i = 0; i < points->n_points; i++) {
	Point2 pt, ptxy;
	double elev;
	int row = Rast_northing_to_row(points->y[i], &wind);
	int col = Rast_easting_to_col(points->x[i], &wind);

	/* rough clipping */
	if (row < 0 || row > Rast_window_rows() - 1 ||
	    col < 0 || col > Rast_window_cols() - 1) {
	    if (first_in != -1)
		in_out = 1;

	    G_debug(1, "outside region - row:%d col:%d", row, col);

	    continue;
	}

	if (first_in < 0)
	    first_in = i;
	else if (in_out)
	    do_warn = 1;

	elev = lowest_cell_near_point(rbuf, parm->raster_type, points->x[i],
				      points->y[i], parm->swidth);

	ptxy[0] = points->x[i];
	ptxy[1] = points->y[i];
	pt[1] = elev;

	/* get distance from this point to previous point */
	if (i)
	    totdist += G_distance(points->x[i - 1], points->y[i - 1],
				  points->x[i], points->y[i]);

	pt[0] = totdist;
	pg_addpt(&pg, pt);
	pg_addpt(&pgxy, ptxy);
	npts++;
    }

    if (do_warn) {
	G_warning(_("Vect runs out of region and re-enters - "
		    "this case is not yet implemented."));
	retval = 1;
    }

    /* now check to see if points go downslope(inorder) or upslope */
    if (pg_y_from_x(&pg, 0.0) > pg_y_from_x(&pg, totdist)) {
	pgpts = pg_getpoints(&pg);
	pgxypts = pg_getpoints(&pgxy);
    }
    else {
	/* pgpts is now high to low */
	pgpts = pg_getpoints_reversed(&pg);

	for (i = 0; i < npts; i++)
	    pgpts[i][0] = totdist - pgpts[i][0];

	pgxypts = pg_getpoints_reversed(&pgxy);
    }

    for (i = 0; i < (npts - 1); i++) {
	if (parm->noflat)
	    /* make sure there are no flat segments in line */
	    traverse_line_noflat(pgpts, parm->sdepth, i, npts);
	else
	    /* ok to have flat segments in line */
	    traverse_line_flat(pgpts, i, npts);
    }

    process_line_segment(npts, rbuf, pgxypts, pgpts, bm, outMap, parm);

    return retval;
}


static void clear_bitmap(struct BM *bm)
{
    int i, j;

    for (i = 0; i < Rast_window_rows(); i++)
	for (j = 0; j < Rast_window_cols(); j++)
	    BM_set(bm, i, j, 0);
}


static void traverse_line_flat(Point2 * pgpts, const int pt, const int npts)
{
    int j, k;

    if (pgpts[pt + 1][1] <= pgpts[pt][1])
	return;

    for (j = (pt + 2); j < npts; j++)
	if (pgpts[j][1] <= pgpts[pt][1])
	    break;

    if (j == npts) {
	/* if we got to the end, level it out */
	for (j = (pt + 1); j < npts; j++)
	    pgpts[j][1] = pgpts[pt][1];
    }
    else {
	/* linear interp between point pt and the next < */
	for (k = (pt + 1); k < j; k++)
	    pgpts[k][1] = LINTERP(pgpts[j][1], pgpts[pt][1],
				  (pgpts[j][0] - pgpts[k][0]) /
				  (pgpts[j][0] - pgpts[pt][0]));
    }
}


static void traverse_line_noflat(Point2 * pgpts, const double depth,
				 const int pt, const int npts)
{
    int j, k;

    if (pgpts[pt + 1][1] < pgpts[pt][1])
	return;

    for (j = (pt + 2); j < npts; j++)
	if (pgpts[j][1] < pgpts[pt][1])
	    break;

    if (j == npts) {
	/* if we got to the end, lower end by depth OR .01 */
	--j;
	pgpts[j][1] = pgpts[pt][1] - (depth > 0 ? depth : 0.01);
    }

    /* linear interp between point pt and the next < */
    for (k = (pt + 1); k < j; k++)
	pgpts[k][1] = LINTERP(pgpts[j][1], pgpts[pt][1],
			      (pgpts[j][0] - pgpts[k][0]) /
			      (pgpts[j][0] - pgpts[pt][0]));
}


/* sets value for a cell */
static void set_min_point(void *data, const int col, const int row,
			  const double elev, const double depth,
			  const RASTER_MAP_TYPE rtype)
{
    switch (rtype) {
    case CELL_TYPE:
	{
	    CELL *cbuf = data;

	    cbuf[row * Rast_window_cols() + col] =
		MIN(cbuf[row * Rast_window_cols() + col], elev) - (int)depth;
	}
	break;
    case FCELL_TYPE:
	{
	    FCELL *fbuf = data;

	    fbuf[row * Rast_window_cols() + col] =
		MIN(fbuf[row * Rast_window_cols() + col], elev) - depth;
	}
	break;
    case DCELL_TYPE:
	{
	    DCELL *dbuf = data;

	    dbuf[row * Rast_window_cols() + col] =
		MIN(dbuf[row * Rast_window_cols() + col], elev) - depth;
	}
	break;
    }
}


/* returns the lowest value cell within radius rad of px, py */
static double lowest_cell_near_point(void *data, const RASTER_MAP_TYPE rtype,
				     const double px, const double py,
				     const double rad)
{
    int r, row, col, row1, row2, col1, col2, rowoff, coloff;
    int rastcols, rastrows;
    double min;
    struct Cell_head wind;

    G_get_window(&wind);
    rastrows = Rast_window_rows();
    rastcols = Rast_window_cols();

    Rast_set_d_null_value(&min, 1);

    /* kludge - fix for lat_lon */
    rowoff = rad / wind.ns_res;
    coloff = rad / wind.ew_res;

    row = Rast_northing_to_row(py, &wind);
    col = Rast_easting_to_col(px, &wind);

    /* get bounding box of line segment */
    row1 = MAX(0, row - rowoff);
    row2 = MIN(rastrows - 1, row + rowoff);
    col1 = MAX(0, col - coloff);
    col2 = MIN(rastcols - 1, col + coloff);

    switch (rtype) {
    case CELL_TYPE:
	{
	    CELL *cbuf = data;

	    if (!(Rast_is_c_null_value(&cbuf[row1 * rastcols + col1])))
		min = cbuf[row1 * rastcols + col1];
	}
	break;
    case FCELL_TYPE:
	{
	    FCELL *fbuf = data;

	    if (!(Rast_is_f_null_value(&fbuf[row1 * rastcols + col1])))
		min = fbuf[row1 * rastcols + col1];
	}
	break;
    case DCELL_TYPE:
	{
	    DCELL *dbuf = data;

	    if (!(Rast_is_d_null_value(&dbuf[row1 * rastcols + col1])))
		min = dbuf[row1 * rastcols + col1];
	}
	break;
    }

    for (r = row1; r < row2; r++) {
	double cy = Rast_row_to_northing(r + 0.5, &wind);
	int c;

	for (c = col1; c < col2; c++) {
	    double cx = Rast_col_to_easting(c + 0.5, &wind);

	    if (G_distance(px, py, cx, cy) <= SQR(rad)) {
		switch (rtype) {
		case CELL_TYPE:
		    {
			CELL *cbuf = data;

			if (Rast_is_d_null_value(&min)) {
			    if (!(Rast_is_c_null_value(&cbuf[r * rastcols + c])))
				min = cbuf[r * rastcols + c];
			}
			else {
			    if (!(Rast_is_c_null_value(&cbuf[r * rastcols + c])))
				if (cbuf[r * rastcols + c] < min)
				    min = cbuf[r * rastcols + c];
			}
		    }
		    break;
		case FCELL_TYPE:
		    {
			FCELL *fbuf = data;

			if (Rast_is_d_null_value(&min)) {
			    if (!(Rast_is_f_null_value(&fbuf[r * rastcols + c])))
				min = fbuf[r * rastcols + c];
			}
			else {
			    if (!(Rast_is_f_null_value(&fbuf[r * rastcols + c])))
				if (fbuf[r * rastcols + c] < min)
				    min = fbuf[r * rastcols + c];
			}
		    }
		    break;
		case DCELL_TYPE:
		    {
			DCELL *dbuf = data;

			if (Rast_is_d_null_value(&min)) {
			    if (!(Rast_is_d_null_value(&dbuf[r * rastcols + c])))
				min = dbuf[r * rastcols + c];
			}
			else {
			    if (!(Rast_is_d_null_value(&dbuf[r * rastcols + c])))
				if (dbuf[r * rastcols + c] < min)
				    min = dbuf[r * rastcols + c];
			}
		    }
		    break;
		}
	    }
	}
    }

    G_debug(3, "min:%.2lf", min);

    return min;
}


/* Now for each segment in the line, use distance from segment 
 * to find beginning row from northernmost point, beginning
 * col from easternmost, ending row & col, then loop through 
 * bounding box and use distance from segment to emboss
 * new elevations */
static void process_line_segment(const int npts, void *rbuf,
				 Point2 * pgxypts, Point2 * pgpts,
				 struct BM *bm, struct Map_info *outMap,
				 const struct parms *parm)
{
    int i, row1, row2, col1, col2;
    int prevrow, prevcol;
    double cellx, celly, cy;
    struct Cell_head wind;
    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();
    int rowoff, coloff;

    G_get_window(&wind);

    Vect_cat_set(cats, 1, 1);

    /* kludge - fix for lat_lon */
    rowoff = parm->swidth / wind.ns_res;
    coloff = parm->swidth / wind.ew_res;

    /* get prevrow and prevcol for iteration 0 of following loop */
    prevrow = Rast_northing_to_row(pgxypts[0][1], &wind);
    prevcol = Rast_easting_to_col(pgxypts[0][0], &wind);

    for (i = 1; i < npts; i++) {
	int c, r;

	int row = Rast_northing_to_row(pgxypts[i][1], &wind);
	int col = Rast_easting_to_col(pgxypts[i][0], &wind);

	/* get bounding box of line segment */
	row1 = MAX(0, MIN(row, prevrow) - rowoff);
	row2 = MIN(Rast_window_rows() - 1, MAX(row, prevrow) + rowoff);
	col1 = MAX(0, MIN(col, prevcol) - coloff);
	col2 = MIN(Rast_window_cols() - 1, MAX(col, prevcol) + coloff);

	for (r = row1; r <= row2; r++) {
	    cy = Rast_row_to_northing(r + 0.5, &wind);

	    for (c = col1; c <= col2; c++) {
		double distance;

		cellx = Rast_col_to_easting(c + 0.5, &wind);
		celly = cy;	/* gets written over in distance2... */

		/* Thought about not going past endpoints (use 
		 * status to check) but then pieces end up missing 
		 * from outside corners - if it goes past ends, 
		 * should probably do some interp or will get flats.
		 * Here we use a bitmap and only change cells once 
		 * on the way down */

		distance = sqrt(dig_distance2_point_to_line(cellx, celly, 0,
						pgxypts[i - 1][0],
						pgxypts[i - 1][1], 0,
						pgxypts[i][0], pgxypts[i][1],
						0, 0, &cellx, &celly, NULL,
						NULL, NULL));

		if (distance <= parm->swidth && !BM_get(bm, c, r)) {
		    double dist, elev;

		    Vect_reset_line(points);

		    dist = G_distance(pgxypts[i][0], pgxypts[i][1],
				      cellx, celly);

		    elev = LINTERP(pgpts[i][1], pgpts[i - 1][1],
				   (dist / (pgpts[i][0] - pgpts[i - 1][0])));

		    BM_set(bm, c, r, 1);

		    /* TODO - may want to use a function for the 
		     * cross section of stream */
		    set_min_point(rbuf, c, r, elev, parm->sdepth,
				  parm->raster_type);

		    /* Add point to output vector map */
		    if (parm->outvect->answer) {
			Vect_append_point(points, cellx, celly,
					  elev - parm->sdepth);
			Vect_write_line(outMap, GV_POINT, points, cats);
		    }
		}
	    }
	}

	prevrow = row;
	prevcol = col;
    }
}
