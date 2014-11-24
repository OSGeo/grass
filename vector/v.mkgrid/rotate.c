#include <math.h>

static double sa = 0.0;
static double ca = 1.0;

void set_angle(double angle)
{
    if (angle != 0) {
	sa = sin(angle);
	ca = cos(angle);
    }
}

/* rotates x and y about the origin (xo,yo) by angle radians */
void rotate(double *x, double *y, double xo, double yo, double angle)
{
    double tmpx, tmpy;

    if (angle == 0)
	return;

    /* first translate */
    tmpx = *x - xo;
    tmpy = *y - yo;

    /* now rotate */
    *x = tmpx * ca - tmpy * sa;
    *y = tmpx * sa + tmpy * ca;

    /* now translate back */
    *x += xo;
    *y += yo;

    return;
}
