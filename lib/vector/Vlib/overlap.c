/*!
   \file lib/vector/Vlib/overlap.c

   \brief Vector library - region/window overlap

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <grass/vector.h>

/*!
  Check if region overlaps with map extent.

  \param Map vector map
  \param n,s,e,w region bounding box

  \return 1 if regions overlap
  \return 0 if not
*/
int
V__map_overlap(struct Map_info *Map, double n, double s, double e, double w)
{
    struct Cell_head W;

    /* updated for Lat lon support 21 Jun 91 */
    W.north = Map->constraint.box.N;
    W.south = Map->constraint.box.S;
    W.east = Map->constraint.box.E;
    W.west = Map->constraint.box.W;
    W.proj = Map->head.proj;

    return G_window_overlap(&W, n, s, e, w);
}
