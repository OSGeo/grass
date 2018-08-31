#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>

/****** OLD CODE
* #include "windround.h"
**********/
/*  D_do_conversions(window, t, b, l, r)
 *       struct Cell_head *window ;
 *       int t, b, l, r ;
 *
 *  Sets up conversion coefficients to translate between three 
 *  coordinate systems:
 *
 *  1.  Screen coordinates   (given by t, b, l, r values)
 *  2.  UTM coordinates      (given by values in window structure)
 *  3.  Window array coors   (given by values in window structure)
 *
 *  Once D_do_conversions is called, lots of conversion coefficients
 *  and conversion routines are available.
 *
 *  Calls to convert row and column (x and y) values in one system to
 *  another system are available.  In addition calls which return the
 *  conversion coefficients are alos provided.
 */

struct vector
{
    double x, y;
};

struct rect
{
    double west;
    double east;
    double south;
    double north;
    struct vector size;
};

/* Bounding rectangles */
static struct rect D;	/* Display coordinates, pixels, (0,0) towards NW */
static struct rect A;	/* Map array coordinates, integers, (0,0) towards NW */
static struct rect U;	/* UTM coordinates, meters, (0,0) towards SW */

/* Conversion factors */
static struct vector D_to_A_conv;	/* Display to Array */
static struct vector A_to_U_conv;	/* Array to UTM     */
static struct vector U_to_D_conv;	/* UTM to Display   */

/* others */
static int is_lat_lon;


static void calc_size(struct rect *rect)
{
    rect->size.x = rect->east  - rect->west;
    rect->size.y = rect->south - rect->north;
}

static void calc_conv(struct vector *conv, 
		      const struct vector *src, const struct vector *dst)
{
    conv->x = dst->x / src->x;
    conv->y = dst->y / src->y;
}

static void fit_aspect(struct rect *rect, const struct rect *ref)
{
    struct vector conv;
    double scale, size, delta;

    calc_conv(&conv, &rect->size, &ref->size);

    if (fabs(conv.y) > fabs(conv.x)) {
	scale = fabs(conv.y) / fabs(conv.x);
	size = rect->size.x / scale;
	delta = rect->size.x - size;
	rect->west += delta/2;
	rect->east -= delta/2;
	rect->size.x = size;
    }
    else {
	scale = fabs(conv.x) / fabs(conv.y);
	size = rect->size.y / scale;
	delta = rect->size.y - size;
	rect->north += delta/2;
	rect->south -= delta/2;
	rect->size.y = size;
    }
}

void D_update_conversions(void)
{
    calc_conv(&D_to_A_conv, &D.size, &A.size);
    calc_conv(&A_to_U_conv, &A.size, &U.size);
    calc_conv(&U_to_D_conv, &U.size, &D.size);
}

void D_fit_d_to_u(void)
{
    fit_aspect(&D, &U);
}

void D_fit_u_to_d(void)
{
    fit_aspect(&U, &D);
}

void D_show_conversions(void)
{
    fprintf(stderr,
	    " D_w %10.1f  D_e %10.1f  D_s %10.1f  D_n %10.1f\n",
	    D.west, D.east, D.south, D.north);
    fprintf(stderr,
	    " A_w %10.1f  A_e %10.1f  A_s %10.1f  A_n %10.1f\n",
	    A.west, A.east, A.south, A.north);
    fprintf(stderr,
	    " U_w %10.1f  U_e %10.1f  U_s %10.1f  U_n %10.1f\n",
	    U.west, U.east, U.south, U.north);

    fprintf(stderr,
	    " D_x %10.1f  D_y %10.1f\n" "\n", D.size.x, D.size.y);
    fprintf(stderr,
	    " A_x %10.1f  A_y %10.1f\n" "\n", A.size.x, A.size.y);
    fprintf(stderr,
	    " U_x %10.1f  U_y %10.1f\n" "\n", U.size.x, U.size.y);

    fprintf(stderr, " D_to_A_conv.x %10.1f D_to_A_conv.y %10.1f \n",
	    D_to_A_conv.x, D_to_A_conv.y);
    fprintf(stderr, " A_to_U_conv.x %10.1f A_to_U_conv.y %10.1f \n",
	    A_to_U_conv.x, A_to_U_conv.y);
    fprintf(stderr, " U_to_D_conv.x %10.1f U_to_D_conv.y %10.1f \n",
	    U_to_D_conv.x, U_to_D_conv.y);
}

/*!
 * \brief initialize conversions
 *
 * The relationship between the earth <b>region</b> and the <b>top, bottom,
 * left</b>, and <b>right</b> screen coordinates is established, which then
 * allows conversions between all three coordinate systems to be performed.
 * Note this routine is called by <i>D_setup</i>.
 *
 *  \param window region
 *  \param t top
 *  \param b bottom
 *  \param l left
 *  \param r right
 *  \return none
 */
void D_do_conversions(const struct Cell_head *window,
		      double t, double b, double l, double r)
{
    D_set_region(window);
    D_set_dst(t, b, l, r);
    D_fit_d_to_u();
    D_update_conversions();
#ifdef DEBUG
    D_show_conversions();
#endif /* DEBUG */
}


int D_is_lat_lon(void)			{    return (is_lat_lon);		}

double D_get_d_to_a_xconv(void)		{    return (D_to_A_conv.x);		}
double D_get_d_to_a_yconv(void)		{    return (D_to_A_conv.y);		}
double D_get_d_to_u_xconv(void)		{    return (1/U_to_D_conv.x);		}
double D_get_d_to_u_yconv(void)		{    return (1/U_to_D_conv.y);		}
double D_get_a_to_u_xconv(void)		{    return (A_to_U_conv.x);		}
double D_get_a_to_u_yconv(void)		{    return (A_to_U_conv.y);		}
double D_get_a_to_d_xconv(void)		{    return (1/D_to_A_conv.x);		}
double D_get_a_to_d_yconv(void)		{    return (1/D_to_A_conv.y);		}
double D_get_u_to_d_xconv(void)		{    return (U_to_D_conv.x);		}
double D_get_u_to_d_yconv(void)		{    return (U_to_D_conv.y);		}
double D_get_u_to_a_xconv(void)		{    return (1/A_to_U_conv.x);		}
double D_get_u_to_a_yconv(void)		{    return (1/A_to_U_conv.y);		}

double D_get_ns_resolution(void)	{    return D_get_a_to_u_yconv();	}
double D_get_ew_resolution(void)	{    return D_get_a_to_u_xconv();	}

double D_get_u_west(void)		{    return (U.west);			}
double D_get_u_east(void)		{    return (U.east);			}
double D_get_u_north(void)		{    return (U.north);			}
double D_get_u_south(void)		{    return (U.south);			}

double D_get_a_west(void)		{    return (A.west);			}
double D_get_a_east(void)		{    return (A.east);			}
double D_get_a_north(void)		{    return (A.north);			}
double D_get_a_south(void)		{    return (A.south);			}

double D_get_d_west(void)		{    return (D.west);			}
double D_get_d_east(void)		{    return (D.east);			}
double D_get_d_north(void)		{    return (D.north);			}
double D_get_d_south(void)		{    return (D.south);			}

void D_set_region(const struct Cell_head *window)
{
    D_set_src(window->north, window->south, window->west, window->east);
    D_set_grid(0, window->rows, 0, window->cols);
    is_lat_lon = (window->proj == PROJECTION_LL);
}

void D_set_src(double t, double b, double l, double r)
{
    U.north = t;
    U.south = b;
    U.west  = l;
    U.east  = r;
    calc_size(&U);
}

/*!
 * \brief returns frame bounds in source coordinate system
 *
 * D_get_src() returns the frame bounds in the source coordinate system
 * (used by D_* functions)
 *
 *  \param t top
 *  \param b bottom
 *  \param l left
 *  \param r right
 *  \return void
 */
void D_get_src(double *t, double *b, double *l, double *r)
{
    *t = U.north;
    *b = U.south;
    *l = U.west;
    *r = U.east;
}

void D_set_grid(int t, int b, int l, int r)
{
    A.north = t;
    A.south = b;
    A.west  = l;
    A.east  = r;
    calc_size(&A);
}

void D_get_grid(int *t, int *b, int *l, int *r)
{
    *t = A.north;
    *b = A.south;
    *l = A.west;
    *r = A.east;
}

void D_set_dst(double t, double b, double l, double r)
{
    D.north = t;
    D.south = b;
    D.west  = l;
    D.east  = r;
    calc_size(&D);
}

/*!
 * \brief returns frame bounds in destination coordinate system
 *
 * D_get_dst() returns the frame bounds in the destination coordinate system
 * (used by R_* commands).
 * The various D_setup() commands all set the destination coordinate
 * system to the current frame reported by R_get_window().
 *
 *  \param t top
 *  \param b bottom
 *  \param l left
 *  \param r right
 *  \return none
 */
void D_get_dst(double *t, double *b, double *l, double *r)
{
    *t = D.north;
    *b = D.south;
    *l = D.west;
    *r = D.east;
}

void D_get_u(double x[2][2])
{
    x[0][0] = U.west;
    x[0][1] = U.east;
    x[1][0] = U.north;
    x[1][1] = U.south;
}

void D_get_a(int x[2][2])
{
    x[0][0] = (int)A.west;
    x[0][1] = (int)A.east;
    x[1][0] = (int)A.north;
    x[1][1] = (int)A.south;
}

void D_get_d(double x[2][2])
{
    x[0][0] = D.west;
    x[0][1] = D.east;
    x[1][0] = D.north;
    x[1][1] = D.south;
}

/*!
 * \brief screen to array (y)
 *
 * Returns a <i>row</i> value in the array coordinate system when provided the
 * corresponding <b>y</b> value in the screen coordinate system.
 *
 *  \param D_row y
 *  \return double
 */

double D_d_to_a_row(double D_row)
{
    return A.north + (D_row - D.north) * D_to_A_conv.y;
}


/*!
 * \brief screen to array (x)
 *
 * Returns a <i>column</i> value in the array coordinate system when provided the
 * corresponding <b>x</b> value in the screen coordinate system.
 *
 *  \param D_col x
 *  \return double
 */

double D_d_to_a_col(double D_col)
{
    return A.west + (D_col - D.west) * D_to_A_conv.x;
}


/*!
 * \brief screen to earth (y)
 *
 * Returns a <i>north</i> value in the earth coordinate system when provided the
 * corresponding <b>y</b> value in the screen coordinate system.
 *
 *  \param D_row y
 *  \return double
 */

double D_d_to_u_row(double D_row)
{
    return U.north + (D_row - D.north) / U_to_D_conv.y;
}


/*!
 * \brief screen to earth (x)
 *
 * Returns an <i>east</i> value in the earth coordinate system when provided the
 * corresponding <b>x</b> value in the screen coordinate system.
 *
 *  \param D_col x
 *  \return double
 */

double D_d_to_u_col(double D_col)
{
    return U.west + (D_col - D.west) / U_to_D_conv.x;
}


/*!
 * \brief array to earth (row)
 *
 * Returns a <i>y</i> value in the earth coordinate system when provided the
 * corresponding <b>row</b> value in the array coordinate system.
 *
 *  \param A_row row
 *  \return double
 */

double D_a_to_u_row(double A_row)
{
    return U.north + (A_row - A.north) * A_to_U_conv.y;
}


/*!
 * \brief array to earth (column)
 *
 * Returns an <i>x</i> value in the earth coordinate system when
 * provided the corresponding <b>column</b> value in the array coordinate
 * system.
 *
 *  \param A_col column
 *  \return double
 */

double D_a_to_u_col(double A_col)
{
    return U.west + (A_col - A.west) * A_to_U_conv.x;
}


/*!
 * \brief array to screen (row)
 *
 * Returns a <i>y</i> value in the screen coordinate system when provided the
 * corresponding <b>row</b> value in the array coordinate system.
 *
 *  \param A_row row
 *  \return double
 */

double D_a_to_d_row(double A_row)
{
    return D.north + (A_row - A.north) / D_to_A_conv.y;
}


/*!
 * \brief array to screen (column)
 *
 * Returns an <i>x</i> value in the screen coordinate system when
 * provided the corresponding <b>column</b> value in the array coordinate
 * system.
 *
 *  \param A_col column
 *  \return double
 */

double D_a_to_d_col(double A_col)
{
    return D.west + (A_col - A.west) / D_to_A_conv.x;
}


/*!
 * \brief earth to screen (north)
 *
 * Returns a <i>y</i> value in the screen coordinate system when provided the
 * corresponding <b>north</b> value in the earth coordinate system.
 *
 *  \param U_row north
 *  \return double
 */

double D_u_to_d_row(double U_row)
{
    return D.north + (U_row - U.north) * U_to_D_conv.y;
}


/*!
 * \brief earth to screen (east)
 *
 * Returns an <i>x</i> value in the screen coordinate system when provided the
 * corresponding <b>east</b> value in the earth coordinate system.
 *
 *  \param U_col east
 *  \return double
 */

double D_u_to_d_col(double U_col)
{
    return D.west + (U_col - U.west) * U_to_D_conv.x;
}


/*!
 * \brief earth to array (north)
 *
 * Returns a <i>row</i> value in the array coordinate system when provided the
 * corresponding <b>north</b> value in the earth coordinate system.
 *
 *  \param U_row north
 *  \return double
 */

double D_u_to_a_row(double U_row)
{
    return A.north + (U_row - U.north) / A_to_U_conv.y;
}


/*!
 * \brief earth to array (east
 *
 * Returns a <i>column</i> value in the array coordinate system when provided the
 * corresponding <b>east</b> value in the earth coordinate system.
 *
 *  \param U_col east
 *  \return double
 */

double D_u_to_a_col(double U_col)
{
    return A.west + (U_col - U.west) / A_to_U_conv.x;
}
