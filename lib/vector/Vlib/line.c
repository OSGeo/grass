/*!
   \file lib/vector/Vlib/line.c

   \brief Vector library - vector feature geometry 

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Some updates for GRASS 7 by Martin Landa <landa.martin gmail.com>
*/

#include <stdlib.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
  \brief Creates and initializes a struct line_pnts (internal use only)

  Use Vect_new_line_struct() instead.

  \return pointer to allocated line_pnts structure
  \return NULL on error
 */
struct line_pnts *Vect__new_line_struct(void);

/*!
  \brief Creates and initializes a line_pnts structure

  This structure is used for reading and writing vector lines and
  polygons.  The library routines handle all memory allocation.  If 3
  lines in memory are needed at the same time, then simply 3 line_pnts
  structures have to be used.
  
  To free allocated memory call Vect_destroy_line_struct().

  Calls G_fatal_error() on error.

  \return pointer to line_pnts
 */
struct line_pnts *Vect_new_line_struct()
{
    struct line_pnts *p;

    if (NULL == (p = Vect__new_line_struct()))
	G_fatal_error("Vect_new_line_struct(): %s", _("Out of memory"));

    return p;
}

struct line_pnts *Vect__new_line_struct()
{
    struct line_pnts *p;

    p = (struct line_pnts *)malloc(sizeof(struct line_pnts));

    /* alloc_points MUST be initialized to zero */
    if (p)
	p->alloc_points = p->n_points = 0;

    if (p)
	p->x = p->y = p->z = NULL;

    return p;
}

/*!
   \brief Frees all memory associated with a line_pnts structure,
   including the structure itself

   \param p pointer to line_pnts structure
 */
void Vect_destroy_line_struct(struct line_pnts *p)
{
    if (p) {			/* probably a moot test */
	if (p->alloc_points) {
	    G_free((char *)p->x);
	    G_free((char *)p->y);
	    G_free((char *)p->z);
	}
	G_free((char *)p);
    }
}

/*!
   \brief Copy points from array to line_pnts structure

   \param Points pointer to line_ptns structure
   \param x,y,z  array of coordinates
   \param n number of points to be copied

   \return 0 on success
   \return -1 on out of memory
 */
int Vect_copy_xyz_to_pnts(struct line_pnts *Points, const double *x, const double *y,
			  const double *z, int n)
{
    int i;

    if (0 > dig_alloc_points(Points, n))
	return -1;

    for (i = 0; i < n; i++) {
	Points->x[i] = x[i];
	Points->y[i] = y[i];
	if (z != NULL)
	    Points->z[i] = z[i];
	else
	    Points->z[i] = 0;
	Points->n_points = n;
    }
    
    return 0;
}


/*!
  \brief Reset line

  Make sure line structure is clean to be re-used, i.e. it has no
  points associated with it Points must have previously been created
  with Vect_new_line_struct().

  \param Points pointer to line_pnts structure to be reset
 */
void Vect_reset_line(struct line_pnts *Points)
{
    Points->n_points = 0;
}

/*!
  \brief Appends one point to the end of a line.

  If you are re-using a line struct, be sure to clear out old data
  first by calling Vect_reset_line().
  
  Calls G_fatal_error() when out of memory.
  
  \param Points pointer to line_pnts structure
  \param x,y,z point coordinates to be added

  \return number of points
  \return -1 on error (out of memory)
 */
int Vect_append_point(struct line_pnts *Points, double x, double y, double z)
{
    register int n;

    if (0 > dig_alloc_points(Points, Points->n_points + 1)) {
	G_fatal_error(_("Out of memory"));
	return -1;
    }
    
    n = Points->n_points;
    Points->x[n] = x;
    Points->y[n] = y;
    Points->z[n] = z;

    return ++(Points->n_points);
}

/*!
  \brief Insert new point at index position and move all old points
  at that position and above up
   
  \param Points pointer to line_pnts structure 
  \param index (from 0 to Points->n_points-1)
  \param x,y,z point coordinates

  \return number of points
  \return -1 on error (alocation)
 */
int Vect_line_insert_point(struct line_pnts *Points, int index, double x,
			   double y, double z)
{
    int n;

    if (index < 0 || index > Points->n_points - 1)
	G_fatal_error("Vect_line_insert_point(): %s",
		      _("Index out of range in"));

    if (0 > dig_alloc_points(Points, Points->n_points + 1))
	return -1;

    /* move up */
    for (n = Points->n_points; n > index; n--) {
	Points->x[n] = Points->x[n - 1];
	Points->y[n] = Points->y[n - 1];
	Points->z[n] = Points->z[n - 1];
    }

    Points->x[index] = x;
    Points->y[index] = y;
    Points->z[index] = z;
    
    return ++(Points->n_points);
}

/*!
  \brief Delete point at given index and move all points above down

  \param Points pointer to line_pnts structure
  \param index point (from 0 to Points->n_points-1)

  \return number of points
 */
int Vect_line_delete_point(struct line_pnts *Points, int index)
{
    int n;
    
    if (index < 0 || index > Points->n_points - 1)
	G_fatal_error("Vect_line_insert_point(): %s",
		      _("Index out of range in"));
    
    if (Points->n_points == 0)
	return 0;

    /* move down */
    for (n = index; n < Points->n_points - 1; n++) {
	Points->x[n] = Points->x[n + 1];
	Points->y[n] = Points->y[n + 1];
	Points->z[n] = Points->z[n + 1];
    }
    
    return --(Points->n_points);
}

/*!
  \brief Get line point of given index

  Calls G_fatal_error() when index is not range in.

  \param Points pointer to line_pnts structure
  \param index point index (from 0 to Points->n_points-1)
  \param x pointer to x coordinate or NULL
  \param y pointer to y coordinate or NULL
  \param z pointer to z coordinate or NULL

  \return number of points
*/
int Vect_line_get_point(const struct line_pnts *Points, int index,
			double *x, double *y, double *z)
{
    if (index < 0 || index > Points->n_points - 1)
	G_fatal_error("Vect_line_get_point(): %s",
		      _("Index out of range in"));
    
    if (x)
	*x = Points->x[index];
    if (y)
	*y = Points->y[index];
    if (z)
	*z = Points->z[index];
    
    return Points->n_points;
}

/*!
  \brief Get number of line points

  \param Points pointer to line_pnts structure
  
  \return number of points
*/
int Vect_get_num_line_points(const struct line_pnts *Points)
{
    return Points->n_points;
}

/*!
   \brief Remove duplicate points, i.e. zero length segments

   \param Points pointer to line_pnts structure

   \return number of points
 */
int Vect_line_prune(struct line_pnts *Points)
{
    int i, j;

    if (Points->n_points > 0) {
	j = 1;
	for (i = 1; i < Points->n_points; i++) {
	    if (Points->x[i] != Points->x[j - 1] ||
		Points->y[i] != Points->y[j - 1]
		|| Points->z[i] != Points->z[j - 1]) {
		Points->x[j] = Points->x[i];
		Points->y[j] = Points->y[i];
		Points->z[j] = Points->z[i];
		j++;
	    }
	}
	Points->n_points = j;
    }

    return (Points->n_points);
}

/*!
   \brief Remove points in threshold

   \param Points pointer to line_pnts structure
   \param threshold threshold value

   \return number of points in result
 */
int Vect_line_prune_thresh(struct line_pnts *Points, double threshold)
{
    int ret;

    ret = dig_prune(Points, threshold);

    if (ret < Points->n_points)
	Points->n_points = ret;

    return (Points->n_points);
}

/*!
  \brief Appends points to the end of a line.
  
  Note, this will append to whatever is in line_pnts structure. If you
  are re-using a line struct, be sure to clear out old data first by
  calling Vect_reset_line().

  \param Points pointer to line_pnts structure
  \param APoints points to be included
  \param direction direction (GV_FORWARD, GV_BACKWARD)
  
  \return new number of points
  \return -1 on out of memory
*/
int Vect_append_points(struct line_pnts *Points, const struct line_pnts *APoints,
		       int direction)
{
    int i, n, on, an;

    on = Points->n_points;
    an = APoints->n_points;
    n = on + an;

    /* Should be OK, dig_alloc_points calls realloc */
    if (0 > dig_alloc_points(Points, n))
	return (-1);

    if (direction == GV_FORWARD) {
	for (i = 0; i < an; i++) {
	    Points->x[on + i] = APoints->x[i];
	    Points->y[on + i] = APoints->y[i];
	    Points->z[on + i] = APoints->z[i];
	}
    }
    else {
	for (i = 0; i < an; i++) {
	    Points->x[on + i] = APoints->x[an - i - 1];
	    Points->y[on + i] = APoints->y[an - i - 1];
	    Points->z[on + i] = APoints->z[an - i - 1];
	}
    }

    Points->n_points = n;
    return n;
}


/*!
  \brief Copy points from line structure to array

  x/y/z arrays MUST be at least as large as Points->n_points

  Also note that  n  is a pointer to int.

  \param Points pointer to line_pnts structure
  \param x,y,z coordinates arrays
  \param n number of points
  
  \return number of points copied
*/
int Vect_copy_pnts_to_xyz(const struct line_pnts *Points, double *x, double *y,
			  double *z, int *n)
{
    int i;

    for (i = 0; i < *n; i++) {
	x[i] = Points->x[i];
	y[i] = Points->y[i];
	if (z != NULL)
	    z[i] = Points->z[i];
	*n = Points->n_points;
    }

    return (Points->n_points);
}

/*!
  \brief  Find point on line in the specified distance.

  From the beginning, measured along line.

  If the distance is greater than line length or negative, error is returned.

  \param Points pointer to line_pnts structure
  \param distance distance value
  \param x,y,z pointers to point coordinates or NULL
  \param angle pointer to angle of line in that point (radians, counter clockwise from x axis) or NULL
  \param slope pointer to slope angle in radians (positive up)
  
  \return number of segment the point is on (first is 1),
  \return 0 error when point is outside the line 
 */
int Vect_point_on_line(const struct line_pnts *Points, double distance,
		       double *x, double *y, double *z, double *angle,
		       double *slope)
{
    int j, np, seg = 0;
    double dist = 0, length;
    double xp = 0, yp = 0, zp = 0, dx = 0, dy = 0, dz = 0, dxy =
	0, dxyz, k, rest;

    G_debug(3, "Vect_point_on_line(): distance = %f", distance);
    if ((distance < 0) || (Points->n_points < 2))
	return 0;

    /* Check if first or last */
    length = Vect_line_length(Points);
    G_debug(3, "  length = %f", length);
    if (distance < 0 || distance > length) {
	G_debug(3, "  -> outside line");
	return 0;
    }

    np = Points->n_points;
    if (distance == 0) {
	G_debug(3, "  -> first point");
	xp = Points->x[0];
	yp = Points->y[0];
	zp = Points->z[0];
	dx = Points->x[1] - Points->x[0];
	dy = Points->y[1] - Points->y[0];
	dz = Points->z[1] - Points->z[0];
	dxy = hypot(dx, dy);
	seg = 1;
    }
    else if (distance == length) {
	G_debug(3, "  -> last point");
	xp = Points->x[np - 1];
	yp = Points->y[np - 1];
	zp = Points->z[np - 1];
	dx = Points->x[np - 1] - Points->x[np - 2];
	dy = Points->y[np - 1] - Points->y[np - 2];
	dz = Points->z[np - 1] - Points->z[np - 2];
	dxy = hypot(dx, dy);
	seg = np - 1;
    }
    else {
	for (j = 0; j < Points->n_points - 1; j++) {
	    /* dxyz = G_distance(Points->x[j], Points->y[j], 
	       Points->x[j+1], Points->y[j+1]); */
	    dx = Points->x[j + 1] - Points->x[j];
	    dy = Points->y[j + 1] - Points->y[j];
	    dz = Points->z[j + 1] - Points->z[j];
	    dxy = hypot(dx, dy);
	    dxyz = hypot(dxy, dz);

	    dist += dxyz;
	    if (dist >= distance) {	/* point is on the current line part */
		rest = distance - dist + dxyz;	/* from first point of segment to point */
		k = rest / dxyz;

		xp = Points->x[j] + k * dx;
		yp = Points->y[j] + k * dy;
		zp = Points->z[j] + k * dz;
		seg = j + 1;
		break;
	    }
	}
    }

    if (x != NULL)
	*x = xp;
    if (y != NULL)
	*y = yp;
    if (z != NULL)
	*z = zp;

    /* calculate angle */
    if (angle != NULL)
	*angle = atan2(dy, dx);

    /* calculate slope */
    if (slope != NULL)
	*slope = atan2(dz, dxy);

    return seg;
}

/*!
  \brief Create line segment.
  
  Creates segment of InPoints from start to end measured along the
  line and write it to OutPoints.
  
  If the distance is greater than line length or negative, error is
  returned.
  
  \param InPoints input line
  \param start segment number
  \param end segment number
  \param OutPoints output line
  
  \return 1 success
  \return 0 error when start > length or end < 0 or start < 0 or end > length
*/
int Vect_line_segment(const struct line_pnts *InPoints, double start, double end,
		      struct line_pnts *OutPoints)
{
    int i, seg1, seg2;
    double length, tmp;
    double x1, y1, z1, x2, y2, z2;

    G_debug(3, "Vect_line_segment(): start = %f, end = %f, n_points = %d",
	    start, end, InPoints->n_points);

    Vect_reset_line(OutPoints);

    if (start > end) {
	tmp = start;
	start = end;
	end = tmp;
    }

    /* Check start/end */
    if (end < 0)
	return 0;
    length = Vect_line_length(InPoints);
    if (start > length)
	return 0;

    /* Find coordinates and segments of start/end */
    seg1 = Vect_point_on_line(InPoints, start, &x1, &y1, &z1, NULL, NULL);
    seg2 = Vect_point_on_line(InPoints, end, &x2, &y2, &z2, NULL, NULL);

    G_debug(3, "  -> seg1 = %d seg2 = %d", seg1, seg2);

    if (seg1 == 0 || seg2 == 0) {
	G_warning(_("Segment outside line, no segment created"));
	return 0;
    }

    Vect_append_point(OutPoints, x1, y1, z1);

    for (i = seg1; i < seg2; i++) {
	Vect_append_point(OutPoints, InPoints->x[i], InPoints->y[i],
			  InPoints->z[i]);
    };

    Vect_append_point(OutPoints, x2, y2, z2);
    Vect_line_prune(OutPoints);

    return 1;
}

/*!
  \brief Calculate line length, 3D-length in case of 3D vector line

  For Lat-Long use Vect_line_geodesic_length() instead.
  
  \param Points pointer to line_pnts structure geometry
  
  \return line length
 */
double Vect_line_length(const struct line_pnts *Points)
{
    int j;
    double dx, dy, dz, len = 0;

    if (Points->n_points < 2)
	return 0;

    for (j = 0; j < Points->n_points - 1; j++) {
	dx = Points->x[j + 1] - Points->x[j];
	dy = Points->y[j + 1] - Points->y[j];
	dz = Points->z[j + 1] - Points->z[j];
	len += hypot(hypot(dx, dy), dz);
    }

    return len;
}


/*!
  \brief Calculate line length.
  
  If projection is LL, the length is measured along the geodesic.
  
  \param Points pointer to line_pnts structure geometry
  
  \return line length
*/
double Vect_line_geodesic_length(const struct line_pnts *Points)
{
    int j, dc;
    double dx, dy, dz, dxy, len = 0;

    dc = G_begin_distance_calculations();

    if (Points->n_points < 2)
	return 0;

    for (j = 0; j < Points->n_points - 1; j++) {
	if (dc == 2)
	    dxy =
		G_geodesic_distance(Points->x[j], Points->y[j],
				    Points->x[j + 1], Points->y[j + 1]);
	else {
	    dx = Points->x[j + 1] - Points->x[j];
	    dy = Points->y[j + 1] - Points->y[j];
	    dxy = hypot(dx, dy);
	}

	dz = Points->z[j + 1] - Points->z[j];
	len += hypot(dxy, dz);
    }

    return len;
}

/*!
  \brief Calculate distance of point to line.
  
  Sets (if not null):
   - px, py - point on line,
   - dist   - distance to line,
   - spdist - distance to point on line from segment beginning,
   - lpdist - distance to point on line from line beginning along line

  \param points pointer to line_pnts structure
  \param ux,uy,uz point coordinates
  \param with_z flag if to use z coordinate (3D calculation)
  \param[out] px,py,pz point on line
  \param[out] dist distance to line
  \param[out] spdist distance to point on line from segment beginning
  \param[out] lpdist distance to point on line from line beginning along line

  \return nearest segment (first is 1)
 */
int Vect_line_distance(const struct line_pnts *points,
		       double ux, double uy, double uz,
		       int with_z,
		       double *px, double *py, double *pz,
		       double *dist, double *spdist, double *lpdist)
{
    int i;
    double distance;
    double new_dist;
    double tpx, tpy, tpz, tdist, tspdist, tlpdist = 0;
    double dx, dy, dz;
    int n_points;
    int segment;

    n_points = points->n_points;

    if (n_points == 1) {
	distance =
	    dig_distance2_point_to_line(ux, uy, uz, points->x[0],
					points->y[0], points->z[0],
					points->x[0], points->y[0],
					points->z[0], with_z, NULL, NULL,
					NULL, NULL, NULL);
	tpx = points->x[0];
	tpy = points->y[0];
	tpz = points->z[0];
	tdist = sqrt(distance);
	tspdist = 0;
	tlpdist = 0;
	segment = 0;

    }
    else {

	distance =
	    dig_distance2_point_to_line(ux, uy, uz, points->x[0],
					points->y[0], points->z[0],
					points->x[1], points->y[1],
					points->z[1], with_z, NULL, NULL,
					NULL, NULL, NULL);
	segment = 1;

	for (i = 1; i < n_points - 1; i++) {
	    new_dist = dig_distance2_point_to_line(ux, uy, uz,
						   points->x[i], points->y[i],
						   points->z[i],
						   points->x[i + 1],
						   points->y[i + 1],
						   points->z[i + 1], with_z,
						   NULL, NULL, NULL, NULL,
						   NULL);
	    if (new_dist < distance) {
		distance = new_dist;
		segment = i + 1;
	    }
	}

	/* we have segment and now we can recalculate other values (speed) */
	new_dist = dig_distance2_point_to_line(ux, uy, uz,
					       points->x[segment - 1],
					       points->y[segment - 1],
					       points->z[segment - 1],
					       points->x[segment],
					       points->y[segment],
					       points->z[segment], with_z,
					       &tpx, &tpy, &tpz, &tspdist,
					       NULL);

	/* calculate distance from beginning of line */
	if (lpdist) {
	    tlpdist = 0;
	    for (i = 0; i < segment - 1; i++) {
		dx = points->x[i + 1] - points->x[i];
		dy = points->y[i + 1] - points->y[i];
		if (with_z)
		    dz = points->z[i + 1] - points->z[i];
		else
		    dz = 0;

		tlpdist += hypot(hypot(dx, dy), dz);
	    }
	    tlpdist += tspdist;
	}
	tdist = sqrt(distance);
    }

    if (px)
	*px = tpx;
    if (py)
	*py = tpy;
    if (pz && with_z)
	*pz = tpz;
    if (dist)
	*dist = tdist;
    if (spdist)
	*spdist = tspdist;
    if (lpdist)
	*lpdist = tlpdist;

    return (segment);
}

/*!
  \brief Calculate geodesic distance of point to line in meters.
  
  Sets (if not null):
   - px, py - point on line,
   - dist   - distance to line,
   - spdist - distance to point on line from segment beginning,
   - lpdist - distance to point on line from line beginning along line

  \param points pointer to line_pnts structure
  \param ux,uy,uz point coordinates
  \param with_z flag if to use z coordinate (3D calculation)
  \param[out] px,py,pz point on line
  \param[out] dist distance to line
  \param[out] spdist distance to point on line from segment beginning
  \param[out] lpdist distance to point on line from line beginning along line

  \return nearest segment (first is 1)
 */
int Vect_line_geodesic_distance(const struct line_pnts *points,
		                double ux, double uy, double uz,
		                int with_z,
		                double *px, double *py, double *pz,
		                double *dist, double *spdist,
				double *lpdist)
{
    int i;
    double distance;
    double new_dist;
    double tpx, tpy, tpz, ttpx, ttpy, ttpz; 
    double tdist, tspdist, tlpdist = 0, tlpdistseg;
    double dz;
    int n_points;
    int segment;

    G_begin_distance_calculations();

    n_points = points->n_points;

    if (n_points == 1) {
	distance = G_distance(ux, uy, points->x[0], points->y[0]);
	if (with_z)
	    distance = hypot(distance, uz - points->z[0]);

	tpx = points->x[0];
	tpy = points->y[0];
	tpz = points->z[0];
	tdist = distance;
	tspdist = 0;
	tlpdist = 0;
	segment = 0;
    }
    else {
	distance =
	    dig_distance2_point_to_line(ux, uy, uz, points->x[0],
					points->y[0], points->z[0],
					points->x[1], points->y[1],
					points->z[1], with_z, 
					&tpx, &tpy, &tpz,
					NULL, NULL);

	distance = G_distance(ux, uy, tpx, tpy);
	if (with_z)
	    distance = hypot(distance, uz - tpz);

	segment = 1;

	for (i = 1; i < n_points - 1; i++) {
	    new_dist = dig_distance2_point_to_line(ux, uy, uz,
						   points->x[i], points->y[i],
						   points->z[i],
						   points->x[i + 1],
						   points->y[i + 1],
						   points->z[i + 1], with_z,
						   &ttpx, &ttpy, &ttpz,
						   NULL, NULL);

	    new_dist = G_distance(ux, uy, ttpx, ttpy);
	    if (with_z)
		new_dist = hypot(new_dist, uz - ttpz);

	    if (new_dist < distance) {
		distance = new_dist;
		segment = i + 1;
		tpx = ttpx;
		tpy = ttpy;
		tpz = ttpz;
	    }
	}

	/* calculate distance from beginning of segment */
	tspdist = G_distance(points->x[segment - 1], points->y[segment - 1],
			     tpx, tpy);
	if (with_z) {
	    dz = points->z[segment - 1] - tpz;
	    tspdist += hypot(tspdist, dz);
	}

	/* calculate distance from beginning of line */
	if (lpdist) {
	    tlpdist = 0;
	    for (i = 0; i < segment - 1; i++) {
		tlpdistseg = G_distance(points->x[i], points->y[i],
		                        points->x[i + 1], points->y[i + 1]);

		if (with_z) {
		    dz = points->z[i + 1] - points->z[i];
		    tlpdistseg += hypot(tlpdistseg, dz);
		}

		tlpdist += tlpdistseg;
	    }
	    tlpdist += tspdist;
	}
	tdist = distance;
    }

    if (px)
	*px = tpx;
    if (py)
	*py = tpy;
    if (pz && with_z)
	*pz = tpz;
    if (dist)
	*dist = tdist;
    if (spdist)
	*spdist = tspdist;
    if (lpdist)
	*lpdist = tlpdist;

    return (segment);
}


/*!
  \brief Calculate distance of 2 points
  
  Simply uses Pythagoras.
  
  \param x1,y1,z1 first point
  \param x2,y2,z2 second point
  \param with_z use z coordinate
  
  \return distance
*/
double Vect_points_distance(double x1, double y1, double z1,	/* point 1 */
			    double x2, double y2, double z2,	/* point 2 */
			    int with_z)
{
    double dx, dy, dz;


    dx = x2 - x1;
    dy = y2 - y1;
    dz = z2 - z1;

    if (with_z)
	return hypot(hypot(dx, dy), dz);
    else
	return hypot(dx, dy);

}

/*!
  \brief Get bounding box of line
  
  \param Points pointer to line_pnts structure
  \param[out] Box bounding box
*/
void Vect_line_box(const struct line_pnts *Points, struct bound_box *Box)
{
    dig_line_box(Points, Box);
}

/*!
  \brief Reverse the order of vertices
  
  \param Points pointer to line_pnts structure to be changed
*/
void Vect_line_reverse(struct line_pnts *Points)
{
    int i, j, np;
    double x, y, z;

    np = (int)Points->n_points / 2;

    for (i = 0; i < np; i++) {
	j = Points->n_points - i - 1;
	x = Points->x[i];
	y = Points->y[i];
	z = Points->z[i];
	Points->x[i] = Points->x[j];
	Points->y[i] = Points->y[j];
	Points->z[i] = Points->z[j];
	Points->x[j] = x;
	Points->y[j] = y;
	Points->z[j] = z;
    }
}

/*!
  \brief Fetches FIRST category number for given vector line and field
  
  \param Map pointer to Map_info structure
  \param line line id
  \param field layer number

  \return -1 no category
  \return category number (>=0)
 */
int Vect_get_line_cat(const struct Map_info *Map, int line, int field)
{

    static struct line_cats *cats = NULL;
    int cat, ltype;

    if (cats == NULL)
	cats = Vect_new_cats_struct();

    ltype = Vect_read_line(Map, NULL, cats, line);
    Vect_cat_get(cats, field, &cat);
    G_debug(3, "Vect_get_line_cat: display line %d, ltype %d, cat %d", line,
	    ltype, cat);

    return cat;
}
