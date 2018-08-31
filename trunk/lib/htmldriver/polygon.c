#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include "driverlib.h"
#include "htmlmap.h"

#define RAD_DEG M_R2D

static void delete_point(int *x, int *y, int count)
{
    int i;

    for (i = 0; i < count; i++) {
	x[i] = x[i + 1];
	y[i] = y[i + 1];
    }


}

static double find_azimuth(double x1, double y1, double x2, double y2)
{
    double xx, yy;

    xx = x1 - x2;
    yy = y1 - y2;

    if (x1 == x2) {
	return (y2 > y1) ? 90.0 : 270.0;
    }
    else {
	if (y2 < y1) {
	    if (x2 > x1) {
		return 360.0 + (RAD_DEG * atan(yy / xx));
	    }
	    else {
		return 180.0 + (RAD_DEG * atan(yy / xx));
	    }
	}
	else {
	    if (x2 > x1) {
		return (RAD_DEG * atan(yy / xx));
	    }
	    else {
		return 180.0 + (RAD_DEG * atan(yy / xx));
	    }
	}
    }
}


void html_polygon(const struct path *p)
{
    int n = p->count;
    struct MapPoly *new;
    int i;
    int delta_x, delta_y;
    int min_x, max_x, min_y, max_y;

    double min_azimuth = 1.0;
    double azimuth1, azimuth2, diff1, diff2;
    int *x = G_malloc(n * sizeof(int));
    int *y = G_malloc(n * sizeof(int));

    for (i = 0; i < n; i++) {
	x[i] = (int) floor(p->vertices[i].x + 0.5);
	y[i] = (int) floor(p->vertices[i].y + 0.5);
    }

    /* 
     * remove points that have adjacent duplicates or have differences of
     * less the the minimum allowed.  remove end points that are same as
     * the begin point (ending point = starting point is added 
     * during Graph_Clse)
     */

    i = 0;
    while (i < (n - 1)) {
	delta_x = x[i] - x[i + 1];
	if (delta_x < 0)
	    delta_x = -delta_x;
	delta_y = y[i] - y[i + 1];
	if (delta_y < 0)
	    delta_y = -delta_y;

	if ((x[i] == x[i + 1] && y[i] == y[i + 1]) ||
	    (delta_x <= html.MINIMUM_DIST && delta_y <= html.MINIMUM_DIST)) {
	    delete_point(&x[i + 1], &y[i + 1], n - i - 1);
	    --n;
	}
	else {
	    ++i;
	}
    }

    /* perform same checks for last point & first point */
    while (1) {
	delta_x = x[0] - x[n - 1];
	if (delta_x < 0)
	    delta_x = -delta_x;
	delta_y = y[0] - y[n - 1];
	if (delta_y < 0)
	    delta_y = -delta_y;

	if ((x[0] == x[n - 1] && y[0] == y[n - 1]) ||
	    (delta_x <= html.MINIMUM_DIST && delta_y <= html.MINIMUM_DIST)) {
	    --n;
	}
	else {
	    break;
	}
    }



    /* 
     * if a polygon has either x or y extents less than the bounding box 
     * minimum, ignore it
     *
     */

    min_x = max_x = x[0];
    min_y = max_y = y[0];
    for (i = 0; i < n; i++) {
	if (x[i] < min_x)
	    min_x = x[i];
	if (x[i] > max_x)
	    max_x = x[i];
	if (y[i] < min_y)
	    min_y = y[i];
	if (y[i] > max_y)
	    max_y = y[i];
    }
    delta_x = max_x - min_x;
    delta_y = max_y - min_y;
    if (delta_x < html.BBOX_MINIMUM || delta_y < html.BBOX_MINIMUM) {
	n = 0;
    }


    /* 
     * remove points in excess of MAX_POINTS vertices
     */

    while (n > html.MAX_POINTS) {

	for (i = 0; i < (n - 2); i++) {

	    /* 
	     * see if middle point can be removed, by checking if the
	     * relative bearing to the middle is less than our current tolerance
	     */

	    azimuth1 = find_azimuth((double)x[i], (double)y[i],
				    (double)x[i + 1], (double)y[i + 1]);
	    azimuth2 = find_azimuth((double)x[i], (double)y[i],
				    (double)x[i + 2], (double)y[i + 2]);

	    diff1 = fmod(fabs((azimuth2 + 360.0) - azimuth1), 360.0);
	    diff2 = fmod(fabs((azimuth1 + 360.0) - azimuth2), 360.0);

	    if (diff1 <= min_azimuth || diff2 <= min_azimuth) {

		delete_point(&x[i + 1], &y[i + 1], n - i - 1);
		--n;
		++i;
		/* either stop deleting points because we're less than 100,
		   or keep deleting points with the same difference as this 
		   one (which might make a smaller polygon yet).  
		   if (n <= 100) {
		   break;
		   }
		 */
	    }

	}

	/* increase minimum azimuth difference for next round */
	min_azimuth += 1.0;
    }

    /*
     * copy remaining points into a new MapPoly
     */

    if (n >= 3) {

	new = (struct MapPoly *)G_malloc(sizeof(struct MapPoly));

	/* grab the last text string written as url */
	new->url = G_store(html.last_text);

	/* hook up new MapPoly into list */
	new->next_poly = NULL;
	*html.tail = new;
	html.tail = &(new->next_poly);

	new->num_pts = n;
	new->x_pts = x;
	new->y_pts = y;
    }
    else {
	G_free(x);
	G_free(y);
    }
}
