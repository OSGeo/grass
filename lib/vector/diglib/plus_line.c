
/**
 * \file plus_line.c
 *
 * \brief Vector library - update topo for lines (lower level functions)
 *
 * Lower level functions for reading/writing/manipulating vectors.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author CERL (probably Dave Gerdes), Radim Blazek
 *
 * \date 2001-2008
 */

#include <sys/types.h>
#include <stdlib.h>
#include <grass/vector.h>

static int add_line(struct Plus_head *plus, int lineid, int type, const struct line_pnts *Points,
		    const struct bound_box *box, off_t offset)
{
    int node, lp, node_new;
    struct P_line *line;

    plus->Line[lineid] = dig_alloc_line();
    line = plus->Line[lineid];

    line->type = type;
    line->offset = offset;

    dig_spidx_add_line(plus, lineid, box);
    if (plus->uplist.do_uplist) {
	dig_line_add_updated(plus, lineid);
	plus->uplist.uplines_offset[plus->uplist.n_uplines - 1] = line->offset;
    }
    
    if (type & GV_POINT) {
	line->topo = NULL;
	return (lineid);
    }
    
    line->topo = dig_alloc_topo(type);

    if (type & GV_CENTROID) {
	struct P_topo_c *topo = (struct P_topo_c *)line->topo;

	topo->area = 0;
	return (lineid);
    }

    /* Add nodes for lines */
    G_debug(3, "Register node: type = %d,  %f,%f", type, Points->x[0],
	    Points->y[0]);

    /* Start node */
    node = dig_find_node(plus, Points->x[0], Points->y[0], Points->z[0]);
    G_debug(3, "node = %d", node);
    if (node == 0) {
	node = dig_add_node(plus, Points->x[0], Points->y[0], Points->z[0]);
	G_debug(3, "Add new node: %d", node);
        node_new = TRUE;
    }
    else {
        G_debug(3, "Old node found: %d", node);
        node_new = FALSE;
    }
    
    if (type == GV_LINE) {
	struct P_topo_l *topo = (struct P_topo_l *)line->topo;

	topo->N1 = node;
	topo->N2 = 0;
    }
    else if (type == GV_BOUNDARY) {
	struct P_topo_b *topo = (struct P_topo_b *)line->topo;

	topo->N1 = node;
	topo->N2 = 0;
	topo->left = 0;
	topo->right = 0;
    }

    dig_node_add_line(plus, node, lineid, Points, type);
    if (plus->uplist.do_uplist)
	dig_node_add_updated(plus, node_new ? -node : node);

    /* End node */
    lp = Points->n_points - 1;
    G_debug(3, "Register node %f,%f", Points->x[lp], Points->y[lp]);
    node = dig_find_node(plus, Points->x[lp], Points->y[lp],
			 Points->z[lp]);
    G_debug(3, "node = %d", node);
    if (node == 0) {
	node = dig_add_node(plus, Points->x[lp], Points->y[lp],
			    Points->z[lp]);
	G_debug(3, "Add new node: %d", node);
        node_new = TRUE;
    }
    else {
	G_debug(3, "Old node found: %d", node);
        node_new = FALSE;
    }
    if (type == GV_LINE) {
	struct P_topo_l *topo = (struct P_topo_l *)line->topo;

	topo->N2 = node;
    }
    else if (type == GV_BOUNDARY) {
	struct P_topo_b *topo = (struct P_topo_b *)line->topo;

	topo->N2 = node;
    }

    dig_node_add_line(plus, node, -lineid, Points, type);
    if (plus->uplist.do_uplist)
	dig_node_add_updated(plus, node_new ? -node : node);

    return (lineid);
}

/*!
 * \brief Add new line to Plus_head structure.
 *
 * \param[in,out] plus pointer to Plus_head structure
 * \param type feature type
 * \param Points line geometry
 * \param box bounding box
 * \param offset line offset
 *
 * \return -1 on error      
 * \return line id
 */
int
dig_add_line(struct Plus_head *plus, int type, const struct line_pnts *Points,
             const struct bound_box *box, off_t offset)
{
    int ret;
    
    /* First look if we have space in array of pointers to lines
     *  and reallocate if necessary */
    if (plus->n_lines >= plus->alloc_lines) {	/* array is full */
	if (dig_alloc_lines(plus, 1000) == -1)
	    return -1;
    }

    ret = add_line(plus, plus->n_lines + 1, type, Points, box, offset);

    if (ret == -1)
	return ret;

    plus->n_lines++;

    switch (type) {
    case GV_POINT:
	plus->n_plines++;
	break;
    case GV_LINE:
	plus->n_llines++;
	break;
    case GV_BOUNDARY:
	plus->n_blines++;
	break;
    case GV_CENTROID:
	plus->n_clines++;
	break;
    case GV_FACE:
	plus->n_flines++;
	break;
    case GV_KERNEL:
	plus->n_klines++;
	break;
    }

    return ret;
}

/*!
 * \brief Restore line in Plus_head structure.
 *
 * \param[in,out] plus pointer to Plus_head structure
 * \param type feature type
 * \param Points line geometry
 * \param box bounding box
 * \param offset line offset
 *
 * \return -1 on error      
 * \return line id
 */
int
dig_restore_line(struct Plus_head *plus, int lineid,
		 int type, const struct line_pnts *Points,
		 const struct bound_box *box, off_t offset)
{
    if (lineid < 1 || lineid > plus->n_lines) {
	return -1;
    }

    return add_line(plus, lineid, type, Points, box, offset);    
}

/*!
 * \brief Delete line from Plus_head structure.
 *
 * Doesn't update area/isle references (dig_del_area() or dig_del_isle()) must be
 * run before the line is deleted if the line is part of such
 * structure). Update is info about line in nodes. If this line is
 * last in node then node is deleted.
 *
 * \param[in,out] plus pointer to Plus_head structure
 * \param[in] line line id
 * \param[in] x,y,z coordinates
 *
 * \return -1 on error
 * \return  0 OK
 *
 */
int dig_del_line(struct Plus_head *plus, int line, double x, double y, double z)
{
    int i, mv;
    plus_t N1 = 0, N2 = 0;
    struct P_line *Line;
    struct P_node *Node;

    G_debug(3, "dig_del_line() line =  %d", line);

    Line = plus->Line[line];
    dig_spidx_del_line(plus, line, x, y, z);

    if (plus->uplist.do_uplist) {
	dig_line_add_updated(plus, line);
	plus->uplist.uplines_offset[plus->uplist.n_uplines - 1] = -1 * Line->offset;
    }
    
    if (!(Line->type & GV_LINES)) {
	/* Delete line */
	dig_free_line(Line);
	plus->Line[line] = NULL;

	return 0;
    }

    /* Delete from nodes (and nodes) */
    if (Line->type == GV_LINE) {
	struct P_topo_l *topo = (struct P_topo_l *)Line->topo;

	N1 = topo->N1;
    }
    else if (Line->type == GV_BOUNDARY) {
	struct P_topo_b *topo = (struct P_topo_b *)Line->topo;

	N1 = topo->N1;
    }

    Node = plus->Node[N1];

    mv = 0;
    for (i = 0; i < Node->n_lines; i++) {
	if (mv) {
	    Node->lines[i - 1] = Node->lines[i];
	    Node->angles[i - 1] = Node->angles[i];
	}
	else {
	    if (Node->lines[i] == line)
		mv = 1;
	}
    }
    Node->n_lines--;

    if (Node->n_lines == 0) {
	G_debug(3, "    node %d has 0 lines -> delete", N1);
	dig_spidx_del_node(plus, N1);
	/* free structures */
	dig_free_node(Node);
	plus->Node[N1] = NULL;
    }
    if (plus->uplist.do_uplist) {
        dig_node_add_updated(plus, Node->n_lines > 0 ? N1 : -N1);
    }
    
    if (Line->type == GV_LINE) {
	struct P_topo_l *topo = (struct P_topo_l *)Line->topo;

	N2 = topo->N2;
    }
    else if (Line->type == GV_BOUNDARY) {
	struct P_topo_b *topo = (struct P_topo_b *)Line->topo;

	N2 = topo->N2;
    }

    Node = plus->Node[N2];
    mv = 0;
    for (i = 0; i < Node->n_lines; i++) {
	if (mv) {
	    Node->lines[i - 1] = Node->lines[i];
	    Node->angles[i - 1] = Node->angles[i];
	}
	else {
	    if (Node->lines[i] == -line)
		mv = 1;
	}
    }
    Node->n_lines--;

    if (Node->n_lines == 0) {
	G_debug(3, "    node %d has 0 lines -> delete", N2);
	dig_spidx_del_node(plus, N2);
	/* free structures */
	dig_free_node(Node);
	plus->Node[N2] = NULL;
    }
    if (plus->uplist.do_uplist) {
        dig_node_add_updated(plus, Node->n_lines > 0 ? N2 : -N2);
    }

    /* Delete line */
    dig_free_line(Line);
    plus->Line[line] = NULL;

    return 0;
}

/*!
 * \brief Get area number on line side.
 *
 * \param[in] plus pointer Plus_head structure
 * \param[in] line line id
 * \param[in] side side id (GV_LEFT || GV_RIGHT)
 *
 * \return  area number 
 * \return  0 no area
 * \return -1 on error
 */
plus_t dig_line_get_area(struct Plus_head * plus, plus_t line, int side)
{
    struct P_line *Line;
    struct P_topo_b *topo;

    Line = plus->Line[line];
    if (!Line) /* dead */
	return -1;
    
    if (Line->type != GV_BOUNDARY)
	return -1;

    topo = (struct P_topo_b *)Line->topo;
    if (side == GV_LEFT) {
	G_debug(3,
		"dig_line_get_area(): line = %d, side = %d (left), area = %d",
		line, side, topo->left);
	return (topo->left);
    }
    if (side == GV_RIGHT) {
	G_debug(3,
		"dig_line_get_area(): line = %d, side = %d (right), area = %d",
		line, side, topo->right);

	return (topo->right);
    }

    return (-1);
}

/*!
 * \brief Set area number on line side
 *
 * \param[in] plus pointer Plus_head structure
 * \param[in] line line id
 * \param[in] side side id (GV_LEFT || GV_RIGHT)
 * \param[in] area area id
 *
 * \return 1
 */
int
dig_line_set_area(struct Plus_head *plus, plus_t line, int side, plus_t area)
{
    struct P_line *Line;
    struct P_topo_b *topo;

    Line = plus->Line[line];
    if (Line->type != GV_BOUNDARY)
	return (0);

    topo = (struct P_topo_b *)Line->topo;

    if (side == GV_LEFT) {
	topo->left = area;
    }
    else if (side == GV_RIGHT) {
	topo->right = area;
    }

    return (1);
}

/*!
 * \brief Set line bounding box
 *
 * \param[in] plus pointer Plus_head structure
 * \param[in] line line id
 * \param[in] Box  bounding box
 *
 * \return 1
 */
/*
int dig_line_set_box(struct Plus_head *plus, plus_t line, struct bound_box * Box)
{
    struct P_line *Line;

    Line = plus->Line[line];

    Line->N = Box->N;
    Line->S = Box->S;
    Line->E = Box->E;
    Line->W = Box->W;
    Line->T = Box->T;
    Line->B = Box->B;

    return (1);
}
*/

/*!
 * \brief Get line bounding box saved in topo
 * 
 * \param[in] plus pointer Plus_head structure
 * \param[in] line line id
 * \param[in,out] Box bounding box
 *
 * \return 1
 */
/*
int dig_line_get_box(struct Plus_head *plus, plus_t line, struct bound_box * Box)
{
    struct P_line *Line;

    Line = plus->Line[line];

    Box->N = Line->N;
    Box->S = Line->S;
    Box->E = Line->E;
    Box->W = Line->W;
    Box->T = Line->T;
    Box->B = Line->B;

    return (1);
}
*/
