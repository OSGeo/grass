/*!
  \file write.c
  
  \brief Vector library - write data
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Radim Blazek
  
  \date 2001
*/

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/Vect.h>

static long write_dummy () { 
    G_warning ("Vect_write_line() %s", _("for this format/level not supported") );
    return -1; 
}
static int rewrite_dummy () { 
    G_warning ("Vect_rewrite_line() %s",  _("for this format/level not supported") );
    return -1; 
}
static int  delete_dummy () { 
    G_warning ("Vect_delete_line() %s",  _("for this format/level not supported") );
    return -1; 
}

#ifndef HAVE_OGR
static int format () { G_fatal_error ( _("Requested format is not compiled in this version") ); return 0; }
#endif

static long (*Write_line_array[][3]) () =
{
    { write_dummy, V1_write_line_nat, V2_write_line_nat } 
#ifdef HAVE_OGR
   ,{ write_dummy, write_dummy, write_dummy }
#else
   ,{ write_dummy, format, format }
#endif
};

static int (*Vect_rewrite_line_array[][3]) () =
{
    { rewrite_dummy, rewrite_dummy, V2_rewrite_line_nat } 
#ifdef HAVE_OGR
   ,{ rewrite_dummy, rewrite_dummy, rewrite_dummy }
#else
   ,{ rewrite_dummy, format, format }
#endif
};

static int (*Vect_delete_line_array[][3]) () =
{
    { delete_dummy, delete_dummy, V2_delete_line_nat } 
#ifdef HAVE_OGR
   ,{ delete_dummy, delete_dummy, delete_dummy } 
#else
   ,{ delete_dummy, format, format }
#endif
};

/*!
  \brief Writes new line to the end of file (table)

  The function calls G_fatal_error() on error.

  \param Map pointer to vector map
  \param type vector type
  \param points line geometry
  \param cats line categories
  
  \return offset into file where the line starts
*/
long
Vect_write_line (
     struct Map_info *Map,
     int type,
     struct line_pnts *points,
     struct line_cats *cats)
{
    long offset;
    
    G_debug (3, "Vect_write_line(): name = %s, format = %d, level = %d", 
	           Map->name, Map->format, Map->level);

    if (!VECT_OPEN (Map))
	G_fatal_error ( _("Unable to write line, vector map is not opened") );

    dig_line_reset_updated ( &(Map->plus) );
    dig_node_reset_updated ( &(Map->plus) );
    if ( !(Map->plus.update_cidx) ) {
        Map->plus.cidx_up_to_date = 0;
    }

    offset = (*Write_line_array[Map->format][Map->level]) (Map, type, points, cats);

    if ( offset == -1 )
	G_fatal_error ( _("Unable to write line (negative offset)") );

    return offset;
}


/*!
  \brief Rewrites line info at the given offset.

  The number of points or cats or type may change. If necessary, the
  old line is deleted and new is written.

  This function calls G_fatal_error() on error.

  \param Map pointer to vector map
  \param line line id
  \param type vector type
  \param points line geometry
  \param cats line categories
  
  \return number of new line
  \return -1 on error
*/
int
Vect_rewrite_line (
     struct Map_info *Map,
     int line,
     int type,
     struct line_pnts *points,
     struct line_cats *cats)
{
    long ret;
    
    G_debug (3, "Vect_rewrite_line(): name = %s, line = %d", Map->name, line);
    
    if (!VECT_OPEN (Map))
	G_fatal_error ( _("Unable to rewrite line, vector map is not opened") );
    
    dig_line_reset_updated ( &(Map->plus) );
    dig_node_reset_updated ( &(Map->plus) );
    if ( !(Map->plus.update_cidx) ) {
        Map->plus.cidx_up_to_date = 0;
    }

    ret = (*Vect_rewrite_line_array[Map->format][Map->level]) (Map, line, type, points, cats);

    if ( ret == -1 )
	G_fatal_error ( _("Unable to rewrite line %d"), line);

    return ret;
}

/*
*  Deletes line at the given offset. Map must be opened on level 2.
*  
*  Returns: 0 ok
*          -1 on error 
*/
/*
int
V1_delete_line (
     struct Map_info *Map,
     long offset)
{
#ifdef GDEBUG
    G_debug (3, "V1_delete_line(): name = %s", Map->name);
#endif
    return (*V1_delete_line_array[Map->format][Map->level]) (Map, offset);
}
*/

/*!
  \brief Deletes line of given number.

  Vector map must be opened on topo level 2.

  This function calls G_fatal_error() on error.

  \param Map pointer to vector map
  \param line line id

  \return 0 on success
  \return -1 on error
*/
int
Vect_delete_line (
    struct Map_info *Map,
    int line)
{
    int ret;
    
    G_debug (3, "Vect_delete_line(): name = %s, line = %d", Map->name, line);
    
    if ( Map->level < 2 ) {
	G_fatal_error ( _("Unable to delete line %d, "
			  "vector map <%s> is not opened on topo level 2"), line, Map->name );
    }
    
    if ( Map->mode != GV_MODE_RW && Map->mode != GV_MODE_WRITE ) {
	G_fatal_error ( _("Unable to delete line %d, "
			  "vector map <%s> is not opened in 'write' mode"), line, Map->name );
    }
    
    dig_line_reset_updated ( &(Map->plus) );
    dig_node_reset_updated ( &(Map->plus) );
    if ( !(Map->plus.update_cidx) ) {
        Map->plus.cidx_up_to_date = 0;
    }
    
    ret = (*Vect_delete_line_array[Map->format][Map->level]) (Map, line);

    if ( ret == -1 )
	G_fatal_error ( _("Unable to delete line %d from vector map <%s>"), line, Map->name );

    return ret;
}
