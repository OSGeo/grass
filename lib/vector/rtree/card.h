
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
* COPYRIGHT:    (C) 2010 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/

#ifndef __CARD__
#define __CARD__

/* balance criteria for node splitting */
/* NOTE: can be changed if needed but
 * must be >= 2 and <= (t)->[nodecard|leafcard] / 2 */
#define MinNodeFill(t) ((t)->minfill_node_split)
#define MinLeafFill(t) ((t)->minfill_leaf_split)

#define MAXKIDS(level, t) ((level) > 0 ? (t)->nodecard : (t)->leafcard)
#define MINFILL(level, t) ((level) > 0 ? (t)->minfill_node_split : (t)->minfill_leaf_split)

#endif
