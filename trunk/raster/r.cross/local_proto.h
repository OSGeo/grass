
/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from
 *               multiple raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __R_CROSS_LOCAL_PROTO_H__
#define __R_CROSS_LOCAL_PROTO_H__

/* cats.c */
int set_cat(CELL, CELL *, struct Categories *);

/* cross.c */
CELL cross(int[], int, int, int);

/* main.c */
int main(int, char *[]);

/* renumber.c */
int renumber(int, int);

/* store.c */
int store_reclass(CELL, int, CELL *);

#endif /* __R_CROSS_LOCAL_PROTO_H__ */
