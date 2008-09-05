#include <math.h>

#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

static int dont_draw;
static double t, b, l, r;
static double basex, basey;
static double curx, cury;

static void remember(double x, double y)
{
    if (x > r)
	r = x;
    if (x < l)
	l = x;
    if (y > b)
	b = y;
    if (y < t)
	t = y;

    curx = x;
    cury = y;
}

static void text_draw(double x, double y)
{
    COM_Line_abs(curx, cury, x, y);

    curx = x;
    cury = y;
}

static void text_move(double x, double y)
{
    curx = x;
    cury = y;
}

void drawchar(double text_size_x, double text_size_y,
	      double sinrot, double cosrot, unsigned char character)
{
    unsigned char *X;
    unsigned char *Y;
    int n_vects;
    int i;
    double ax, ay;
    double x, y;
    void (*Do) (double, double);
    int ix, iy;

    x = basex;
    y = basey;

    get_char_vects(character, &n_vects, &X, &Y);

    Do = text_move;

    for (i = 1; i < n_vects; i++) {
	if (X[i] == ' ') {
	    Do = text_move;
	    continue;
	}

	ix = 10 + X[i] - 'R';
	iy = 10 - Y[i] + 'R';
	ax = text_size_x * ix;
	ay = text_size_y * iy;

	if (dont_draw) {
	    remember(x + (ax * cosrot - ay * sinrot),
		     y - (ax * sinrot + ay * cosrot));
	}
	else {
	    (*Do) (x + (ax * cosrot - ay * sinrot),
		   y - (ax * sinrot + ay * cosrot));
	    Do = text_draw;
	}
    }
    /*  This seems to do variable spacing
       ix = 10 + X[i] - 'R';
     */
    ix = 20;
    iy = 0;
    ax = text_size_x * ix;
    ay = text_size_y * iy;

    if (!dont_draw)
	text_move(basex + (ax * cosrot - ay * sinrot),
		  basey - (ax * sinrot + ay * cosrot));
    else
	remember(basex + (ax * cosrot - ay * sinrot),
		 basey - (ax * sinrot + ay * cosrot));
}

void soft_text_ext(double x, double y,
		   double text_size_x, double text_size_y,
		   double text_rotation, const char *string)
{
    t = 999999;
    b = 0;
    l = 999999;
    r = 0;
    dont_draw = 1;
    soft_text(x, y, text_size_x, text_size_y, text_rotation, string);
    dont_draw = 0;
}

void get_text_ext(double *top, double *bot, double *left, double *rite)
{
    *top = t;
    *bot = b;
    *left = l;
    *rite = r;
}

# define RpD ((2 * M_PI) / 360.)	/* radians/degree */
# define D2R(d) (double)(d * RpD)	/* degrees->radians */

void soft_text(double x, double y,
	       double text_size_x, double text_size_y, double text_rotation,
	       const char *string)
{
    double sinrot = sin(D2R(text_rotation));
    double cosrot = cos(D2R(text_rotation));

    curx = basex = x;
    cury = basey = y;
    while (*string) {
	drawchar(text_size_x, text_size_y, sinrot, cosrot, *string++);
	basex = curx;
	basey = cury;
    }
}

void onechar(double x, double y,
	     double text_size_x, double text_size_y, double text_rotation,
	     unsigned char achar)
{
    double sinrot = sin(D2R(text_rotation));
    double cosrot = cos(D2R(text_rotation));

    curx = basex = x;
    cury = basey = y;
    drawchar(text_size_x, text_size_y, sinrot, cosrot, achar);
}
