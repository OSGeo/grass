/*!
   \file lib/ogsf/gv_quick.c

   \brief OGSF library - 

   GRASS OpenGL gsurf OGSF Library 

   Trying some stuff to draw a quick version of a vector map, to represent
   it when doing interactive translations.

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown, USACERL (December 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "rowcol.h"

/*!
   \brief target number of desired points to represent entire file
 */
#define TFAST_PTS 800

/*!
   \brief max number of lines desired
 */
#define MFAST_LNS  400

static geoline *copy_line(geoline *);
static geoline *thin_line(geoline *, float);

/*!
   \brief Copy line

   \param gln source line (geoline)

   \return pointer to geoline struct
   \return on failure
 */
static geoline *copy_line(geoline * gln)
{
    geoline *newln;
    int i, np;

    newln = (geoline *) G_malloc(sizeof(geoline));	/* G_fatal_error */
    if (!newln) {
	return (NULL);
    }

    np = newln->npts = gln->npts;

    if (2 == (newln->dims = gln->dims)) {
	newln->p2 = (Point2 *) G_calloc(np, sizeof(Point2));	/* G_fatal_error */
	if (!newln->p2) {
	    return (NULL);
	}

	for (i = 0; i < np; i++) {
	    newln->p2[i][X] = gln->p2[i][X];
	    newln->p2[i][Y] = gln->p2[i][Y];
	}
    }
    else {
	newln->p3 = (Point3 *) G_calloc(np, sizeof(Point3));	/* G_fatal_error */
	if (!newln->p3) {
	    return (NULL);
	}

	for (i = 0; i < np; i++) {
	    newln->p3[i][X] = gln->p3[i][X];
	    newln->p3[i][Y] = gln->p3[i][Y];
	    newln->p3[i][Z] = gln->p3[i][Z];
	}
    }

    newln->next = NULL;

    return (newln);
}


/*!
   \brief Thin line

   For now, just eliminate points at regular interval

   \param gln line (geoline)
   \param factor

   \return pointer to geoline struct
   \return NULL on failure
 */
static geoline *thin_line(geoline * gln, float factor)
{
    geoline *newln;
    int i, nextp, targp;

    newln = (geoline *) G_malloc(sizeof(geoline));	/* G_fatal_error */
    if (!newln) {
	return (NULL);
    }

    targp = (int)(gln->npts / factor);

    if (targp < 2) {
	targp = 2;
    }

    newln->npts = targp;

    if (2 == (newln->dims = gln->dims)) {
	newln->p2 = (Point2 *) G_calloc(targp, sizeof(Point2));	/* G_fatal_error */
	if (!newln->p2) {
	    return (NULL);
	}

	for (i = 0; i < targp; i++) {
	    if (i == targp - 1) {
		nextp = gln->npts - 1;	/* avoid rounding error */
	    }
	    else {
		nextp = (int)((i * (gln->npts - 1)) / (targp - 1));
	    }

	    newln->p2[i][X] = gln->p2[nextp][X];
	    newln->p2[i][Y] = gln->p2[nextp][Y];
	}
    }
    else {
	newln->p3 = (Point3 *) G_calloc(targp, sizeof(Point3));	/* G_fatal_error */
	if (!newln->p3) {
	    return (NULL);
	}

	for (i = 0; i < targp; i++) {
	    if (i == targp - 1) {
		nextp = gln->npts - 1;	/* avoid rounding error */
	    }
	    else {
		nextp = (int)((i * (gln->npts - 1)) / (targp - 1));
	    }

	    newln->p3[i][X] = gln->p3[nextp][X];
	    newln->p3[i][Y] = gln->p3[nextp][Y];
	    newln->p3[i][Z] = gln->p3[nextp][Z];
	}
    }

    newln->next = NULL;

    return (newln);
}

/*!
   \brief Get line width

   \param gln line (geoline)

   \return line width
 */
float gv_line_length(geoline * gln)
{
    int n;
    float length = 0.0;

    for (n = 0; n < gln->npts - 1; n++) {
	if (gln->p2) {
	    length += GS_P2distance(gln->p2[n + 1], gln->p2[n]);
	}
	else {
	    length += GS_distance(gln->p3[n + 1], gln->p3[n]);
	}
    }

    return (length);
}

/*!
   \brief Get number of line vertices

   \param gln line (geoline)

   \return number of vertices
 */
int gln_num_points(geoline * gln)
{
    int np = 0;
    geoline *tln;

    for (tln = gln; tln; tln = tln->next) {
	np += tln->npts;
    }

    return (np);
}

/*!
   \brief Get number of points in vector

   \param gv vector (geovect)

   \return number of points
 */
int gv_num_points(geovect * gv)
{
    return (gln_num_points(gv->lines));
}



/*!
   \brief Decimate line

   strategy here: if line has more than average number of points, decimate
   by eliminating points, otherwise decimate by eliminating shorter lines

   \param gv vector (geovect)

   \return
 */
int gv_decimate_lines(geovect * gv)
{
    int T_pts, A_ppl, N_s;
    float decim_factor, slength[MFAST_LNS], T_slength, A_slength;
    geoline *gln, *prev;

    /* should check if already exists & free if != gv->lines */
    if (TFAST_PTS > (T_pts = gv_num_points(gv))) {
	gv->fastlines = gv->lines;

	return (1);
    }

    N_s = 0;
    T_slength = 0.0;
    decim_factor = T_pts / TFAST_PTS;
    A_ppl = T_pts / gv->n_lines;	/* (int) Average points per line */

    prev = NULL;

    for (gln = gv->lines; gln; gln = gln->next) {
	if (gln->npts > A_ppl) {
	    if (prev) {
		prev->next = thin_line(gln, decim_factor);
		prev = prev->next;
	    }
	    else {
		prev = gv->fastlines = thin_line(gln, decim_factor);
	    }
	}
	else if (N_s < MFAST_LNS) {
	    T_slength += slength[N_s++] = gv_line_length(gln);
	}
    }

    A_slength = T_slength / N_s;
    N_s = 0;

    for (gln = gv->lines; gln; gln = gln->next) {
	if (gln->npts <= A_ppl) {
	    if (N_s < MFAST_LNS) {
		if (slength[N_s++] > A_slength) {
		    if (prev) {
			prev->next = copy_line(gln);
			prev = prev->next;
		    }
		    else {
			prev = gv->fastlines = copy_line(gln);
		    }
		}
	    }
	}
    }

    G_debug(3, "Decimated lines have %d points.",
	    gln_num_points(gv->fastlines));

    return (1);
}
