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

#ifndef EDGE_H
#define EDGE_H

#define ORG(e)   ((e)->org)
#define DEST(e)  ((e)->dest)
#define ONEXT(e) ((e)->onext)
#define OPREV(e) ((e)->oprev)
#define DNEXT(e) ((e)->dnext)
#define DPREV(e) ((e)->dprev)

#define OTHER_VERTEX(e,p) (ORG(e) == (p) ? DEST(e) : ORG(e))
#define NEXT(e,p)         (ORG(e) == (p) ? ONEXT(e) : DNEXT(e))
#define PREV(e,p)         (ORG(e) == (p) ? OPREV(e) : DPREV(e))

#define SAME_EDGE(e1,e2) ((e1) == (e2))

#define LEFT  0
#define RIGHT 1

struct edge *join(struct edge *e1, struct vertex *v1,
		  struct edge *e2, struct vertex *v2, int side);
void delete_edge(struct edge *e);
void splice(struct edge *a, struct edge *b, struct vertex *v);
struct edge *create_edge(struct vertex *v1, struct vertex *v2);

#endif
