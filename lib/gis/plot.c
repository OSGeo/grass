
/*****************************************************************
 * Plot lines and filled polygons. Input space is database window.
 * Output space and output functions are user defined.
 * Converts input east,north lines and polygons to output x,y
 * and calls user supplied line drawing routines to do the plotting.
 *
 * Handles global wrap-around for lat-lon databases.
 *
 * Does not perform window clipping.
 * Clipping must be done by the line draw routines supplied by the user.
 *
 * Note:
 *  Hopefully, cartographic style projection plotting will be added later.
 *******************************************************************/
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>

static double xconv, yconv;
static double left, right, top, bottom;
static int ymin, ymax;
static struct Cell_head window;
static int fastline(double, double, double, double);
static int slowline(double, double, double, double);
static int plot_line(double, double, double, double, int (*)());
static double nearest(double, double);
static int edge(double, double, double, double);
static int edge_point(double, int);

#define POINT struct point
POINT {
    double x;
    int y;
};
static int edge_order(const void *, const void *);
static int row_solid_fill(int, double, double);
static int row_dotted_fill(int, double, double);
static int dotted_fill_gap = 2;
static int ifloor(double);
static int iceil(double);
static int (*row_fill) () = row_solid_fill;
static int (*move) (int, int);
static int (*cont) (int, int);

/*!
 * \brief returns east larger than west
 *
 * If the region projection is
 * PROJECTION_LL, then this routine returns an equivalent <b>east</b> that is
 * larger, but no more than 360 degrees larger, than the coordinate for the
 * western edge of the region. Otherwise no adjustment is made and the original
 * <b>east</b> is returned.
 *
 *  \param east
 *  \param region
 *  \return double
 */

/*
 * G_setup_plot (t, b, l, r, Move, Cont)
 *     double t, b, l, r;
 *     int (*Move)(), (*Cont)();
 *
 * initialize the plotting capability.
 *    t,b,l,r:   top, bottom, left, right of the output x,y coordinate space.
 *    Move,Cont: subroutines that will draw lines in x,y space.
 *       Move(x,y)   move to x,y (no draw)
 *       Cont(x,y)   draw from previous position to x,y
 * Notes:
 *   Cont() is responsible for clipping.
 *   The t,b,l,r are only used to compute coordinate transformations.
 *   The input space is assumed to be the current GRASS window.
 */

/*!
 * \brief initialize plotting routines
 *
 * Initializes the plotting
 * capability. This routine must be called once before calling the
 * <b>G_plot_*(~)</b> routines described below.
 * The parameters <b>t, b, l, r</b> are the top, bottom, left, and right of the
 * output x,y coordinate space. They are not integers, but doubles to allow for
 * subpixel registration of the input and output coordinate spaces. The input
 * coordinate space is assumed to be the current GRASS region, and the routines
 * supports both planimetric and latitude- longitude coordinate systems.
 * <b>Move</b> and <b>Cont</b> are subroutines that will draw lines in x,y
 * space. They will be called as follows:
 * Move(x, y) move to x,y (no draw)
 * Cont(x, y) draw from previous position
 * to x,y. Cont(~) is responsible for clipping
 *
 *  \param ~
 *  \return int
 */

int G_setup_plot(double t, double b, double l, double r,
		 int (*Move) (int, int), int (*Cont) (int, int))
{
    G_get_set_window(&window);

    left = l;
    right = r;
    top = t;
    bottom = b;

    xconv = (right - left) / (window.east - window.west);
    yconv = (bottom - top) / (window.north - window.south);

    if (top < bottom) {
	ymin = iceil(top);
	ymax = ifloor(bottom);
    }
    else {
	ymin = iceil(bottom);
	ymax = ifloor(top);
    }

    move = Move;
    cont = Cont;

    return 0;
}

/*!
 * \brief set row_fill routine to row_solid_fill or row_dotted_fill
 *
 * After calling this function, <b>G_plot_polygon()</b> and
 * <b>G_plot_area()</b> fill shapes with solid or dotted lines.  If gap is
 * greater than zero, this value will be used for row_dotted_fill.  Otherwise,
 * row_solid_fill is used.
 *
 *  \param int
 *  \return int
 */
int G_setup_fill(int gap)
{
    if (gap > 0) {
	row_fill = row_dotted_fill;
	dotted_fill_gap = gap + 1;
    }
    else
	row_fill = row_solid_fill;

    return 0;
}

#define X(e) (left + xconv * ((e) - window.west))
#define Y(n) (top + yconv * (window.north - (n)))

#define EAST(x) (window.west + ((x)-left)/xconv)
#define NORTH(y) (window.north - ((y)-top)/yconv)


/*!
 * \brief east,north to x,y
 *
 * The map coordinates <b>east,north</b> are converted
 * to pixel coordinates <b>x,y.</b>
 *
 *  \param east
 *  \param north
 *  \param x
 *  \param y
 *  \return int
 */

int G_plot_where_xy(double east, double north, int *x, int *y)
{
    *x = ifloor(X(G_adjust_easting(east, &window)) + 0.5);
    *y = ifloor(Y(north) + 0.5);

    return 0;
}


/*!
 * \brief x,y to east,north
 *
 * The pixel coordinates <b>x,y</b> are converted to map
 * coordinates <b>east,north.</b>
 *
 *  \param x
 *  \param y
 *  \param east
 *  \param north
 *  \return int
 */

int G_plot_where_en(int x, int y, double *east, double *north)
{
    *east = G_adjust_easting(EAST(x), &window);
    *north = NORTH(y);

    return 0;
}

int G_plot_point(double east, double north)
{
    int x, y;

    G_plot_where_xy(east, north, &x, &y);
    move(x, y);
    cont(x, y);

    return 0;
}

/*
 * Line in map coordinates is plotted in output x,y coordinates
 * This routine handles global wrap-around for lat-long databses.
 *
 */

/*!
 * \brief plot line between latlon coordinates
 *
 * A line from <b>east1,north1</b>
 * to <b>east2,north2</b> is plotted in output x,y coordinates (e.g. pixels for
 * graphics.) This routine handles global wrap-around for latitude-longitude
 * databases.
 *
 *  \param east1
 *  \param north1
 *  \param east2
 *  \param north2
 *  \return int
 */

int G_plot_line(double east1, double north1, double east2, double north2)
{
    return plot_line(east1, north1, east2, north2, fastline);
}

int G_plot_line2(double east1, double north1, double east2, double north2)
{
    return plot_line(east1, north1, east2, north2, slowline);
}

/* fastline converts double rows/cols to ints then plots
 * this is ok for graphics, but not the best for vector to raster
 */

static int fastline(double x1, double y1, double x2, double y2)
{
    move(ifloor(x1 + 0.5), ifloor(y1 + 0.5));
    cont(ifloor(x2 + 0.5), ifloor(y2 + 0.5));

    return 0;
}

/* NOTE (shapiro): 
 *   I think the adding of 0.5 in slowline is not correct
 *   the output window (left, right, top, bottom) should already
 *   be adjusted for this: left=-0.5; right = window.cols-0.5;
 */

static int slowline(double x1, double y1, double x2, double y2)
{
    double dx, dy;
    double m, b;
    int xstart, xstop, ystart, ystop;

    dx = x2 - x1;
    dy = y2 - y1;

    if (fabs(dx) > fabs(dy)) {
	m = dy / dx;
	b = y1 - m * x1;

	if (x1 > x2) {
	    xstart = iceil(x2 - 0.5);
	    xstop = ifloor(x1 + 0.5);
	}
	else {
	    xstart = iceil(x1 - 0.5);
	    xstop = ifloor(x2 + 0.5);
	}
	if (xstart <= xstop) {
	    ystart = ifloor(m * xstart + b + 0.5);
	    move(xstart, ystart);
	    while (xstart <= xstop) {
		cont(xstart++, ystart);
		ystart = ifloor(m * xstart + b + 0.5);
	    }
	}
    }
    else {
	if (dx == dy)		/* they both might be 0 */
	    m = 1.;
	else
	    m = dx / dy;
	b = x1 - m * y1;

	if (y1 > y2) {
	    ystart = iceil(y2 - 0.5);
	    ystop = ifloor(y1 + 0.5);
	}
	else {
	    ystart = iceil(y1 - 0.5);
	    ystop = ifloor(y2 + 0.5);
	}
	if (ystart <= ystop) {
	    xstart = ifloor(m * ystart + b + 0.5);
	    move(xstart, ystart);
	    while (ystart <= ystop) {
		cont(xstart, ystart++);
		xstart = ifloor(m * ystart + b + 0.5);
	    }
	}
    }

    return 0;
}

static int plot_line(double east1, double north1, double east2, double north2,
		     int (*line) (double, double, double, double))
{
    double x1, x2, y1, y2;

    y1 = Y(north1);
    y2 = Y(north2);

    if (window.proj == PROJECTION_LL) {
	if (east1 > east2)
	    while ((east1 - east2) > 180)
		east2 += 360;
	else if (east2 > east1)
	    while ((east2 - east1) > 180)
		east1 += 360;
	while (east1 > window.east) {
	    east1 -= 360.0;
	    east2 -= 360.0;
	}
	while (east1 < window.west) {
	    east1 += 360.0;
	    east2 += 360.0;
	}
	x1 = X(east1);
	x2 = X(east2);

	line(x1, y1, x2, y2);

	if (east2 > window.east || east2 < window.west) {
	    while (east2 > window.east) {
		east1 -= 360.0;
		east2 -= 360.0;
	    }
	    while (east2 < window.west) {
		east1 += 360.0;
		east2 += 360.0;
	    }
	    x1 = X(east1);
	    x2 = X(east2);
	    line(x1, y1, x2, y2);
	}
    }
    else {
	x1 = X(east1);
	x2 = X(east2);
	line(x1, y1, x2, y2);
    }

    return 0;
}

/*
 * G_plot_polygon (x, y, n)
 * 
 *    double *x       x coordinates of vertices
 *    double *y       y coordinates of vertices
 *    int n           number of verticies
 *
 * polygon fill from map coordinate space to plot x,y space.
 * for lat-lon, handles global wrap-around as well as polar polygons.
 *
 * returns 0 ok, 2 n<3, -1 weird internal error, 1 no memory
 */

static POINT *P;
static int np;
static int npalloc = 0;

#define OK 0
#define TOO_FEW_EDGES 2
#define NO_MEMORY 1
#define OUT_OF_SYNC -1

static double nearest(double e0, double e1)
{
    while (e0 - e1 > 180)
	e1 += 360.0;
    while (e1 - e0 > 180)
	e1 -= 360.0;

    return e1;
}


/*!
 * \brief plot filled polygon with n vertices
 *
 * The polygon, described by the <b>n</b> vertices
 * <b>east,north</b>, is plotted in the output x,y space as a filled polygon.
 *
 *  \param east
 *  \param north
 *  \param n
 *  \return int
 */

int G_plot_polygon(const double *x, const double *y, int n)
{
    int i;
    int pole;
    double x0, x1;
    double y0, y1;
    double shift, E, W = 0L;
    double e0, e1;
    int shift1, shift2;

    if (n < 3)
	return TOO_FEW_EDGES;

    /* traverse the perimeter */

    np = 0;
    shift1 = 0;

    /* global wrap-around for lat-lon, part1 */
    if (window.proj == PROJECTION_LL) {
	/*
	   pole = G_pole_in_polygon(x,y,n);
	 */
	pole = 0;

	e0 = x[n - 1];
	E = W = e0;

	x0 = X(e0);
	y0 = Y(y[n - 1]);

	if (pole && !edge(x0, y0, x0, Y(90.0 * pole)))
	    return NO_MEMORY;

	for (i = 0; i < n; i++) {
	    e1 = nearest(e0, x[i]);
	    if (e1 > E)
		E = e1;
	    if (e1 < W)
		W = e1;

	    x1 = X(e1);
	    y1 = Y(y[i]);

	    if (!edge(x0, y0, x1, y1))
		return NO_MEMORY;

	    x0 = x1;
	    y0 = y1;
	    e0 = e1;
	}
	if (pole && !edge(x0, y0, x0, Y(90.0 * pole)))
	    return NO_MEMORY;

	shift = 0;		/* shift into window */
	while (E + shift > window.east)
	    shift -= 360.0;
	while (E + shift < window.west)
	    shift += 360.0;
	shift1 = X(x[n - 1] + shift) - X(x[n - 1]);
    }
    else {
	x0 = X(x[n - 1]);
	y0 = Y(y[n - 1]);

	for (i = 0; i < n; i++) {
	    x1 = X(x[i]);
	    y1 = Y(y[i]);
	    if (!edge(x0, y0, x1, y1))
		return NO_MEMORY;
	    x0 = x1;
	    y0 = y1;
	}
    }

    /* check if perimeter has odd number of points */
    if (np % 2)
	return OUT_OF_SYNC;

    /* sort the edge points by col(x) and then by row(y) */
    qsort(P, np, sizeof(POINT), &edge_order);

    /* plot */
    for (i = 1; i < np; i += 2) {
	if (P[i].y != P[i - 1].y)
	    return OUT_OF_SYNC;
	row_fill(P[i].y, P[i - 1].x + shift1, P[i].x + shift1);
    }
    if (window.proj == PROJECTION_LL) {	/* now do wrap-around, part 2 */
	shift = 0;
	while (W + shift < window.west)
	    shift += 360.0;
	while (W + shift > window.east)
	    shift -= 360.0;
	shift2 = X(x[n - 1] + shift) - X(x[n - 1]);
	if (shift2 != shift1) {
	    for (i = 1; i < np; i += 2) {
		row_fill(P[i].y, P[i - 1].x + shift2, P[i].x + shift2);
	    }
	}
    }
    return OK;
}

/*
 * G_plot_area (xs, ys, rpnts, rings)
 *      double **xs;  -- pointer to pointer for X's
 *      double **ys;  -- pointer to pointer for Y's
 *      int *rpnts;   -- array of ints w/ num points per ring
 *      int rings;    -- number of rings
 *
 * Essentially a copy of G_plot_polygon, with minor mods to
 * handle a set of polygons.  return values are the same.
 */

/*!
 * \brief plot multiple polygons
 *
 * Like G_plot_polygon, except it takes a set of polygons,
 * each with \textbf{npts[<i>i</i>]} vertices, where the number of polygons 
 * is specified with the <b>rings</b> argument.  It is especially useful for 
 * plotting vector areas with interior islands.
 *
 *  \param xs
 *  \param ys
 *  \param npts
 *  \param rings
 *  \return int
 */

int G_plot_area(double *const *xs, double *const *ys, int *rpnts, int rings)
{
    int i, j, n;
    int pole;
    double x0, x1, *x;
    double y0, y1, *y;
    double shift, E, W = 0L;
    double e0, e1;
    int *shift1 = NULL, shift2;

    /* traverse the perimeter */

    np = 0;
    shift1 = (int *)G_calloc(sizeof(int), rings);

    for (j = 0; j < rings; j++) {
	n = rpnts[j];

	if (n < 3)
	    return TOO_FEW_EDGES;

	x = xs[j];
	y = ys[j];

	/* global wrap-around for lat-lon, part1 */
	if (window.proj == PROJECTION_LL) {
	    /*
	       pole = G_pole_in_polygon(x,y,n);
	     */
	    pole = 0;

	    e0 = x[n - 1];
	    E = W = e0;

	    x0 = X(e0);
	    y0 = Y(y[n - 1]);

	    if (pole && !edge(x0, y0, x0, Y(90.0 * pole)))
		return NO_MEMORY;

	    for (i = 0; i < n; i++) {
		e1 = nearest(e0, x[i]);
		if (e1 > E)
		    E = e1;
		if (e1 < W)
		    W = e1;

		x1 = X(e1);
		y1 = Y(y[i]);

		if (!edge(x0, y0, x1, y1))
		    return NO_MEMORY;

		x0 = x1;
		y0 = y1;
		e0 = e1;
	    }
	    if (pole && !edge(x0, y0, x0, Y(90.0 * pole)))
		return NO_MEMORY;

	    shift = 0;		/* shift into window */
	    while (E + shift > window.east)
		shift -= 360.0;
	    while (E + shift < window.west)
		shift += 360.0;
	    shift1[j] = X(x[n - 1] + shift) - X(x[n - 1]);
	}
	else {
	    x0 = X(x[n - 1]);
	    y0 = Y(y[n - 1]);

	    for (i = 0; i < n; i++) {
		x1 = X(x[i]);
		y1 = Y(y[i]);
		if (!edge(x0, y0, x1, y1))
		    return NO_MEMORY;
		x0 = x1;
		y0 = y1;
	    }
	}
    }				/* for() */

    /* check if perimeter has odd number of points */
    if (np % 2)
	return OUT_OF_SYNC;

    /* sort the edge points by col(x) and then by row(y) */
    qsort(P, np, sizeof(POINT), &edge_order);

    /* plot */
    for (j = 0; j < rings; j++) {
	for (i = 1; i < np; i += 2) {
	    if (P[i].y != P[i - 1].y)
		return OUT_OF_SYNC;
	    row_fill(P[i].y, P[i - 1].x + shift1[j], P[i].x + shift1[j]);
	}
	if (window.proj == PROJECTION_LL) {	/* now do wrap-around, part 2 */
	    n = rpnts[j];
	    x = xs[j];
	    y = ys[j];

	    shift = 0;
	    while (W + shift < window.west)
		shift += 360.0;
	    while (W + shift > window.east)
		shift -= 360.0;
	    shift2 = X(x[n - 1] + shift) - X(x[n - 1]);
	    if (shift2 != shift1[j]) {
		for (i = 1; i < np; i += 2) {
		    row_fill(P[i].y, P[i - 1].x + shift2, P[i].x + shift2);
		}
	    }
	}
    }
    G_free(shift1);
    return OK;

}

static int edge(double x0, double y0, double x1, double y1)
{
    register double m;
    double dy, x;
    int ystart, ystop;


    /* tolerance to avoid FPE */
    dy = y0 - y1;
    if (fabs(dy) < 1e-10)
	return 1;

    m = (x0 - x1) / dy;

    if (y0 < y1) {
	ystart = iceil(y0);
	ystop = ifloor(y1);
	if (ystop == y1)
	    ystop--;		/* if line stops at row center, don't include point */
    }
    else {
	ystart = iceil(y1);
	ystop = ifloor(y0);
	if (ystop == y0)
	    ystop--;		/* if line stops at row center, don't include point */
    }
    if (ystart > ystop)
	return 1;		/* does not cross center line of row */

    x = m * (ystart - y0) + x0;
    while (ystart <= ystop) {
	if (!edge_point(x, ystart++))
	    return 0;
	x += m;
    }
    return 1;
}

static int edge_point(double x, int y)
{

    if (y < ymin || y > ymax)
	return 1;
    if (np >= npalloc) {
	if (npalloc > 0) {
	    npalloc *= 2;
	    P = (POINT *) G_realloc(P, npalloc * sizeof(POINT));
	}
	else {
	    npalloc = 32;
	    P = (POINT *) G_malloc(npalloc * sizeof(POINT));
	}
	if (P == NULL) {
	    npalloc = 0;
	    return 0;
	}
    }
    P[np].x = x;
    P[np++].y = y;
    return 1;
}

static int edge_order(const void *aa, const void *bb)
{
    const struct point *a = aa, *b = bb;

    if (a->y < b->y)
	return (-1);
    if (a->y > b->y)
	return (1);

    if (a->x < b->x)
	return (-1);
    if (a->x > b->x)
	return (1);

    return (0);
}

static int row_solid_fill(int y, double x1, double x2)
{
    int i1, i2;

    i1 = iceil(x1);
    i2 = ifloor(x2);
    if (i1 <= i2) {
	move(i1, y);
	cont(i2, y);
    }

    return 0;
}

static int row_dotted_fill(int y, double x1, double x2)
{
    int i1, i2, i;

    if (y != iceil(y / dotted_fill_gap) * dotted_fill_gap)
	return 0;

    i1 = iceil(x1 / dotted_fill_gap) * dotted_fill_gap;
    i2 = ifloor(x2);
    if (i1 <= i2) {
	for (i = i1; i <= i2; i += dotted_fill_gap) {
	    move(i, y);
	    cont(i, y);
	}
    }

    return 0;
}

static int ifloor(double x)
{
    int i;

    i = (int)x;
    if (i > x)
	i--;
    return i;
}

static int iceil(double x)
{
    int i;

    i = (int)x;
    if (i < x)
	i++;
    return i;
}

/*
 * G_plot_fx(e1,e2)
 *
 * plot f(x) from x=e1 to x=e2
 */


/*!
 * \brief plot f(east1) to f(east2)
 *
 * The function <b>f(east)</b> is plotted from
 * <b>east1</b> to <b>east2.</b> The function <b>f(east)</b> must return
 * the map northing coordinate associated with east.
 *
 *  \param ~
 *  \return int
 */

int G_plot_fx(double (*f) (double), double east1, double east2)
{
    double east, north, north1;
    double incr;


    incr = fabs(1.0 / xconv);

    east = east1;
    north = f(east1);

    if (east1 > east2) {
	while ((east1 -= incr) > east2) {
	    north1 = f(east1);
	    G_plot_line(east, north, east1, north1);
	    north = north1;
	    east = east1;
	}
    }
    else {
	while ((east1 += incr) < east2) {
	    north1 = f(east1);
	    G_plot_line(east, north, east1, north1);
	    north = north1;
	    east = east1;
	}
    }
    G_plot_line(east, north, east2, f(east2));

    return 0;
}
