/*!
   \file lib/vector/Vlib/dangles.c

   \brief Vector library - clean geometry (dangles)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
*/

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#define REMOVE_DANGLE 0
#define CHTYPE_DANGLE 1
#define SELECT_DANGLE 2

static void dangles(struct Map_info *, int, int, double,
		    struct Map_info *, struct ilist *);

/*!
   \brief Remove dangles from vector map.

   Remove dangles of given type shorter than maxlength from vector
   map.

   Line is considered to be a dangle if on at least one end node is no
   other line of given type(s). If a dangle is formed by more lines,
   such string of lines is taken as one dangle and either deleted are
   all parts or nothing.

   Optionally deleted dangles are written to error map.

   Input map must be opened on level 2 for update.

   \param Map input map where have to be deleted
   \param type type of dangles (GV_LINES, GV_LINE or GV_BOUNDARY)
   \param maxlength maxlength of dangles or -1 for all dangles
   \param[out] Err vector map where deleted dangles are written or NULL

   \return
 */
void
Vect_remove_dangles(struct Map_info *Map, int type, double maxlength,
		    struct Map_info *Err)
{
    dangles(Map, type, REMOVE_DANGLE, maxlength, Err, NULL);
}

/*!
   \brief Change boundary dangles to lines.

   Change boundary dangles to lines. 

   Boundary is considered to be a dangle if on at least one end node
   is no other boundary. If a dangle is formed by more boundaries,
   such string of boundaries is taken as one dangle.

   Optionally deleted dangles are written to error map. 

   Input map must be opened on level 2 for update at least on GV_BUILD_BASE.

   \param Map input map where have to be deleted
   \param maxlength maxlength of dangles or -1 for all dangles
   \param[out] Err vector map where deleted dangles are written or NULL

   \return 
 */
void
Vect_chtype_dangles(struct Map_info *Map, double maxlength,
		    struct Map_info *Err)
{
    dangles(Map, 0, CHTYPE_DANGLE, maxlength, Err, NULL);
}

/*!
   \brief Select dangles from vector map.

   Remove dangles of given type shorter than maxlength from vector map.

   Line is considered to be a dangle if on at least one end node is no
   other line of given type(s).  If a dangle is formed by more lines,
   such string of lines is taken as one dangle.

   Input map must be opened on level 2 for update.

   \param Map input map where have to be deleted
   \param type type of dangles (GV_LINES, GV_LINE or GV_BOUNDARY)
   \param maxlength maxlength of dangles or -1 for all dangles
   \param[out] List list of selected features

   \return
 */
void
Vect_select_dangles(struct Map_info *Map, int type, double maxlength,
		    struct ilist *List)
{
    dangles(Map, type, SELECT_DANGLE, maxlength, NULL, List);
}

/*
   Line is considered to be a dangle if on at least one end node is no
   other line of given type(s). If a dangle is formed by more lines,
   such string of lines is taken as one dangle and either deleted are
   all parts or nothing.  Optionally, if chtype is set to 1, only
   GV_BOUNDARY are checked for dangles, and if dangle is found lines
   are not deleted but rewritten with type GVLINE.  Optionally deleted
   dangles are written to error map.  Input map must be opened on level
   2 for update at least on GV_BUILD_BASE.

   Parameters:
   Map input map where dangles have to be deleted
   type type of dangles 
   option dangle option (REMOVE_DANGLE, CHTYPE_DANGLE, SELECT_DANGLE)
   maxlength maxlength of dangles or -1 for all dangles
   Err vector map where deleted dangles are written or NULL
   List_dangle list of feature (selected dangles) ids 
 */
static void dangles(struct Map_info *Map, int type, int option,
		    double maxlength, struct Map_info *Err,
		    struct ilist *List_dangle)
{
    struct line_pnts *Points;
    struct line_cats *Cats;
    int i, line, ltype, next_line = 0, nnodelines;
    int nnodes, node, node1, node2, next_node;
    int lcount, tmp_next_line = 0;
    double length;
    int dangles_removed;	/* number of removed dangles */
    int lines_removed;		/* number of lines removed */
    struct ilist *List;		/* List of lines in chain */
    char *lmsg;

    next_line = tmp_next_line = 0;
    dangles_removed = 0;
    lines_removed = 0;

    type &= GV_LINES;		/* to work only with lines and boundaries */

    if (option == CHTYPE_DANGLE) {
	type = GV_BOUNDARY;	/* process boundaries only */
	lmsg = _("Changed");
    }
    else if (option == REMOVE_DANGLE) {
	lmsg = _("Removed");
    }
    else {
	lmsg = _("Selected");
    }

    if (List_dangle)
	Vect_reset_list(List_dangle);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    List = Vect_new_list();

    nnodes = Vect_get_num_nodes(Map);
    G_debug(2, "nnodes =  %d", nnodes);

    for (node = 1; node <= nnodes; node++) {
	G_percent(node, nnodes, 1);
	G_debug(3, "node =  %d", node);
	if (!Vect_node_alive(Map, node))
	    continue;

	nnodelines = Vect_get_node_n_lines(Map, node);

	lcount = 0;		/* number of lines of given type */
	for (i = 0; i < nnodelines; i++) {
	    line = Vect_get_node_line(Map, node, i);
	    G_debug(3, "    node line %d = %d", i, line);

	    ltype = Vect_read_line(Map, NULL, NULL, abs(line));

	    if (ltype & type) {
		lcount++;
		next_line = line;
	    }
	}

	Vect_reset_list(List);
	if (lcount == 1) {
	    G_debug(3, "    node %d is dangle -> follow the line %d", node,
		    next_line);

	    while (next_line != 0) {
		Vect_list_append(List, abs(next_line));

		/* Look at the next end of the line if just one another line of the type is connected */
		Vect_get_line_nodes(Map, abs(next_line), &node1, &node2);
		next_node = next_line > 0 ? node2 : node1;

		G_debug(3, "    next_node = %d", next_node);

		nnodelines = Vect_get_node_n_lines(Map, next_node);

		lcount = 0;	/* number of lines of given type (except current next_line) */
		for (i = 0; i < nnodelines; i++) {
		    line = Vect_get_node_line(Map, next_node, i);
		    G_debug(3, "      node line %d = %d", i, line);

		    ltype = Vect_read_line(Map, NULL, NULL, abs(line));

		    if (ltype & type && abs(line) != abs(next_line)) {
			lcount++;
			tmp_next_line = line;
		    }
		}
		if (lcount == 1)
		    next_line = tmp_next_line;
		else
		    next_line = 0;

	    }

	    /* Length of the chain */
	    length = 0;
	    for (i = 0; i < List->n_values; i++) {
		G_debug(3, "  chain line %d = %d", i, List->value[i]);
		ltype = Vect_read_line(Map, Points, NULL, List->value[i]);
		length += Vect_line_length(Points);
	    }

	    if (maxlength < 0 || length < maxlength) {	/* delete the chain */
		G_debug(3, "  delete the chain (length=%g)", length);

		for (i = 0; i < List->n_values; i++) {
		    ltype = Vect_read_line(Map, Points, Cats, List->value[i]);

		    /* Write to Err deleted dangle */
		    if (Err) {
			Vect_write_line(Err, ltype, Points, Cats);
		    }

		    if (option == REMOVE_DANGLE) {
			Vect_delete_line(Map, List->value[i]);
		    }
		    else if (option == CHTYPE_DANGLE) {
			G_debug(3, "  rewrite line %d", List->value[i]);
			Vect_rewrite_line(Map, List->value[i], GV_LINE,
					  Points, Cats);
		    }
		    else {
			if (List_dangle) {
			    Vect_list_append(List_dangle, List->value[i]);
			}
		    }
		    lines_removed++;
		}
	    }			/* delete the chain */

	    dangles_removed++;
	}			/* lcount == 1 */
    }				/* node <= nnodes */
    G_verbose_message(_("%s lines: %d"), lmsg, lines_removed);
    G_verbose_message(_("%s dangles: %d"), lmsg, dangles_removed);
}
