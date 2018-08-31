/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
/*  @(#)prune.c 3.0  2/19/98  */
/* by Michel Wurtz for GRASS 4.2.1 - <mw@engees.u-strasbg.fr>
 * This is a complete rewriting of the previous dig_prune subroutine.
 * The goal remains : it resamples a dense string of x,y coordinates to
 * produce a set of coordinates that approaches hand digitizing.
 * That is, the density of points is very low on straight lines, and
 * highest on tight curves.
 *
 * The algorithm used is very different, and based on the suppression
 * of intermediate points, when they are closer than thresh from a
 * moving straight line.  
 *
 * The distance between a point M                ->   ->
 * and a AD segment is given                  || AM ^ AD ||
 * by the following formula :            d = ---------------
 *                                                  ->
 *                                               || AD ||
 *
 *                     ->   ->                             ->
 * When comparing   || AM ^ AD ||   and    t = thresh * || AD ||
 *
 *                     ->   ->       ->   ->
 * we call  sqdist = | AM ^ AD | = | OA ^ OM + beta | 
 *
 *                  ->   ->
 *  with     beta = OA ^ OD 
 *
 * The implementation is based on an old integer routine (optimised
 * for machine without math coprocessor), itself inspired by a PL/1
 * routine written after a fortran program on some prehistoric
 * hardware (IBM 360 probably). Yeah, I'm older than before :-)
 *
 * The algorithm used doesn't eliminate "duplicate" points (following
 * points with same coordinates).  So we should clean the set of points
 * before.  As a side effect, dig_prune can be called with a null thresh
 * value.  In this case only cleaning is made. The command v.prune is to
 * be modified accordingly.
 *
 * Another important note : Don't try too big threshold, this subroutine
 * may produce strange things with some pattern (mainly loops, or crossing
 * of level curves): Try the set { 0,0 -5,0 -4,10 -6,20 -5,30 -5,20 10,10}
 * with a thershold of 5. This isn't a programmation, but a conceptal bug ;-)
 *
 * Input parameters :
 * points->x, ->y   - double precision sets of coordinates.
 * points->n_points - the total number of points in the set.
 * thresh           - the distance that a string must wander from a straight
 *                    line before another point is selected.
 *
 * Value returned : - the final number of points in the pruned set.
 */

#include <stdio.h>
#include <grass/vector.h>
#include <math.h>

int dig_prune(struct line_pnts *points, double thresh)
{
    double *ox, *oy, *nx, *ny;
    double cur_x, cur_y;
    int o_num;
    int n_num;			/* points left */
    int at_num;
    int ij = 0,			/* position of farest point */
	ja, jd, i, j, k, n, inu, it;	/* indicateur de parcours du segment */

    double sqdist;		/* square of distance */
    double fpdist;		/* square of distance from chord to farest point */
    double t, beta;		/* as explained in commented algorithm  */

    double dx, dy;		/* temporary variables */

    double sx[18], sy[18];	/* temporary table for processing points */
    int nt[17], nu[17];

    /* nothing to do if less than 3 points ! */
    if (points->n_points <= 2)
	return (points->n_points);

    ox = points->x;
    oy = points->y;
    nx = points->x;
    ny = points->y;

    o_num = points->n_points;
    n_num = 0;

    /* Eliminate duplicate points */

    at_num = 0;
    while (at_num < o_num) {
	if (nx != ox) {
	    *nx = *ox++;
	    *ny = *oy++;
	}
	else {
	    ox++;
	    oy++;
	}
	cur_x = *nx++;
	cur_y = *ny++;
	n_num++;
	at_num++;

	while (*ox == cur_x && *oy == cur_y) {
	    if (at_num == o_num)
		break;
	    at_num++;
	    ox++;
	    oy++;
	}
    }

    /*  Return if less than 3 points left.  When all points are identical,
     *  output only one point (is this valid for calling function ?) */

    if (n_num <= 2)
	return n_num;

    if (thresh == 0.0)		/* Thresh is null, nothing more to do */
	return n_num;

    /* some (re)initialisations */

    o_num = n_num;
    ox = points->x;
    oy = points->y;

    sx[0] = ox[0];
    sy[0] = oy[0];
    n_num = 1;
    at_num = 2;
    k = 1;
    sx[1] = ox[1];
    sy[1] = oy[1];
    nu[0] = 9;
    nu[1] = 0;
    inu = 2;

    while (at_num < o_num) {	/* Position of last point to be    */
	if (o_num - at_num > 14)	/* processed in a segment.         */
	    n = at_num + 9;	/* There must be at least 6 points */
	else			/* in the current segment.         */
	    n = o_num;
	sx[0] = sx[nu[1]];	/* Last point written becomes      */
	sy[0] = sy[nu[1]];	/* first of new segment.           */
	if (inu > 1) {		/* One point was keeped in the     *//* previous segment :              */
	    sx[1] = sx[k];	/* Last point of the old segment   */
	    sy[1] = sy[k];	/* becomes second of the new one.  */
	    k = 1;
	}
	else {			/* No point keeped : farest point  */
	    sx[1] = sx[ij];	/* is loaded in second position    */
	    sy[1] = sy[ij];	/* to avoid cutting lines with     */
	    sx[2] = sx[k];	/* small cuvature.                 */
	    sy[2] = sy[k];	/* First point of previous segment */
	    k = 2;		/* becomes the third one.          */
	}
	/* Loding remaining points         */
	for (j = at_num; j < n; j++) {
	    k++;
	    sx[k] = ox[j];
	    sy[k] = oy[j];
	}

	jd = 0;
	ja = k;
	nt[0] = 0;
	nu[0] = k;
	inu = 0;
	it = 0;
	for (;;) {
	    if (jd + 1 == ja)	/* Exploration of segment terminated */
		goto endseg;

	    dx = sx[ja] - sx[jd];
	    dy = sy[ja] - sy[jd];
	    t = thresh * hypot(dx, dy);
	    beta = sx[jd] * sy[ja] - sx[ja] * sy[jd];

	    /* Initializing ij, we don't take 0 as initial value
	     ** for fpdist, in case ja and jd are the same
	     */
	    ij = (ja + jd + 1) >> 1;
	    fpdist = 1.0;

	    for (j = jd + 1; j < ja; j++) {
		sqdist = fabs(dx * sy[j] - dy * sx[j] + beta);
		if (sqdist > fpdist) {
		    ij = j;
		    fpdist = sqdist;
		}
	    }
	    if (fpdist > t) {	/* We found a point to be keeped    *//* Restart from farest point        */
		jd = ij;
		nt[++it] = ij;
	    }
	    else
	  endseg:{		/* All points are inside threshold. */
		/* Former start becomes new end     */
		nu[++inu] = jd;
		if (--it < 0)
		    break;
		ja = jd;
		jd = nt[it];
	    }
	}
	for (j = inu - 1; j > 0; j--) {	/* Copy of segment's keeped points  */
	    i = nu[j];
	    ox[n_num] = sx[i];
	    oy[n_num] = sy[i];
	    n_num++;
	}
	at_num = n;
    }
    i = nu[0];
    ox[n_num] = sx[i];
    oy[n_num] = sy[i];
    n_num++;
    return n_num;
}
