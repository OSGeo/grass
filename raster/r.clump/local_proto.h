
/****************************************************************************
 *
 * MODULE:       r.clump
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Recategorizes data in a raster map layer by grouping cells
 *               that form physically discrete areas into unique categories.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* clump.c */
CELL clump(int *, int, int, int);
CELL clump_n(int *, char **, int, double, int, int, int);

/* minsize.c */
int merge_small_clumps(int *in_fd, int nin, DCELL *rng,
                        int diag, int min_size, int *n_clumps,
			int cfd, int out_fd);

#endif /* __LOCAL_PROTO_H__ */
