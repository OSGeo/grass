/*!
   \file lib/vector/Vlib/bridges.c

   \brief Vector library - clean geometry (bridges)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/rbtree.h>
#include <grass/glocale.h>

static int cmp_int(const void *a, const void *b)
{
    const int *ai = a;
    const int *bi = b;
    
    return (*ai < *bi ? -1 : (*ai > *bi));
}

static void remove_bridges(struct Map_info *Map, int chtype, struct Map_info *Err, 
                           int *lrm, int *brm);

/*!
   \brief Remove bridges from vector map.

   Remove bridges (type boundary) connecting areas to islands or 2
   islands.  Islands and areas must be already clean, i.e. without
   dangles.  Bridge may be formed by more lines.  Optionally deleted
   bridges are written to error map.  Input map must be opened on
   level 2 for update at least on level GV_BUILD_BASE

   \param Map input map where bridges are deleted
   \param Err vector map where deleted bridges are written or NULL
   \param lines_removed number of lines removed
   \param bridges_removed Err number of bridges removed
*/
void Vect_remove_bridges(struct Map_info *Map, struct Map_info *Err,
                         int *lines_removed, int *bridges_removed)
{
    remove_bridges(Map, 0, Err, lines_removed, bridges_removed);
}

/*!
   \brief Change type of bridges in vector map.

   Change the type of bridges (type boundary) connecting areas to
   islands or 2 islands. Islands and areas must be already clean,
   i.e. without dangles. Bridge may be formed by more lines.
   Optionally changed bridges are written to error map.  Input map
   must be opened on level 2 for update at least on level
   GV_BUILD_BASE.

   \param Map input map where bridges are changed
   \param Err vector map where changed bridges are written or NULL
   \param lines_changed number of lines changed
   \param bridges_changed Err number of bridges changed
*/
void Vect_chtype_bridges(struct Map_info *Map, struct Map_info *Err,
                    int *lines_changed, int *bridges_changed)
{
    remove_bridges(Map, 1, Err, lines_changed, bridges_changed);
}

/* 
   Called by Vect_remove_bridges() and Vect_chtype_bridges():
   chtype = 0 -> works like Vect_remove_bridges()
   chtype = 1 -> works like Vect_chtype_bridges()

   Algorithm: Go thorough all lines, 
   if both sides of the line have left and side 0 (candidate) do this check:
   follow adjacent lines in one direction (nearest to the right at the end node),
   if we reach this line again without dangle in the way, but with this line 
   traversed from other side it is a bridge.

   List of all lines in chain is created during the cycle.
 */
void remove_bridges(struct Map_info *Map, int chtype, struct Map_info *Err,
                    int *lrm, int *brm)
{
    int type, nlines, line, *bline;
    int left, right, node1, node2, current_line, next_line, abs_line;
    int bridges_removed = 0;	/* number of removed bridges */
    int lines_removed = 0;	/* number of lines removed */
    struct Plus_head *Plus;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct RB_TREE *CycleTree, *BridgeTree;
    struct RB_TRAV trav;

    int dangle, other_side;

    Plus = &(Map->plus);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    CycleTree = rbtree_create(cmp_int, sizeof(int));
    BridgeTree = rbtree_create(cmp_int, sizeof(int));

    nlines = Vect_get_num_lines(Map);

    G_debug(1, "nlines =  %d", nlines);

    for (line = 1; line <= nlines; line++) {
	G_percent(line, nlines, 1);
	if (!Vect_line_alive(Map, line))
	    continue;

	type = Vect_read_line(Map, NULL, NULL, line);
	if (!(type & GV_BOUNDARY))
	    continue;

	Vect_get_line_areas(Map, line, &left, &right);

	if (left != 0 || right != 0)
	    continue;		/* Cannot be bridge */

	G_debug(2, "line %d - bridge candidate", line);

	Vect_get_line_nodes(Map, line, &node1, &node2);

	if (abs(node1) == abs(node2))
	    continue;		/* either zero length or loop -> cannot be a bridge */

	current_line = -line;	/* we start with negative (go forward, node2 ) */

	G_debug(3, "current line: %d", line);
	dangle = 0;
	other_side = 0;
	rbtree_clear(CycleTree);
	rbtree_clear(BridgeTree);

	while (1) {
	    next_line =
		dig_angle_next_line(Plus, current_line, GV_RIGHT,
				    GV_BOUNDARY, NULL);
	    abs_line = abs(next_line);

	    /* Add this line to the list */
	    if (rbtree_find(CycleTree, (void *)&abs_line)) {
		if (!rbtree_find(BridgeTree, (void *)&abs_line)) {
		    rbtree_insert(BridgeTree, (void *)&abs_line);
		}
	    }
	    else {
		rbtree_insert(CycleTree, (void *)&abs_line);
	    }

	    if (abs(next_line) == abs(current_line)) {
		G_debug(4, "  dangle -> no bridge");
		dangle = 1;
		break;
	    }
	    if (abs(next_line) == line) {	/* start line reached */
		/* which side */
		if (next_line < 0) {	/* other side (connected by node 2) */
		    G_debug(5, "  other side reached");
		    other_side = 1;
		}
		else {		/* start side */
		    break;
		}
	    }

	    current_line = -next_line;	/* change the sign to look at the next node in following cycle */
	}

	if (!dangle && other_side) {
	    G_debug(3, " line %d is part of bridge chain", line);

	    rbtree_init_trav(&trav, BridgeTree);
	    /* for (i = 0; i < BridgeList->n_values; i++) { */
	    while ((bline = rbtree_traverse(&trav))) {
		Vect_read_line(Map, Points, Cats, *bline);

		if (Err) {
		    Vect_write_line(Err, GV_BOUNDARY, Points, Cats);
		}

		if (!chtype)
		    Vect_delete_line(Map, *bline);
		else
		    Vect_rewrite_line(Map, *bline, GV_LINE,
				      Points, Cats);

		lines_removed++;
	    }
	    bridges_removed++;
	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    rbtree_destroy(CycleTree);
    rbtree_destroy(BridgeTree);

    if (lrm)
	*lrm = lines_removed;
    if (brm)
	*brm = bridges_removed;

    G_verbose_message(_("Removed lines: %d"), lines_removed);
    G_verbose_message(_("Removed bridges: %d"), bridges_removed);
}
