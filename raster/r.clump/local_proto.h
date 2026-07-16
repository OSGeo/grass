/****************************************************************************
 *
 * MODULE:       r.clump
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Recategorizes data in a raster map layer by grouping cells
 *               that form physically discrete areas into unique categories.
 *
 * SPDX-FileCopyrightText: 2006 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 ***************************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* clump.c */
CELL clump(int *, int, int, int);
CELL clump_n(int *, char **, int, double, int, int, int);

/* minsize.c */
int merge_small_clumps(int *in_fd, int nin, DCELL *rng, int diag, int min_size,
                       int *n_clumps, int cfd, int out_fd);

#endif /* __LOCAL_PROTO_H__ */

