/***************************************************************
 *
 * MODULE:       v.delaunay
 *
 * AUTHOR(S):    Martin Pavlovsky (Google SoC 2008, Paul Kelly mentor)
 *               Based on "dct" by Geoff Leach, Department of Computer 
 *               Science, RMIT.
 *
 * PURPOSE:      Creates a Delaunay triangulation vector map
 *
 * COPYRIGHT:    (C) RMIT 1993
 *               (C) 2008-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 * 
 * The following notices apply to portions of the code originally
 * derived from work by Geoff Leach of RMIT:
 *
 *   Author: Geoff Leach, Department of Computer Science, RMIT.
 *   email: gl@cs.rmit.edu.au
 *
 *   Date: 6/10/93
 *
 *   Version 1.0
 *   
 *   Copyright (c) RMIT 1993. All rights reserved.
 *
 *   License to copy and use this software purposes is granted provided 
 *   that appropriate credit is given to both RMIT and the author.
 *
 *   License is also granted to make and use derivative works provided
 *   that appropriate credit is given to both RMIT and the author.
 *
 *   RMIT makes no representations concerning either the merchantability 
 *   of this software or the suitability of this software for any particular 
 *   purpose.  It is provided "as is" without express or implied warranty 
 *   of any kind.
 *
 *   These notices must be retained in any copies of any part of this software.
 * 
 **************************************************************/

#ifndef GEOM_PRIMITIVES_H
#define GEOM_PRIMITIVES_H

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define CREATE_VECTOR(p1, p2, dx, dy) ((dx) = (p2)->x - (p1)->x, (dy) = (p2)->y - (p1)->y)

#define DOT_PRODUCT_2V(a1, a2, b1, b2) ((a1) * (b1) + (a2) * (b2))

#define CROSS_PRODUCT_2V(a1, a2, b1, b2) ((a1) * (b2) - (a2) * (b1))
/*
   cross-product around p2
   ((p2->x - p1->x) * (p3->y - p2->y) - (p2->y - p1->y) * (p3->x - p2->x))

   around p1
 */
#define CROSS_PRODUCT_3P(p1, p2, p3) (((p2)->x - (p1)->x) * ((p3)->y - (p1)->y) - ((p2)->y - (p1)->y) * ((p3)->x - (p1)->x))

/* predicate testing if p3 is to the left of the line through p1 and p2 */
#define LEFT_OF(p1, p2, p3) (CROSS_PRODUCT_3P(p1, p2, p3) > 0)

/* predicate testing if p3 is to the right of the line through p1 and p2 */
#define RIGHT_OF(p1, p2, p3) (CROSS_PRODUCT_3P(p1, p2, p3) < 0)

#endif
