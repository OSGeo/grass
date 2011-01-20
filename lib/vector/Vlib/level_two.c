/*!
   \file lib/vector/Vlib/level_two.c

   \brief Vector library - topology level functions

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Get number of nodes in vector map

   \param Map vector map

   \return number of nodes
 */
plus_t Vect_get_num_nodes(const struct Map_info *map)
{
    return (map->plus.n_nodes);
}

/*!
   \brief Get number of primitives in vector map

   \param map vector map
   \patam type feature type

   \return number of primitives
 */
plus_t Vect_get_num_primitives(const struct Map_info *map, int type)
{
    plus_t num = 0;

    if (type & GV_POINT)
	num += map->plus.n_plines;
    if (type & GV_LINE)
	num += map->plus.n_llines;
    if (type & GV_BOUNDARY)
	num += map->plus.n_blines;
    if (type & GV_CENTROID)
	num += map->plus.n_clines;
    if (type & GV_FACE)
	num += map->plus.n_flines;
    if (type & GV_KERNEL)
	num += map->plus.n_klines;

    return num;
}

/*!
   \brief Fetch number of features (points, lines, boundaries, centroids) in vector map

   \param map vector map

   \return number of features
 */
plus_t Vect_get_num_lines(const struct Map_info *map)
{
    return (map->plus.n_lines);
}

/*!
   \brief Get number of areas in vector map

   \param map vector map

   \return number of areas
 */
plus_t Vect_get_num_areas(const struct Map_info *map)
{
    return (map->plus.n_areas);
}

/*!
   \brief Fetch number of kernels in vector map

   \param map vector map

   \return number of kernels
 */
plus_t Vect_get_num_kernels(const struct Map_info *map)
{
    return (map->plus.n_klines);
}


/*!
   \brief Get number of faces in vector map

   \param map vector map

   \return number of faces
 */
plus_t Vect_get_num_faces(const struct Map_info *map)
{
    return (map->plus.n_flines);
}


/*!
   \brief Fetch number of volumes in vector map

   \param map vector map

   \return number of volumes
 */
plus_t Vect_get_num_volumes(const struct Map_info *map)
{
    return (map->plus.n_volumes);
}


/*!
   \brief Get number of islands in vector map

   \param map vector map

   \return number of islands
 */
plus_t Vect_get_num_islands(const struct Map_info *map)
{
    return (map->plus.n_isles);
}


/*!
   \brief Fetch number of holes in vector map

   \param map vector map

   \return number of holes
 */
plus_t Vect_get_num_holes(const struct Map_info *map)
{
    return (map->plus.n_holes);
}


/*!
   \brief Get number of defined dblinks

   \param map vector map

   \return number of dblinks
 */
int Vect_get_num_dblinks(const struct Map_info *map)
{
    return (map->dblnk->n_fields);
}

/*!
   \brief Get number of updated features

   \param map vector map

   \return number of updated features
 */
int Vect_get_num_updated_lines(const struct Map_info *map)
{
    return (map->plus.n_uplines);
}

/*!
   \brief Get updated line by index

   \param map vector map
   \param idx index

   \return updated line
 */
int Vect_get_updated_line(const struct Map_info *map, int idx)
{
    return (map->plus.uplines[idx]);
}

/*!
   \brief Get number of updated nodes

   \param map vector map

   \return number of updated nodes
 */
int Vect_get_num_updated_nodes(const struct Map_info *map)
{
    return (map->plus.n_upnodes);
}

/*!
   \brief Get updated node by index

   \param map vector map
   \param idx index

   \return updated node
 */
int Vect_get_updated_node(const struct Map_info *map, int idx)
{
    return (map->plus.upnodes[idx]);
}

/*!
   \brief Get node coordinates

   \param map vector map
   \param num node id
   \param x,y,z coordinates values (for 2D coordinates z is NULL)

   \return 0
 */
int
Vect_get_node_coor(const struct Map_info *map, int num, double *x, double *y,
		   double *z)
{
    struct P_node *Node;

    Node = map->plus.Node[num];
    *x = Node->x;
    *y = Node->y;

    if (z != NULL)
	*z = Node->z;

    return (0);
}

/*!
   \brief Get line nodes

   \param Map vector map
   \param line line id
   \param n1, n2 ids of line nodes (or NULL)

   \return 1
 */
int Vect_get_line_nodes(const struct Map_info *Map, int line, int *n1, int *n2)
{

    if (Map->level < 2)
	G_fatal_error(_("Vector map <%s> is not open on level >= 2"),
		      Vect_get_full_name(Map));

    if (n1 != NULL)
	*n1 = Map->plus.Line[line]->N1;

    if (n2 != NULL)
	*n2 = Map->plus.Line[line]->N2;

    return 1;
}

/*!
   \brief Get area/isle ids on the left and right

   \param Map vector map
   \param line line id
   \param[out] left,right area/isle id on the left and right

   \return 1
 */
int Vect_get_line_areas(const struct Map_info *Map, int line, int *left, int *right)
{

    if (Map->level < 2)
	G_fatal_error(_("Vector map <%s> is not open on level >= 2"),
		      Vect_get_full_name(Map));

    if (left != NULL)
	*left = Map->plus.Line[line]->left;

    if (right != NULL)
	*right = Map->plus.Line[line]->right;

    return 1;
}

/*!
   \brief Get number of lines for node

   \param Map vector map
   \param node node id

   \return numbers of lines
 */
int Vect_get_node_n_lines(const struct Map_info *Map, int node)
{

    if (Map->level < 2)
	G_fatal_error(_("Vector map <%s> is not open on level >= 2"),
		      Vect_get_full_name(Map));

    return (Map->plus.Node[node]->n_lines);

}

/*!
   \brief Get line id for node line index

   \param Map vector map
   \param node node id
   \param line line index (range: 0 - Vect_get_node_n_lines())

   \return line id
 */
int Vect_get_node_line(const struct Map_info *Map, int node, int line)
{
    if (Map->level < 2)
	G_fatal_error(_("Vector map <%s> is not open on level >= 2"),
		      Vect_get_full_name(Map));

    return (Map->plus.Node[node]->lines[line]);
}

/*!
   \brief Angle of segment of the line connected to the node

   \param Map vector map
   \param node node number
   \param line line index (range: 0 - Vect_get_node_n_lines())

   \return angle of segment of the line connected to the node
 */
float Vect_get_node_line_angle(const struct Map_info *Map, int node, int line)
{
    if (Map->level < 2)
	G_fatal_error(_("Vector map <%s> is not open on level >= 2"),
		      Vect_get_full_name(Map));

    return (Map->plus.Node[node]->angles[line]);
}

/*!
   \brief Get area id the centroid is within

   \param Map vector map
   \param centroid centroid id

   \return area id the centroid is within
   \return 0 for not in area
   \return negative id if area/centroid (?) is duplicate
 */
int Vect_get_centroid_area(const struct Map_info *Map, int centroid)
{
    if (Map->level < 2)
	G_fatal_error(_("Vector map <%s> is not open on level >= 2"),
		      Vect_get_full_name(Map));

    return (Map->plus.Line[centroid]->left);
}
