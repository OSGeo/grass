#include <grass/gis.h>
/* compute square of distance of point (x,y) to line segment (x1,y1 - x2,y2) */

#define ZERO(x) x < tolerance && x > -tolerance
#define TOLERANCE 1.0e-10
static double tolerance = TOLERANCE;

void G_set_distance_to_line_tolerance(double t)
{
    if (t <= 0.0)
	t = TOLERANCE;
    tolerance = t;
}

double G_distance2_point_to_line(double x, double y,	/* point */
				 double x1, double y1, double x2, double y2)
{				/* line segment */
    double dx, dy, t;

    dx = x2 - x1;
    dy = y2 - y1;

    if (ZERO(dx) && ZERO(dy)) {	/* line is degenerate */
	dx = x1 - x;
	dy = y1 - y;
	return dx * dx + dy * dy;	/* compute distance x,y to x1,y1 */
    }

    t = (dx * (x - x1) + dy * (y - y1)) / (dx * dx + dy * dy);

    if (t < 0.0) {		/* go to x1,y1 */
	dx = x - x1;
	dy = y - y1;
    }
    else if (t > 1.0) {		/* go to x2,y2 */
	dx = x - x2;
	dy = y - y2;
    }
    else {			/* go t from x1,y1 towards x2,y2 */

	dx = x - (dx * t + x1);
	dy = y - (dy * t + y1);
    }
    return dx * dx + dy * dy;
}
