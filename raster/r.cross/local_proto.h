/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from
 *               multiple raster map layers.
 *
 * SPDX-FileCopyrightText: 2006 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
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
