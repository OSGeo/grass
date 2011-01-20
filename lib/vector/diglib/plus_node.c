
/**
 * \file plus_node.c
 *
 * \brief Vector library - update topo for nodes (lower level functions)
 *
 * Lower level functions for reading/writing/manipulating vectors.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author CERL (probably Dave Gerdes), Radim Blazek
 *
 * \date 2001-2006
 */

#include <stdlib.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static double dist_squared(double, double, double, double);

/*!
 * \brief Add line info to node
 *
 * Line will be negative if END node
 *
 * 'node' must of course already exist space will be alloced to add 'line' to array
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] nodeid node id
 * \param[in] lineid line id
 * \param[in] points line geometry
 * \param[in] type line type 
 *
 * \return -1 on error      
 * \return 0 line not added (degenerate)
 * \return new number of lines in node 
 */
int
dig_node_add_line(struct Plus_head *plus, int nodeid, int lineid,
		  const struct line_pnts *points, int type)
{
    register int i, j, nlines;
    float angle;
    int ret;
    struct P_node *node;

    G_debug(3, "dig_node_add_line(): node = %d line = %d", nodeid, lineid);

    node = plus->Node[nodeid];
    nlines = node->n_lines;

    /* reallocate memory */
    ret = dig_node_alloc_line(node, 1);
    if (ret == -1)
	return -1;

    if (type & GV_LINES) {
	if (lineid < 0)
	    angle = dig_calc_end_angle(points, 0);
	else
	    angle = dig_calc_begin_angle(points, 0);
    }
    else {
	angle = -9.;
    }
    G_debug(3, "    angle = %f", angle);

    /* make sure the new angle is less than the empty space at end */
    node->angles[nlines] = 999.;

    for (i = 0; i <= nlines; i++) {	/* alloced for 1 more */
	if (angle < node->angles[i]) {
	    /* make room for insertion */
	    for (j = nlines - 1; j >= i; j--) {
		node->angles[j + 1] = node->angles[j];
		node->lines[j + 1] = node->lines[j];
	    }
	    node->angles[i] = angle;
	    node->lines[i] = lineid;
	    break;
	}
    }

    node->n_lines++;

    G_debug(3,
	    "dig_node_add_line(): line %d added position %d n_lines: %d angle %f",
	    lineid, i, node->n_lines, angle);

    return ((int)node->n_lines);
}


/*!
 * \brief Add new node to plus structure 
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] x,y,z coordinates
 *
 * \return -1 on error      
 * \return number of node
 */
int dig_add_node(struct Plus_head *plus, double x, double y, double z)
{
    int nnum;
    struct P_node *node;

    /* First look if we have space in array of pointers to nodes
     *  and reallocate if necessary */
    G_debug(3, "dig_add_node(): n_nodes = %d, alloc_nodes = %d",
	    plus->n_nodes, plus->alloc_nodes);
    if (plus->n_nodes >= plus->alloc_nodes) {	/* array is full */
	if (dig_alloc_nodes(plus, 1000) == -1)
	    return -1;
    }

    /* allocate node structure */
    nnum = plus->n_nodes + 1;

    plus->Node[nnum] = dig_alloc_node();

    node = plus->Node[nnum];
    node->x = x;
    node->y = y;
    node->z = z;

    dig_spidx_add_node(plus, nnum, x, y, z);

    plus->n_nodes++;

    G_debug(3, "new node = %d, n_nodes = %d, alloc_nodes = %d", nnum,
	    plus->n_nodes, plus->alloc_nodes);

    return (nnum);
}

/*!
 * \brief Return actual index into node arrays of the first set of matching coordinates
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] x,y coordinates
 * \param[in] thresh threshold value
 *
 * \return node index
 * \return -1 if no node found
 */
int dig_which_node(struct Plus_head *plus, double x, double y, double thresh)
{
    register int i;
    register int first_time;
    register int have_match;
    int winner;
    double least_dist, dist;
    struct P_node *node;

    first_time = 1;
    have_match = 0;
    winner = 0;
    least_dist = 0.0;
    for (i = 1; i <= plus->n_nodes; i++) {
	if (plus->Node[i] == NULL)
	    continue;

	node = plus->Node[i];
	if ((fabs(node->x - x) <= thresh) && (fabs(node->y - y) <= thresh)) {
	    dist = dist_squared(x, y, node->x, node->y);
	    if (first_time) {
		least_dist = dist;
		first_time = 0;
		winner = i;
		have_match = 1;
	    }
	    if (dist < least_dist) {
		least_dist = dist;
		winner = i;
	    }
	}
    }

    if (!have_match)
	return (-1);

    return (winner);
}				/*  which_node ()  */

/*!
 * \brief Return line angle
 *
 * Lines is specified by line id in topology, NOT by order number.
 * Negative id if looking for line end point.
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] nodeid node id
 * \param[in] lineid line id
 *
 * \return line angle <-PI,PI>
 * \return 0 not reached
 */
float dig_node_line_angle(struct Plus_head *plus, int nodeid, int lineid)
{
    int i, nlines;
    struct P_node *node;

    G_debug(3, "dig_node_line_angle: node = %d line = %d", nodeid, lineid);

    node = plus->Node[nodeid];
    nlines = node->n_lines;

    for (i = 0; i < nlines; i++) {
	if (node->lines[i] == lineid)
	    return (node->angles[i]);
    }

    G_fatal_error(_("Attempt to read line angle for the line which is not connected to the node: "
		   "node %d, line %d"), nodeid, lineid);

    return 0.0;			/* not reached */
}

static double dist_squared(double x1, double y1, double x2, double y2)
{
    double dx, dy;

    dx = x1 - x2;
    dy = y1 - y2;
    return (dx * dx + dy * dy);
}
