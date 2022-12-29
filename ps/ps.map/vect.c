/* Functions: adjust_line 
 *
 * Author: Radim Blazek Feb 2000
 *
 */

#include <stdlib.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/gis.h>
#include "vector.h"

#define LENGTH(DX, DY)  (  sqrt( (DX*DX)+(DY*DY) )  )

/* nearest returns nearest longitude coordinate, copy from src/libes */
static double nearest_longitude(double e0, double e1)
{
    while (e0 - e1 > 180)
	e1 += 360.0;
    while (e1 - e0 > 180)
	e1 -= 360.0;
    return e1;
}

/* if projection is PROJECTION_LL adjust_line will change
 *   longitudes to nearest form previous point
 */
void adjust_line(struct line_pnts *Points)
{
    int i, np;

    if (G_projection() == PROJECTION_LL) {
	np = Points->n_points;
	for (i = 1; i < np; i++) {
	    Points->x[i] = nearest_longitude(Points->x[i - 1], Points->x[i]);
	}
    }
}
