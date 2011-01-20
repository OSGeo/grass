/*!
   \file diglib/spindex.c

   \brief Vector library - spatial index (lower level functions)

   Lower level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes
   \author Update to GRASS 5.7 Radim Blazek
   \author Update to GRASS 7 Markus Metz
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Initit spatial index (nodes, lines, areas, isles)

   \param Plus pointer to Plus_head structure

   \return 1 OK
   \return 0 on error      
 */
int dig_spidx_init(struct Plus_head *Plus)
{
    int ndims;

    ndims = (Plus->with_z != 0) ? 3 : 2;
    Plus->spidx_with_z = (Plus->with_z != 0);
    
    if (Plus->Spidx_file) {
	int fd;
	char *filename;
	
	filename = G_tempfile();
	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	Plus->Node_spidx = RTreeNewIndex(fd, 0, ndims);
	remove(filename);

	filename = G_tempfile();
	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	Plus->Line_spidx = RTreeNewIndex(fd, 0, ndims);
	remove(filename);

	filename = G_tempfile();
	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	Plus->Area_spidx = RTreeNewIndex(fd, 0, ndims);
	remove(filename);

	filename = G_tempfile();
	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	Plus->Isle_spidx = RTreeNewIndex(fd, 0, ndims);
	remove(filename);

	Plus->Face_spidx = NULL;
	Plus->Volume_spidx = NULL;
	Plus->Hole_spidx = NULL;
    }
    else {
	Plus->Node_spidx = RTreeNewIndex(-1, 0, ndims);
	Plus->Line_spidx = RTreeNewIndex(-1, 0, ndims);
	Plus->Area_spidx = RTreeNewIndex(-1, 0, ndims);
	Plus->Isle_spidx = RTreeNewIndex(-1, 0, ndims);
	Plus->Face_spidx = NULL;
	Plus->Volume_spidx = NULL;
	Plus->Hole_spidx = NULL;
    }

    Plus->Node_spidx_offset = 0L;
    Plus->Line_spidx_offset = 0L;
    Plus->Area_spidx_offset = 0L;
    Plus->Isle_spidx_offset = 0L;
    Plus->Face_spidx_offset = 0L;
    Plus->Volume_spidx_offset = 0L;
    Plus->Hole_spidx_offset = 0L;

    Plus->Spidx_built = 0;
    
    return 1;
}

/*! 
   \brief Free spatial index for nodes

   \param Plus pointer to Plus_head structure
 */
void dig_spidx_free_nodes(struct Plus_head *Plus)
{
    int ndims;

    ndims = Plus->with_z ? 3 : 2;

    /* Node spidx */
    if (Plus->Node_spidx->fd > -1) {
	int fd;
	char *filename;
	
	close(Plus->Node_spidx->fd);
	RTreeFreeIndex(Plus->Node_spidx);
	filename = G_tempfile();
	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	Plus->Node_spidx = RTreeNewIndex(fd, 0, ndims);
	remove(filename);
    }
    else {
	RTreeFreeIndex(Plus->Node_spidx);
	Plus->Node_spidx = RTreeNewIndex(-1, 0, ndims);
    }
}

/*! 
   \brief Free spatial index for lines

   \param Plus pointer to Plus_head structure
 */
void dig_spidx_free_lines(struct Plus_head *Plus)
{
    int ndims;

    ndims = Plus->with_z ? 3 : 2;

    /* Line spidx */
    if (Plus->Line_spidx->fd > -1) {
	int fd;
	char *filename;
	
	close(Plus->Line_spidx->fd);
	RTreeFreeIndex(Plus->Line_spidx);
	filename = G_tempfile();
	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	Plus->Line_spidx = RTreeNewIndex(fd, 0, ndims);
	remove(filename);
    }
    else {
	RTreeFreeIndex(Plus->Line_spidx);
	Plus->Line_spidx = RTreeNewIndex(-1, 0, ndims);
    }
}

/*! 
   \brief Reset spatial index for areas

   \param Plus pointer to Plus_head structure
 */
void dig_spidx_free_areas(struct Plus_head *Plus)
{
    int ndims;

    ndims = Plus->with_z ? 3 : 2;

    /* Area spidx */
    if (Plus->Area_spidx->fd > -1) {
	int fd;
	char *filename;
	
	close(Plus->Area_spidx->fd);
	RTreeFreeIndex(Plus->Area_spidx);
	filename = G_tempfile();
	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	Plus->Area_spidx = RTreeNewIndex(fd, 0, ndims);
	remove(filename);
    }
    else {
	RTreeFreeIndex(Plus->Area_spidx);
	Plus->Area_spidx = RTreeNewIndex(-1, 0, ndims);
    }
}

/*! 
   \brief Reset spatial index for isles

   \param Plus pointer to Plus_head structure
 */
void dig_spidx_free_isles(struct Plus_head *Plus)
{
    int ndims;

    ndims = Plus->with_z ? 3 : 2;

    /* Isle spidx */
    if (Plus->Isle_spidx->fd > -1) {
	int fd;
	char *filename;
	
	close(Plus->Isle_spidx->fd);
	RTreeFreeIndex(Plus->Isle_spidx);
	filename = G_tempfile();
	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	Plus->Isle_spidx = RTreeNewIndex(fd, 0, ndims);
	remove(filename);
    }
    else {
	RTreeFreeIndex(Plus->Isle_spidx);
	Plus->Isle_spidx = RTreeNewIndex(-1, 0, ndims);
    }
}

/*! 
   \brief Free spatial index (nodes, lines, areas, isles)

   \param Plus pointer to Plus_head structure
 */
void dig_spidx_free(struct Plus_head *Plus)
{
    /* Node spidx */
    if (Plus->Node_spidx->fd > -1)
	close(Plus->Node_spidx->fd);
    RTreeFreeIndex(Plus->Node_spidx);

    /* Line spidx */
    if (Plus->Line_spidx->fd > -1)
	close(Plus->Line_spidx->fd);
    RTreeFreeIndex(Plus->Line_spidx);

    /* Area spidx */
    if (Plus->Area_spidx->fd > -1)
	close(Plus->Area_spidx->fd);
    RTreeFreeIndex(Plus->Area_spidx);

    /* Isle spidx */
    if (Plus->Isle_spidx->fd > -1)
	close(Plus->Isle_spidx->fd);
    RTreeFreeIndex(Plus->Isle_spidx);

    /* 3D future : */
    /* Face spidx */
    /* Volume spidx */
    /* Hole spidx */
}

/*!
   \brief Add new node to spatial index 

   \param Plus pointer to Plus_head structure
   \param node node id
   \param x,y,z node coordinates

   \return 1 OK
   \return 0 on error      
 */
int
dig_spidx_add_node(struct Plus_head *Plus, int node,
		   double x, double y, double z)
{
    struct Rect rect;

    G_debug(3, "dig_spidx_add_node(): node = %d, x,y,z = %f, %f, %f", node, x,
	    y, z);

    rect.boundary[0] = x;
    rect.boundary[1] = y;
    rect.boundary[2] = z;
    rect.boundary[3] = x;
    rect.boundary[4] = y;
    rect.boundary[5] = z;
    RTreeInsertRect(&rect, node, Plus->Node_spidx);

    return 1;
}

/*!
   \brief Add new line to spatial index 

   \param Plus pointer to Plus_head structure
   \param line line id
   \param box bounding box

   \return 0
 */
int dig_spidx_add_line(struct Plus_head *Plus, int line, struct bound_box * box)
{
    struct Rect rect;

    G_debug(3, "dig_spidx_add_line(): line = %d", line);

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;
    RTreeInsertRect(&rect, line, Plus->Line_spidx);

    return 0;
}

/*!
   \brief Add new area to spatial index 

   \param Plus pointer to Plus_head structure
   \param area area id
   \param box bounding box

   \return 0
 */
int dig_spidx_add_area(struct Plus_head *Plus, int area, struct bound_box * box)
{
    struct Rect rect;

    G_debug(3, "dig_spidx_add_area(): area = %d", area);

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;
    RTreeInsertRect(&rect, area, Plus->Area_spidx);

    return 0;
}

/*!
   \brief Add new island to spatial index 

   \param Plus pointer to Plus_head structure
   \param isle isle id
   \param box bounding box

   \return 0
 */

int dig_spidx_add_isle(struct Plus_head *Plus, int isle, struct bound_box * box)
{
    struct Rect rect;

    G_debug(3, "dig_spidx_add_isle(): isle = %d", isle);

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;
    RTreeInsertRect(&rect, isle, Plus->Isle_spidx);

    return 0;
}

/*!
   \brief Delete node from spatial index 

   G_fatal_error() called on error.

   \param Plus pointer to Plus_head structure
   \param node node id

   \return 0
 */
int dig_spidx_del_node(struct Plus_head *Plus, int node)
{
    int ret;
    struct P_node *Node;
    struct Rect rect;

    G_debug(3, "dig_spidx_del_node(): node = %d", node);

    Node = Plus->Node[node];

    rect.boundary[0] = Node->x;
    rect.boundary[1] = Node->y;
    rect.boundary[2] = Node->z;
    rect.boundary[3] = Node->x;
    rect.boundary[4] = Node->y;
    rect.boundary[5] = Node->z;

    ret = RTreeDeleteRect(&rect, node, Plus->Node_spidx);

    if (ret)
	G_fatal_error(_("Unable to delete node %d from spatial index"), node);

    return 0;
}

/*!
   \brief Delete line from spatial index 

   G_fatal_error() called on error.

   \param Plus pointer to Plus_head structure
   \param line line id

   \return 0
 */
int dig_spidx_del_line(struct Plus_head *Plus, int line)
{
    struct P_line *Line;
    struct Rect rect;
    int ret;

    G_debug(3, "dig_spidx_del_line(): line = %d", line);

    Line = Plus->Line[line];

    G_debug(3, "  box(x1,y1,z1,x2,y2,z2): %f %f %f %f %f %f", Line->W,
	    Line->S, Line->B, Line->E, Line->N, Line->T);

    rect.boundary[0] = Line->W;
    rect.boundary[1] = Line->S;
    rect.boundary[2] = Line->B;
    rect.boundary[3] = Line->E;
    rect.boundary[4] = Line->N;
    rect.boundary[5] = Line->T;

    ret = RTreeDeleteRect(&rect, line, Plus->Line_spidx);

    G_debug(3, "  ret = %d", ret);

    if (ret)
	G_fatal_error(_("Unable to delete line %d from spatial index"), line);

    return 0;
}

/*!
   \brief Delete area from spatial index 

   G_fatal_error() called on error.

   \param Plus pointer to Plus_head structure
   \param area area id

   \return 0
 */
int dig_spidx_del_area(struct Plus_head *Plus, int area)
{
    int ret;
    struct P_area *Area;
    struct Rect rect;

    G_debug(3, "dig_spidx_del_area(): area = %d", area);

    Area = Plus->Area[area];

    if (Area == NULL) {
	G_fatal_error(_("Attempt to delete sidx for dead area"));
    }

    rect.boundary[0] = Area->W;
    rect.boundary[1] = Area->S;
    rect.boundary[2] = Area->B;
    rect.boundary[3] = Area->E;
    rect.boundary[4] = Area->N;
    rect.boundary[5] = Area->T;

    ret = RTreeDeleteRect(&rect, area, Plus->Area_spidx);

    if (ret)
	G_fatal_error(_("Unable to delete area %d from spatial index"), area);

    return 0;
}

/*! 
   \brief Delete isle from spatial index 

   G_fatal_error() called on error.

   \param Plus pointer to Plus_head structure
   \param isle isle id

   \return 0
 */
int dig_spidx_del_isle(struct Plus_head *Plus, int isle)
{
    int ret;
    struct P_isle *Isle;
    struct Rect rect;

    G_debug(3, "dig_spidx_del_isle(): isle = %d", isle);

    Isle = Plus->Isle[isle];

    rect.boundary[0] = Isle->W;
    rect.boundary[1] = Isle->S;
    rect.boundary[2] = Isle->B;
    rect.boundary[3] = Isle->E;
    rect.boundary[4] = Isle->N;
    rect.boundary[5] = Isle->T;

    ret = RTreeDeleteRect(&rect, isle, Plus->Isle_spidx);

    if (ret)
	G_fatal_error(_("Unable to delete isle %d from spatial index"), isle);

    return 0;
}

/* This function is called by  RTreeSearch() to add selected node/line/area/isle to thelist */
static int _add_item(int id, struct ilist *list)
{
    dig_list_add(list, id);
    return 1;
}

/*!
   \brief Select nodes by bbox 

   \param Plus pointer to Plus_head structure
   \param box bounding box
   \param list list of selected lines

   \return number of selected nodes
   \return -1 on error
 */
int
dig_select_nodes(struct Plus_head *Plus, const struct bound_box * box,
		 struct ilist *list)
{
    struct Rect rect;

    G_debug(3, "dig_select_nodes()");

    list->n_values = 0;

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;

    if (Plus->Spidx_new)
	RTreeSearch(Plus->Node_spidx, &rect, (void *)_add_item, list);
    else
	rtree_search(Plus->Node_spidx, &rect, (void *)_add_item, list, Plus);

    return (list->n_values);
}

/* This function is called by  RTreeSearch() for nodes to add selected node to list */
static int _add_node(int id, int *node)
{
    *node = id;
    return 0;
}

/*!
   \brief Find one node by coordinates 

   \param Plus pointer to Plus_head structure
   \param x,y,z coordinates

   \return number of node
   \return 0 not found
 */
int dig_find_node(struct Plus_head *Plus, double x, double y, double z)
{
    struct Rect rect;
    struct ilist list;
    int node;

    G_debug(3, "dig_find_node()");

    dig_init_list(&list);

    rect.boundary[0] = x;
    rect.boundary[1] = y;
    rect.boundary[2] = z;
    rect.boundary[3] = x;
    rect.boundary[4] = y;
    rect.boundary[5] = z;

    node = 0;
    if (Plus->Spidx_new)
	RTreeSearch(Plus->Node_spidx, &rect, (void *)_add_node, &node);
    else
	rtree_search(Plus->Node_spidx, &rect, (void *)_add_node, &node, Plus);

    return node;
}

/*!
   \brief Select lines by box 

   \param Plus pointer to Plus_head structure
   \param box bounding box
   \param list list of selected lines

   \return number of selected lines
 */
int
dig_select_lines(struct Plus_head *Plus, const struct bound_box * box,
		 struct ilist *list)
{
    struct Rect rect;

    G_debug(3, "dig_select_lines()");

    list->n_values = 0;

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;

    if (Plus->Spidx_new)
	RTreeSearch(Plus->Line_spidx, &rect, (void *)_add_item, list);
    else
	rtree_search(Plus->Line_spidx, &rect, (void *)_add_item, list, Plus);

    return (list->n_values);
}

/*! 
   \brief Select areas by box 

   \param Plus pointer to Plus_head structure
   \param box bounding box
   \param list list of selected lines

   \return number of selected areas
 */
int
dig_select_areas(struct Plus_head *Plus, const struct bound_box * box,
		 struct ilist *list)
{
    struct Rect rect;

    G_debug(3, "dig_select_areas()");

    list->n_values = 0;

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;

    if (Plus->Spidx_new)
	RTreeSearch(Plus->Area_spidx, &rect, (void *)_add_item, list);
    else
	rtree_search(Plus->Area_spidx, &rect, (void *)_add_item, list, Plus);

    return (list->n_values);
}

/*!
   \brief Select isles by box 

   \param Plus pointer to Plus_head structure
   \param box bounding box
   \param list list of selected lines

   \return number of selected isles
 */
int
dig_select_isles(struct Plus_head *Plus, const struct bound_box * box,
		 struct ilist *list)
{
    struct Rect rect;

    G_debug(3, "dig_select_isles()");

    list->n_values = 0;

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;

    if (Plus->Spidx_new)
	RTreeSearch(Plus->Isle_spidx, &rect, (void *)_add_item, list);
    else
	rtree_search(Plus->Isle_spidx, &rect, (void *)_add_item, list, Plus);

    return (list->n_values);
}
