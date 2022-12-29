
/****************************************************************************
 *
 * MODULE:       r.contour
 *
 * AUTHOR(S):    Terry Baker - CERL
 *               Andrea Aime <aaime liberto it>
 *
 * PURPOSE:      Produces a vector map of specified contours from a
 *               raster map layer.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* cont.c */
void contour(double *, int, struct Map_info, DCELL **, struct Cell_head, int);
int checkedge(DCELL, DCELL, double);

/* main.c */
DCELL **get_z_array(int, int, int);
double *getlevels(struct Option *, struct Option *, struct Option *,
		  struct Option *, struct FPRange *, int *);
void displaceMatrix(DCELL **, int, int, double *, int);

#endif /* __LOCAL_PROTO_H__ */
