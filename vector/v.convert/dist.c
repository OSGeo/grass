#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include "conv.h"
#include "local_proto.h"

double ldist(double x, double y, struct Line *p)
{
    int i;
    double dist, ndist;

    i = (p->n_points == 1) ? 0 : 1;
    dist = dig_distance2_point_to_line(x, y, 0, p->x[0], p->y[0], 0,
				       p->x[i], p->y[i], 0,
				       0, NULL, NULL, NULL, NULL, NULL);

    for (i = 1; i < p->n_points - 1; i++) {
	ndist = dig_distance2_point_to_line(x, y, 0, p->x[i], p->y[i], 0,
					    p->x[i + 1], p->y[i + 1], 0,
					    0, NULL, NULL, NULL, NULL, NULL);

	if (ndist < dist) {
	    dist = ndist;
	}
    }
    return (dist);
}
