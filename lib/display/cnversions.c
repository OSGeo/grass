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

struct rectangle
{
    double west;
    double east;
    double south;
    double north;
};

struct vector
{
    double x, y;
};

/* Bounding rectangles */
static struct rectangle U;	/* UTM coordinates, meters, (0,0) towards SW */
static struct rectangle A;	/* Map array coordinates, integers, (0,0) towards NW */
static struct rectangle D;	/* Display coordinates, pixels, (0,0) towards NW */

/* Conversion factors */
static struct vector U_to_D_conv;	/* UTM to Display   */
static struct vector D_to_A_conv;	/* Display to Array */

/* others */
static int is_lat_lon;
static struct vector resolution;

/*!
 * \brief initialize conversions
 *
 * The relationship between the
 * earth <b>region</b> and the <b>top, bottom, left</b>, and <b>right</b>
 * screen coordinates is established, which then allows conversions between all
 * three coordinate systems to be performed.
 * Note this routine is called by <i>D_setup.</i>
 *
 *  \param region
 *  \param top
 *  \param bottom
 *  \param left
 *  \param right
 *  \return int
 */

int D_do_conversions(const struct Cell_head *window, int t, int b, int l,
		     int r)
{
    struct vector ARRAY_SIZE;
    struct rectangle WIND;
    struct vector D_size, U_size;

    WIND.north = (double)t;
    WIND.south = (double)b;
    WIND.west = (double)l;
    WIND.east = (double)r;

    is_lat_lon = (window->proj == PROJECTION_LL);

    resolution.y = window->ns_res;
    resolution.x = window->ew_res;

    /* Key all coordinate limits off UTM window limits  */
    U.west = window->west;
    U.east = window->east;
    U.south = window->south;
    U.north = window->north;

    U_size.y = U.north - U.south;
    U_size.x = U.east - U.west;

    D_size.x = WIND.east - WIND.west;
    D_size.y = WIND.south - WIND.north;

    U_to_D_conv.x = D_size.x / U_size.x;
    U_to_D_conv.y = D_size.y / U_size.y;

    if (U_to_D_conv.x > U_to_D_conv.y) {
	U_to_D_conv.x = U_to_D_conv.y;
	D.west =
	    (double)(int)((WIND.west + WIND.east -
			   U_size.x * U_to_D_conv.x) / 2);
	D.east =
	    (double)(int)((WIND.west + WIND.east +
			   U_size.x * U_to_D_conv.x) / 2);
	D.north = WIND.north;
	D.south = WIND.south;
    }
    else {
	U_to_D_conv.y = U_to_D_conv.x;
	D.west = WIND.west;
	D.east = WIND.east;
	D.north =
	    (double)(int)((WIND.north + WIND.south -
			   U_size.y * U_to_D_conv.y) / 2);
	D.south =
	    (double)(int)((WIND.north + WIND.south +
			   U_size.y * U_to_D_conv.y) / 2);
    }

    D_size.x = D.east - D.west;
    D_size.y = D.south - D.north;

    ARRAY_SIZE.x = window->cols;
    ARRAY_SIZE.y = window->rows;

    A.west = 0.0;
    A.north = 0.0;
    A.east = (double)ARRAY_SIZE.x;
    A.south = (double)ARRAY_SIZE.y;

    D_to_A_conv.x = (double)ARRAY_SIZE.x / D_size.x;
    D_to_A_conv.y = (double)ARRAY_SIZE.y / D_size.y;

#ifdef DEBUG
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
	    " ARRAY_ROWS %d  resolution_ns %10.2f\n", ARRAY_SIZE.y,
	    window->ns_res);
    fprintf(stderr, " ARRAY_COLS %d  resolution_ew %10.2f\n", ARRAY_SIZE.x,
	    window->ew_res);
    fprintf(stderr, " D_to_A_conv.x %10.1f D_to_A_conv.y %10.1f \n",
	    D_to_A_conv.x, D_to_A_conv.y);
    fprintf(stderr, " BOT %10.1f  TOP %10.1f  LFT %10.1f  RHT %10.1f\n",
	    WIND.south, WIND.north, WIND.west, WIND.east);
#endif /* DEBUG */

    return (0);
}

int D_is_lat_lon(void)
{
    return (is_lat_lon);
}

double D_get_ns_resolution(void)
{
    return (resolution.y);
}
double D_get_ew_resolution(void)
{
    return (resolution.x);
}

double D_get_u_to_d_xconv(void)
{
    return (U_to_D_conv.x);
}
double D_get_u_to_d_yconv(void)
{
    return (U_to_D_conv.y);
}

double D_get_u_west(void)
{
    return (U.west);
}
double D_get_u_east(void)
{
    return (U.east);
}
double D_get_u_north(void)
{
    return (U.north);
}
double D_get_u_south(void)
{
    return (U.south);
}

double D_get_a_west(void)
{
    return (A.west);
}
double D_get_a_east(void)
{
    return (A.east);
}
double D_get_a_north(void)
{
    return (A.north);
}
double D_get_a_south(void)
{
    return (A.south);
}

double D_get_d_west(void)
{
    return (D.west);
}
double D_get_d_east(void)
{
    return (D.east);
}
double D_get_d_north(void)
{
    return (D.north);
}
double D_get_d_south(void)
{
    return (D.south);
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

void D_get_d(int x[2][2])
{
    x[0][0] = (int)D.west;
    x[0][1] = (int)D.east;
    x[1][0] = (int)D.north;
    x[1][1] = (int)D.south;
}

/*!
 * \brief earth to array (north)
 *
 * Returns a <i>row</i> value in the array coordinate system when provided the
 * corresponding <b>north</b> value in the earth coordinate system.
 *
 *  \param north
 *  \return double
 */

double D_u_to_a_row(double U_row)
{
    return (U.north - U_row) / resolution.y;
}


/*!
 * \brief earth to array (east
 *
 * Returns a <i>column</i> value in the array coordinate system when provided the
 * corresponding <b>east</b> value in the earth coordinate system.
 *
 *  \param east
 *  \return double
 */

double D_u_to_a_col(double U_col)
{
    return (U_col - U.west) / resolution.x;
}


/*!
 * \brief array to screen (row)
 *
 * Returns a <i>y</i> value in the screen coordinate system when provided the
 * corresponding <b>row</b> value in the array coordinate system.
 *
 *  \param row
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
 *  \param column
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
 *  \param north
 *  \return double
 */

double D_u_to_d_row(double U_row)
{
    return D.north + (U.north - U_row) * U_to_D_conv.y;
}


/*!
 * \brief earth to screen (east)
 *
 * Returns an <i>x</i> value in the screen coordinate system when provided the
 * corresponding <b>east</b> value in the earth coordinate system.
 *
 *  \param east
 *  \return double
 */

double D_u_to_d_col(double U_col)
{
    return D.west + (U_col - U.west) * U_to_D_conv.x;
}


/*!
 * \brief screen to earth (y)
 *
 * Returns a <i>north</i> value in the earth coordinate system when provided the
 * corresponding <b>y</b> value in the screen coordinate system.
 *
 *  \param y
 *  \return double
 */

double D_d_to_u_row(double D_row)
{
    return U.north - (D_row - D.north) / U_to_D_conv.y;
}


/*!
 * \brief screen to earth (x)
 *
 * Returns an <i>east</i> value in the earth coordinate system when provided the
 * corresponding <b>x</b> value in the screen coordinate system.
 *
 *  \param x
 *  \return double
 */

double D_d_to_u_col(double D_col)
{
    return U.west + (D_col - D.west) / U_to_D_conv.x;
}


/*!
 * \brief screen to array (y)
 *
 * Returns a <i>row</i> value in the array coordinate system when provided the
 * corresponding <b>y</b> value in the screen coordinate system.
 *
 *  \param y
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
 *  \param x
 *  \return double
 */

double D_d_to_a_col(double D_col)
{
    return A.west + (D_col - D.west) * D_to_A_conv.x;
}
