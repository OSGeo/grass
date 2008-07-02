/*
****************************************************************************
*
* MODULE:       Vector library 
*   	    	
* AUTHOR(S):    Dave Gerdes, CERL.
*               Update to GRASS 5.7 Radim Blazek.
*
* PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#include <stdlib.h>
#include <grass/Vect.h>

/*  These routines all eventually call calloc() to allocate and zero
   **  the new space.  BUT It is not neccessarily safe to assume that
   **  the memory will be zero.  The next memory location asked for could
   **  have been previously used and not zeroed.  (e.g. compress())
 */

/* alloc_node (map, add)
** alloc_line (map, add)
** alloc_area (map, add)
** alloc_points (map, num)
** node_alloc_line (node, add)
** area_alloc_line (node, add)
**
** Allocate array space to add 'add' elements
*/


/* allocate new node structure */
P_NODE *
dig_alloc_node ( ){
    P_NODE *Node;

    Node = (P_NODE *) malloc ( sizeof(P_NODE) );
    if (Node == NULL) return NULL;

    Node->n_lines = 0;
    Node->alloc_lines = 0;
    Node->lines = NULL;
    Node->angles = NULL;
  
    return (Node);
}

/* dig_node_alloc_line (node, add)
**     allocate space in  P_node,  lines and angles arrays to add 'add' more
**     lines
**
**  Returns   0 ok    or    -1 on error
*/

int 
dig_node_alloc_line ( P_NODE * node, int add) {
  int  num;
  char *p;

  G_debug (3, "dig_node_alloc_line(): add = %d", add); 
  
  num = node->n_lines + add;

  p = realloc ( node->lines, num * sizeof(plus_t) );
  if ( p == NULL ) return -1; 
  node->lines = (plus_t *) p;
  
  p = realloc ( node->angles, num * sizeof(float) );
  if ( p == NULL ) return -1;
  node->angles = (float *) p;
  
  node->alloc_lines = num;
  return (0);
}


/* Reallocate array of pointers to nodes.
*  Space for 'add' number of nodes is added.
* 
*  Returns: 0 success
*          -1 error
*/
int 
dig_alloc_nodes ( struct Plus_head *Plus, int add) {
    int  size;
    char *p;

    size = Plus->alloc_nodes + 1 + add;
    p = realloc ( Plus->Node, size * sizeof(P_NODE *) );
    if ( p == NULL ) return -1;
	 
    Plus->Node = (P_NODE **) p;
    Plus->alloc_nodes = size - 1;

    return (0);
}

/* allocate new line structure */
P_LINE *
dig_alloc_line ( ){
    P_LINE *Line;

    Line = (P_LINE *) malloc ( sizeof(P_LINE) );
    if (Line == NULL) return NULL;

    return (Line);
}

/* Reallocate array of pointers to lines.
*  Space for 'add' number of lines is added.
* 
*  Returns: 0 success
*          -1 error
*/
int 
dig_alloc_lines ( struct Plus_head *Plus, int add) {
    int  size;
    char *p;

    size = Plus->alloc_lines + 1 + add;
    p = realloc ( Plus->Line, size * sizeof(P_LINE *) );
    if ( p == NULL ) return -1;
	 
    Plus->Line = (P_LINE **) p;
    Plus->alloc_lines = size - 1;

    return (0);
}

/* Reallocate array of pointers to areas.
*  Space for 'add' number of areas is added.
* 
*  Returns: 0 success
*          -1 error
*/
int 
dig_alloc_areas ( struct Plus_head *Plus, int add) {
    int  size;
    char *p;

    size = Plus->alloc_areas + 1 + add;
    p = realloc ( Plus->Area, size * sizeof(P_AREA *) );
    if ( p == NULL ) return -1;
	 
    Plus->Area = (P_AREA **) p;
    Plus->alloc_areas = size - 1;

    return (0);
}

/* Reallocate array of pointers to isles.
*  Space for 'add' number of isles is added.
* 
*  Returns: 0 success
*          -1 error
*/
int 
dig_alloc_isles ( struct Plus_head *Plus, int add) {
    int  size;
    char *p;

    G_debug (3, "dig_alloc_isle():");
    size = Plus->alloc_isles + 1 + add;
    p = realloc ( Plus->Isle, size * sizeof(P_ISLE *) );
    if ( p == NULL ) return -1;
	 
    Plus->Isle = (P_ISLE **) p;
    Plus->alloc_isles = size - 1;

    return (0);
}

/* allocate new area structure */
P_AREA *
dig_alloc_area (){
    P_AREA *Area;

    Area = (P_AREA *) malloc ( sizeof(P_AREA) );
    if (Area == NULL) return NULL;

    Area->n_lines = 0;
    Area->alloc_lines = 0;
    Area->lines = NULL;

    Area->alloc_isles = 0;
    Area->n_isles = 0;
    Area->isles = NULL;
    
    Area->centroid = 0;

    return (Area);
}

/* alloc new isle structure */
P_ISLE * 
dig_alloc_isle ()
{
    P_ISLE *Isle;

    Isle = (P_ISLE *) malloc ( sizeof(P_ISLE) );
    if (Isle == NULL) return NULL;

    Isle->n_lines = 0;
    Isle->alloc_lines = 0;
    Isle->lines = NULL;
    
    Isle->area = 0;

    return (Isle);
}


/* allocate room for  'num'   X and Y  arrays in struct line_pnts 
   **   returns -1 on out of memory 
 */
int 
dig_alloc_points (
		   struct line_pnts *points,
		   int num)
{
  int alloced;
  char *p;

  alloced = points->alloc_points;
  /* alloc_space will just return if no space is needed */
  if (!(p =
	dig__alloc_space (num, &alloced, 50, (char *) points->x,
			  sizeof (double))))
    {
      return (dig_out_of_memory ());
    }
  points->x = (double *) p;

  alloced = points->alloc_points;
  /* alloc_space will just return if no space is needed */
  if (!(p =
	dig__alloc_space (num, &alloced, 50, (char *) points->y,
			  sizeof (double))))
    {
      return (dig_out_of_memory ());
    }
  points->y = (double *) p;

  alloced = points->alloc_points;
  /* alloc_space will just return if no space is needed */
  if (!(p =
	dig__alloc_space (num, &alloced, 50, (char *) points->z,
			  sizeof (double))))
    {
      return (dig_out_of_memory ());
    }
  points->z = (double *) p;

  points->alloc_points = alloced;
  return (0);
}

/* allocate room for  'num'  fields and category arrays 
   ** in struct line_cats 
   **   returns -1 on out of memory 
 */
int 
dig_alloc_cats (
		 struct line_cats *cats,
		 int num)
{
  int alloced;
  char *p;

  /* alloc_space will just return if no space is needed */
  alloced = cats->alloc_cats;
  if (!(p =
	dig__alloc_space (num, &alloced, 1, (int *) cats->field,
			  sizeof (int))))
    {
      return (dig_out_of_memory ());
    }
  cats->field = (int *) p;

  alloced = cats->alloc_cats;
  if (!(p =
	dig__alloc_space (num, &alloced, 1, (int *) cats->cat,
			  sizeof (int))))
    {
      return (dig_out_of_memory ());
    }
  cats->cat = (int *) p;

  cats->alloc_cats = alloced;
  return (0);
}

/* area_alloc_line (area, add)
**     allocate space in  P_area for add new lines
**
**  Returns   0 ok    or    -1 on error
*/
int 
dig_area_alloc_line ( P_AREA * area, int add) {
  int num;
  char *p;

  num = area->alloc_lines + add;

  p = realloc ( area->lines, num * sizeof(plus_t) );
  if ( p == NULL ) return -1; 
  area->lines = (plus_t *) p;
 
  area->alloc_lines = num;
  
  return (0);
}

/* area_alloc_isle (area, add)
**     allocate space in  P_area for add new isles
**
**  Returns   0 ok    or    -1 on error
*/
int 
dig_area_alloc_isle ( P_AREA * area, int add) {
  int num;
  char *p;

  G_debug (5, "dig_area_alloc_isle(): add = %d", add ); 
  num = area->alloc_isles + add;

  p = realloc ( area->isles, num * sizeof(plus_t) );
  if ( p == NULL ) return -1; 
  area->isles = (plus_t *) p;

  area->alloc_isles = num;
  return (0);
}

/* dig_isle_alloc_line (isle, add)
   **     allocate space in  P_isle for add new lines
   **
   **  Returns   0 ok    or    -1 on error
 */

int 
dig_isle_alloc_line (
		      P_ISLE * isle,
		      int add)
{
  int num;
  char *p;

  G_debug (3, "dig_isle_alloc_line():");
  num = isle->alloc_lines + add;

  p = realloc ( isle->lines, num * sizeof(plus_t) );
  if ( p == NULL ) return -1; 
  isle->lines = (plus_t *) p;
 
  isle->alloc_lines = num;
  
  return (0);
}



/* for now just print message and return error code */
int 
dig_out_of_memory ()
{
  fprintf (stderr, "OUT OF MEMORY!\n");
  return (-1);
}

