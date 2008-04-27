/*!
  \file level_two.c
  
  \brief Vector library - topo level 
  
  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.

  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  Update to GRASS 5.7 Radim Blazek and David D. Gray.
  
  \date 2001-2008
*/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

/*!
  \brief Get number of nodes

  \param Map vector map

  \return number of nodes
 */
int 
Vect_get_num_nodes (struct Map_info *map)
{
  return (map->plus.n_nodes);
}

/*!
  \brief Get number of primitives

  \param map vector map
  \patam type feature type

  \return number of primitives of given type 
 */
int 
Vect_get_num_primitives (struct Map_info *map, int type)
{
    int num = 0;
    
    if ( type & GV_POINT ) num += map->plus.n_plines;
    if ( type & GV_LINE ) num += map->plus.n_llines;
    if ( type & GV_BOUNDARY ) num += map->plus.n_blines;
    if ( type & GV_CENTROID ) num += map->plus.n_clines;
    if ( type & GV_FACE ) num += map->plus.n_flines;
    if ( type & GV_KERNEL ) num += map->plus.n_klines;
  
    return num;
}

/*!
  \brief Fet number of line vectors (points, lines, centroids)

  \param map vector map

  \return number of line vectors
 */
int 
Vect_get_num_lines (struct Map_info *map)
{
  return (map->plus.n_lines);
}

/*!
  \brief Get number of areas

  \param map vector map

  \return number of areas
 */
int 
Vect_get_num_areas (struct Map_info *map)
{
  return (map->plus.n_areas);
}

/*!
  \brief Get number of faces

  \param map vector map
  
  \return number of faces
*/
int
Vect_get_num_faces (struct Map_info *map)
{
	  return (map->plus.n_flines);
}

/*!
  \brief Get number of islands

  \param map vector map

  \return number of islands
 */
int 
Vect_get_num_islands (struct Map_info *map)
{
  return (map->plus.n_isles);
}

/*!
  \brief Get number of defined dblinks

  \param map vector map

  \return number of dblinks
 */
int 
Vect_get_num_dblinks (struct Map_info *map)
{
  return (map->dblnk->n_fields);
}

/*!
  \brief Get number of updated lines

  \param map vector map

  \return number of updated lines
 */
int 
Vect_get_num_updated_lines (struct Map_info *map)
{
  return (map->plus.n_uplines);
}

/*!
  \brief Get updated line by index

  \param map vector map
  \param idx index

  \return updated line
 */
int 
Vect_get_updated_line (struct Map_info *map, int idx)
{
  return (map->plus.uplines[idx]);
}

/*!
  \brief Get number of updated nodes

  \param map vector map

  \return number of updated nodes
 */
int 
Vect_get_num_updated_nodes (struct Map_info *map)
{
  return (map->plus.n_upnodes);
}

/*!
  \brief Get updated node by index

  \param map vector map
  \param idx index

  \return updated node
 */
int 
Vect_get_updated_node (struct Map_info *map, int idx)
{
  return (map->plus.upnodes[idx]);
}

/*!
  \brief Get 2D/3D coordinates of node

  \param map vector map
  \param num 
  \param x,y,z coordinates values

  \return 2D/3D coordinates of node
 */
int 
Vect_get_node_coor (struct Map_info *map, int num, double *x, double *y, double *z)
{
    P_NODE *Node;

    Node = map->plus.Node[num];
    *x = Node->x;
    *y = Node->y;

    if ( z != NULL )
        *z = Node->z;
  
    return (0);
}

/*!
  \brief Get starting and ending node of line
  
  \param Map vector map
  \param line line id
  \param n1, n2 ids of line nodes

  \return numbers of line nodes
*/
int 
Vect_get_line_nodes ( struct Map_info *Map, int line, int *n1, int *n2)
{

    if ( Map->level < 2 )
	G_fatal_error (_("Vector map <%s> is not open on level >= 2"), Vect_get_full_name(Map));
    
    if ( n1 != NULL ) 
	*n1 = Map->plus.Line[line]->N1;

    if ( n2 != NULL ) 
	*n2 = Map->plus.Line[line]->N2;

    return 1;
}

/*!
  \brief Get areas/isles on the left and right

  \param Map vector map
  \param line
  \param[out] left,right numbers of areas/isles on the left and right

  \return numbers of areas/isles on the left and right
*/
int 
Vect_get_line_areas ( struct Map_info *Map, int line, int *left, int *right)
{

    if ( Map->level < 2 )
	G_fatal_error (_("Vector map <%s> is not open on level >= 2"), Vect_get_full_name(Map));
    
    if ( left != NULL ) 
	*left = Map->plus.Line[line]->left;

    if ( right != NULL ) 
	*right = Map->plus.Line[line]->right;

    return 1;
}

/*!
  \brief Returns number of lines for node
  
  \param Map vector map
  \param node node id

  \return numbers of lines for a node
*/
int 
Vect_get_node_n_lines ( struct Map_info *Map, int node )
{

    if ( Map->level < 2 )
	G_fatal_error (_("Vector map <%s> is not open on level >= 2"), Vect_get_full_name(Map));
    
    return ( Map->plus.Node[node]->n_lines );

}

/*!
  \brief Returns line number for node line index
  
  \param Map vector map
  \param node node number
  \param line line index, range : 0 - Vect_get_node_n_lines()

  \return line number for node line index 
*/
int 
Vect_get_node_line ( struct Map_info *Map, int node, int line )
{
    if ( Map->level < 2 )
	G_fatal_error (_("Vector map <%s> is not open on level >= 2"), Vect_get_full_name(Map));
    
    return ( Map->plus.Node[node]->lines[line] );
}

/*!
  \brief Angle of segment of the line connected to the node

  \param Map vector map
  \param node node number
  \param line line index, range : 0 - Vect_get_node_n_lines()

  \return angle of segment of the line connected to the node
*/
float 
Vect_get_node_line_angle ( struct Map_info *Map, int node, int line )
{
    if ( Map->level < 2 )
	G_fatal_error (_("Vector map <%s> is not open on level >= 2"), Vect_get_full_name(Map));
    
    return ( Map->plus.Node[node]->angles[line] );
}

/*!
  \brief Returns ID of area the centroid is within

  \param Map vector map
  \param centroid centroid id

  \return ID of area the centroid is within
  \return 0 for not in area
  \return negative ID if area/centroid (?) is duplicate
*/
int 
Vect_get_centroid_area ( struct Map_info *Map, int centroid )
{
    if ( Map->level < 2 )
	G_fatal_error (_("Vector map <%s> is not open on level >= 2"), Vect_get_full_name(Map));
    
    return ( Map->plus.Line[centroid]->left );
}
