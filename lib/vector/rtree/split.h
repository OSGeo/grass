/****************************************************************************
 * MODULE:       R-Tree library
 *
 * AUTHOR(S):    Antonin Guttman - original code
 *               Daniel Green (green@superliminal.com) - major clean-up
 *                               and implementation of bounding spheres
 *               Markus Metz - file-based and memory-based R*-tree
 *
 * PURPOSE:      Multidimensional index
 *
 * SPDX-FileCopyrightText: 2010 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *****************************************************************************/

/*-----------------------------------------------------------------------------
| Definitions and global variables.
-----------------------------------------------------------------------------*/

/* METHOD 0: R-Tree, quadratic split */
/* METHOD 1: R*-Tree split */
#define METHOD 1

void RTreeInitPVars(struct RTree_PartitionVars *, int, int, struct RTree *);
