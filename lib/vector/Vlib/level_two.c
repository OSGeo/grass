/*!
   \file lib/vector/Vlib/level_two.c

   \brief Vector library - topology level functions

   (C) 2001-2009, 2011-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Update to GRASS 7 by Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static void check_level(const struct Map_info *Map)
{
    if (Map->level < 2)
	G_fatal_error(_("Vector map <%s> is not open at topological level"),
		      Vect_get_full_name(Map));
}

/*!
   \brief Get number of nodes in vector map

   \param Map pointer to Map_info struct

   \return number of nodes
 */
plus_t Vect_get_num_nodes(const struct Map_info *Map)
{
    return (Map->plus.n_nodes);
}

/*!
   \brief Get number of primitives in vector map

   \param Map pointer to Map_info struct
   \param type feature type

   \return number of primitives
 */
plus_t Vect_get_num_primitives(const struct Map_info *Map, int type)
{
    plus_t num = 0;

    if (type & GV_POINT)
	num += Map->plus.n_plines;
    if (type & GV_LINE)
	num += Map->plus.n_llines;
    if (type & GV_BOUNDARY)
	num += Map->plus.n_blines;
    if (type & GV_CENTROID)
	num += Map->plus.n_clines;
    if (type & GV_FACE)
	num += Map->plus.n_flines;
    if (type & GV_KERNEL)
	num += Map->plus.n_klines;

    return num;
}

/*!
   \brief Fetch number of features (points, lines, boundaries, centroids) in vector map

   \param Map pointer to Map_info struct

   \return number of features
 */
plus_t Vect_get_num_lines(const struct Map_info *Map)
{
    return (Map->plus.n_lines);
}

/*!
   \brief Get number of areas in vector map

   \param Map pointer to Map_info struct

   \return number of areas
 */
plus_t Vect_get_num_areas(const struct Map_info *Map)
{
    return (Map->plus.n_areas);
}

/*!
   \brief Fetch number of kernels in vector map

   \param Map pointer to Map_info struct

   \return number of kernels
 */
plus_t Vect_get_num_kernels(const struct Map_info *Map)
{
    return (Map->plus.n_klines);
}


/*!
   \brief Get number of faces in vector map

   \param Map pointer to Map_info struct

   \return number of faces
 */
plus_t Vect_get_num_faces(const struct Map_info *Map)
{
    return (Map->plus.n_flines);
}


/*!
   \brief Fetch number of volumes in vector map

   \param Map pointer to Map_info struct

   \return number of volumes
 */
plus_t Vect_get_num_volumes(const struct Map_info *Map)
{
    return (Map->plus.n_volumes);
}


/*!
   \brief Get number of islands in vector map

   \param Map pointer to Map_info struct

   \return number of islands
 */
plus_t Vect_get_num_islands(const struct Map_info *Map)
{
    return (Map->plus.n_isles);
}


/*!
   \brief Fetch number of holes in vector map

   \param Map pointer to Map_info struct

   \return number of holes
 */
plus_t Vect_get_num_holes(const struct Map_info *Map)
{
    return (Map->plus.n_holes);
}


/*!
   \brief Get number of defined dblinks

   \param Map pointer to Map_info struct

   \return number of dblinks
 */
int Vect_get_num_dblinks(const struct Map_info *Map)
{
    /* available on level 1 ? */
    return (Map->dblnk->n_fields);
}

/*!
   \brief Get number of updated features

   Note: Vect_set_updated() must be called to maintain list of updated
   features

   \param Map pointer to Map_info struct

   \return number of updated features
 */
int Vect_get_num_updated_lines(const struct Map_info *Map)
{
    return (Map->plus.uplist.n_uplines);
}

/*!
   \brief Get updated line by index

   Note: Vect_set_updated() must be called to maintain list of updated
   features

   \param Map pointer to Map_info struct
   \param idx index

   \return updated line
 */
int Vect_get_updated_line(const struct Map_info *Map, int idx)
{
    return (Map->plus.uplist.uplines[idx]);
}

/*!
   \brief Get updated line offset by index

   Note: Vect_set_updated() must be called to maintain list of updated
   features

   \param Map pointer to Map_info struct
   \param idx index

   \return updated line
 */
off_t Vect_get_updated_line_offset(const struct Map_info *Map, int idx)
{
    return (Map->plus.uplist.uplines_offset[idx]);
}

/*!
   \brief Get number of updated nodes

   \param Map pointer to Map_info struct

   \return number of updated nodes
 */
int Vect_get_num_updated_nodes(const struct Map_info *Map)
{
    return (Map->plus.uplist.n_upnodes);
}

/*!
   \brief Get updated (modified) node by index

   Note: Vect_set_updated() must be called to maintain list of updated
   features

   Negative id:
    - if Node[id] is not NULL then the node was added
    - if Node[id] is NULL then the node was deleted
   Positive id:
    - node was updated

   \param Map pointer to Map_info struct
   \param idx index

   \return id of modified node
 */
int Vect_get_updated_node(const struct Map_info *Map, int idx)
{
    return (Map->plus.uplist.upnodes[idx]);
}

/*!
   \brief Get line type

   \param Map pointer to Map_info struct
   \param line line id

   \return line type
 */
int Vect_get_line_type(const struct Map_info *Map, int line)
{
    check_level(Map);
    
    if (!Vect_line_alive(Map, line))
	return 0;
	
    return (Map->plus.Line[line]->type);
}

/*!
   \brief Get node coordinates

   \param Map pointer to Map_info struct
   \param num node id (starts at 1)
   \param[out] x,y,z coordinates values (for 2D coordinates z is NULL)

   \return 0 on success
   \return -1 on error
 */
int Vect_get_node_coor(const struct Map_info *Map, int num, double *x, double *y,
                       double *z)
{
    struct P_node *Node;

    if (num < 1 || num > Map->plus.n_nodes) {
        G_warning(_("Invalid node id: %d"), num);
        return -1;
    }
    
    Node = Map->plus.Node[num];
    *x = Node->x;
    *y = Node->y;

    if (z != NULL)
	*z = Node->z;

    return 0;
}

/*!
   \brief Get line nodes

   \param Map pointer to Map_info struct
   \param line line id
   \param n1, n2 ids of line nodes (or NULL)

   \return 1
 */
int Vect_get_line_nodes(const struct Map_info *Map, int line, int *n1, int *n2)
{
    char type;

    check_level(Map);

    type = Vect_get_line_type(Map, line);

    if (!(type & GV_LINES))
	G_fatal_error(_("Nodes not available for line %d"), line);
    
    if (type == GV_LINE) {
	struct P_topo_l *topo = (struct P_topo_l *)Map->plus.Line[line]->topo;

	if (n1 != NULL)
	    *n1 = topo->N1;
	if (n2 != NULL)
	    *n2 = topo->N2;
    }
    else if (type == GV_BOUNDARY) {
	struct P_topo_b *topo = (struct P_topo_b *)Map->plus.Line[line]->topo;

	if (n1 != NULL)
	    *n1 = topo->N1;
	if (n2 != NULL)
	    *n2 = topo->N2;
    }

    return 1;
}

/*!
   \brief Get area id on the left and right side of the boundary

   Negative area id indicates an isle.
   
   \param Map pointer to Map_info struct
   \param line line id
   \param[out] left,right area id on the left and right side

   \return 1 on success
   \return -1 on failure (topology not available, line is not a boundary)
 */
int Vect_get_line_areas(const struct Map_info *Map, int line, int *left, int *right)
{
    struct P_topo_b *topo;

    check_level(Map);
    
    if (!Map->plus.Line[line]->topo) {
        G_warning(_("Areas not available for line %d"), line);
        return -1;
    }

    if (Vect_get_line_type(Map, line) != GV_BOUNDARY) {
	G_warning(_("Line %d is not a boundary"), line);
        return -1;
    }

    topo = (struct P_topo_b *)Map->plus.Line[line]->topo;
    if (left != NULL)
	*left = topo->left;
                  
    if (right != NULL)
	*right = topo->right;
    
    return 1;
}

/*!
   \brief Get number of lines for node

   \param Map pointer to Map_info struct
   \param node node id

   \return numbers of lines
 */
int Vect_get_node_n_lines(const struct Map_info *Map, int node)
{
    check_level(Map);
    
    return (Map->plus.Node[node]->n_lines);

}

/*!
   \brief Get line id for node line index

   \param Map pointer to Map_info struct
   \param node node id
   \param line line index (range: 0 - Vect_get_node_n_lines())

   \return line id
 */
int Vect_get_node_line(const struct Map_info *Map, int node, int line)
{
    check_level(Map);
    
    return (Map->plus.Node[node]->lines[line]);
}

/*!
   \brief Angle of segment of the line connected to the node

   \param Map pointer to Map_info struct
   \param node node number
   \param line line index (range: 0 - Vect_get_node_n_lines())

   \return angle of segment of the line connected to the node
 */
float Vect_get_node_line_angle(const struct Map_info *Map, int node, int line)
{
    check_level(Map);

    return (Map->plus.Node[node]->angles[line]);
}

/*!
   \brief Get area id the centroid is within

   \param Map pointer to Map_info struct
   \param centroid centroid id

   \return area id the centroid is within
   \return 0 for not in area
   \return negative id if centroid is duplicated in the area
 */
int Vect_get_centroid_area(const struct Map_info *Map, int centroid)
{
    struct P_topo_c *topo;

    check_level(Map);
    
    if (Map->plus.Line[centroid]->type != GV_CENTROID)
	return 0;
    
    topo = (struct P_topo_c *)Map->plus.Line[centroid]->topo;
    if(!topo)
	G_fatal_error(_("Topology info not available for feature %d"), centroid);
    
    return (topo->area);
}

/*!
  \brief Enable/disable maintanance of list of updated lines/nodes

  See Plus_head.uplist for details.
  
  \param Map pointer to Map_info struct
  \param enable TRUE/FALSE to enable/disable
*/
void Vect_set_updated(struct Map_info *Map, int enable)
{
    G_debug(1, "Vect_set_updated(): name = '%s' enabled = %d", Map->name, enable);
    
    check_level(Map);
    
    Map->plus.uplist.do_uplist = enable != 0 ? TRUE : FALSE;
}

/*!
  \brief Reset list of updated lines/nodes

  \param Map pointer to Map_info struct
*/
void Vect_reset_updated(struct Map_info *Map)
{
    struct Plus_head *Plus;

    check_level(Map);
    
    Plus = &(Map->plus);
    dig_line_reset_updated(Plus);
    dig_node_reset_updated(Plus);
}
