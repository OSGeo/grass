/*!
  \file read.c
  
  \brief Vector library - reading data
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  Update to GRASS 5.7 Radim Blazek and David D. Gray.
  
  \date 2001
*/

#include <grass/Vect.h>
#include <grass/glocale.h>

static int read_next_dummy () { return -1; }

#ifndef HAVE_OGR
static int format () { G_fatal_error (_("Requested format is not compiled in this version")x); return 0; }
#endif

static int (*Read_next_line_array[][3]) () =
{
    { read_next_dummy, V1_read_next_line_nat, V2_read_next_line_nat }
#ifdef HAVE_OGR
   ,{ read_next_dummy, V1_read_next_line_ogr, V2_read_next_line_ogr }
#else
   ,{ read_next_dummy, format, format }
#endif
};

static int (*V2_read_line_array[]) () =
{
   V2_read_line_nat 
#ifdef HAVE_OGR
   , V2_read_line_ogr
#else
   , format
#endif
};

/*!
  \brief Get next vector line

  \param Map vector map
  \param[out] line_p line geometry
  \param[out] line_c line categories

  \return line type,
  \return -1 on Out of memory,
  \return -2 on EOF   

*/
int
Vect_read_next_line (
    struct Map_info *Map,
    struct line_pnts *line_p,
    struct line_cats *line_c)
{

    G_debug (3, "Vect_read_next_line()");
  
    if (!VECT_OPEN (Map))
        return -1;

    return (*Read_next_line_array[Map->format][Map->level]) (Map, line_p, line_c);
}

/*!
  \brief Get vector line

  \param Map vector map
  \param[out] line_p line geometry
  \param[out] line_c line categories
  \param line line id 
  \return line type,
  \return -1 on Out of memory,
  \return -2 on EOF   
*/
int
Vect_read_line (
                struct Map_info *Map,
                struct line_pnts *line_p,
                struct line_cats *line_c,
                int    line)
{

    G_debug (3, "Vect_read_line()");
  
    if (!VECT_OPEN (Map))
	G_fatal_error ("Vect_read_line(): %s", _("vector map is not opened"));

    if (line < 1 || line > Map->plus.n_lines)
        G_fatal_error (_("Vect_read_line(): line %d is not reasonable (max line in vector map: %d)"),
		       line, Map->plus.n_lines );
    
    return (*V2_read_line_array[Map->format]) (Map, line_p, line_c, line);
}

/*!
  \brief Check if line is alive or dead

  \param Map vector map
  \param line line id

  \return 1 if line alive
  \return 0 if line is dead
*/
int
Vect_line_alive ( struct Map_info *Map, int line )
{
    if ( Map->plus.Line[line] != NULL ) return 1;
    
    return 0;
}

/*!
  \brief Check if node is alive or dead

  \param Map vector map
  \param node node id

  \return 1 if node alive
  \return 0 if node is dead
*/
int
Vect_node_alive ( struct Map_info *Map, int node )
{
    if ( Map->plus.Node[node] != NULL ) return 1;
    
    return 0;
}

/*!
  \brief Check if area is alive or dead

  \param Map vector map
  \param area area id

  \return 1 if area alive
  \return 0 if area is dead
*/
int
Vect_area_alive ( struct Map_info *Map, int area )
{
    if ( Map->plus.Area[area] != NULL ) return 1;
    
    return 0;
}

/*!
  \brief Check if isle is alive or dead

  \param Map vector map
  \param isle isle id

  \return 1 if isle alive
  \return 0 if isle is dead
*/
int
Vect_isle_alive ( struct Map_info *Map, int isle )
{
    if ( Map->plus.Isle[isle] != NULL ) return 1;
    
    return 0;
}
