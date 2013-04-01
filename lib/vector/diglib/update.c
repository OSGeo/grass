
/**
 * \file lib/vector/diglib/update.c
 *
 * \brief Vector library - update topology (lower level functions)
 *
 * Lower level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/vector.h>

/*!
   \brief Reset number of updated lines

   \param Plus pointer to Plus_head structure
 */
void dig_line_reset_updated(struct Plus_head *Plus)
{
    Plus->uplist.n_uplines = 0;
}

/*!
   \brief Add new line to updated

   \param Plus pointer to Plus_head structure
   \param line line id
   \param offset line offset (negative offset is ignored)
 */
void dig_line_add_updated(struct Plus_head *Plus, int line)
{
    G_debug(3, "dig_line_add_updated(): line = %d", line);

    /* Check if already in list
    for (i = 0; i < Plus->uplist.n_uplines; i++)
	if (Plus->uplist.uplines[i] == line)
	    return;
    */

    /* Alloc space if needed */
    if (Plus->uplist.n_uplines == Plus->uplist.alloc_uplines) {
	Plus->uplist.alloc_uplines += 1000;
	Plus->uplist.uplines =
	    (int *)G_realloc(Plus->uplist.uplines,
			     Plus->uplist.alloc_uplines * sizeof(int));
	Plus->uplist.uplines_offset =
	    (off_t *)G_realloc(Plus->uplist.uplines_offset,
			     Plus->uplist.alloc_uplines * sizeof(off_t));
    }

    Plus->uplist.uplines[Plus->uplist.n_uplines] = line;
    Plus->uplist.n_uplines++;
}

/*!
   \brief Reset number of updated nodes

   \param Plus pointer to Plus_head structure
 */
void dig_node_reset_updated(struct Plus_head *Plus)
{
    Plus->uplist.n_upnodes = 0;
}

/*!
   \brief Add node to updated

   \param Plus pointer to Plus_head structure
   \param node node id
 */
void dig_node_add_updated(struct Plus_head *Plus, int node)
{
    int i;

    G_debug(3, "dig_node_add_updated(): node = %d", node);

    /* Check if already in list */
    for (i = 0; i < Plus->uplist.n_upnodes; i++)
	if (Plus->uplist.upnodes[i] == node)
	    return;

    /* Alloc space if needed */
    if (Plus->uplist.n_upnodes == Plus->uplist.alloc_upnodes) {
	Plus->uplist.alloc_upnodes += 1000;
	Plus->uplist.upnodes =
	    (int *)G_realloc(Plus->uplist.upnodes,
			     Plus->uplist.alloc_upnodes * sizeof(int));
    }

    Plus->uplist.upnodes[Plus->uplist.n_upnodes] = node;
    Plus->uplist.n_upnodes++;
}
