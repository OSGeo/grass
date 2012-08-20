/*!
   \file lib/vector/Vlib/read.c

   \brief Vector library - read features

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Update to GRASS 7 Martin Landa <landa.martin gmail.com>
 */

#include <sys/types.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static int read_dummy()
{
    G_warning("Vect_read_line() %s",
	      _("for this format/level not supported"));
    return -1;
}

#if !defined HAVE_OGR || !defined HAVE_POSTGRES
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}
#endif

static int (*Read_next_line_array[][3]) () = {
    {
	read_dummy, V1_read_next_line_nat, V2_read_next_line_nat}
#ifdef HAVE_OGR
    , {
        read_dummy, V1_read_next_line_ogr, V2_read_next_line_ogr}
    , {
	read_dummy, V1_read_next_line_ogr, V2_read_next_line_ogr}
#else
    , {
	read_dummy, format, format}
    , {
	read_dummy, format, format}
#endif
#ifdef HAVE_POSTGRES
    , {
	read_dummy, V1_read_next_line_pg, V2_read_next_line_pg}
#else
    , {
	read_dummy, format, format, format}
#endif
};

static int (*Read_line_array[]) () = {
    V2_read_line_nat
#ifdef HAVE_OGR
    , V2_read_line_sfa
    , V2_read_line_sfa
#else
    , format
    , format
#endif
#ifdef HAVE_POSTGRES
    , V2_read_line_pg
#else
    , format
#endif
};

    
/*!
  \brief Get line id for sequential reading.

  This function returns id of feature which has been read by calling
  Vect_read_next_line().

  \param Map pointer to Map_info struct

  \return feature id
*/
int Vect_get_next_line_id(const struct Map_info *Map)
{
    G_debug(3, "Vect_get_next_line()");

    if (!VECT_OPEN(Map))
	G_fatal_error(_("Vector map is not open for reading"));
    
    return Map->next_line - 1;
}

/*!
   \brief Read next vector feature

   This function implements sequential access, constraints are
   reflected, see Vect_set_constraint_region(),
   Vect_set_constraint_type(), or Vect_set_constraint_field().
     
   Use Vect_rewind() to reset reading.

   G_fatal_error() is called on failure.
   
   \param Map pointer Map_info struct
   \param[out] line_p feature geometry
   (pointer to line_pnts struct)
   \param[out] line_c feature categories
   (pointer to line_cats struct)

   \return feature type (GV_POINT, GV_LINE, ...)
   \return -1 on error
   \return -2 nothing to read
 */
int Vect_read_next_line(const struct Map_info *Map,
			struct line_pnts *line_p, struct line_cats *line_c)
{
    int ret;
    
    G_debug(3, "Vect_read_next_line(): next_line = %d", Map->next_line);

    if (!VECT_OPEN(Map))
	G_fatal_error(_("Vector map is not open for reading"));
    
    ret = (*Read_next_line_array[Map->format][Map->level]) (Map, line_p,
							    line_c);
    /* 
    if (ret == -1)
	G_fatal_error(_("Unable to read feature %d vector map <%s>"),
		      Map->next_line, Vect_get_full_name(Map));
    */
    return ret;
}

/*!
   \brief Read vector feature

   This function implements random access. Note that constraits are
   ignored!

   G_fatal_error() is called on failure.
   
   \param Map pointer to vector map
   \param[out] line_p feature geometry
   (pointer to line_pnts struct)
   \param[out] line_c feature categories
   (pointer to line_cats struct)
   \param line feature id (starts at 1)
   
   \return feature type
   \return -1 on failure
   \return -2 nothing to read   
 */
int Vect_read_line(const struct Map_info *Map,
		   struct line_pnts *line_p, struct line_cats *line_c, int line)
{
    int ret;
    
    G_debug(3, "Vect_read_line(): line = %d", line);

    if (!VECT_OPEN(Map))
	G_fatal_error(_("Vector map is not open for reading"));

    if (line < 1 || line > Map->plus.n_lines)
	G_fatal_error(_("Requested feature id %d is not reasonable"
			"(max features in vector map <%s>: %d)"),
		      line, Vect_get_full_name(Map), Map->plus.n_lines);

    ret = (*Read_line_array[Map->format]) (Map, line_p, line_c, line);

    /*
    if (ret == -1)
	G_fatal_error(_("Unable to read feature %d from vector map <%s>"),
		      line, Vect_get_full_name(Map));
    */
    return ret;
}

/*!
  \brief Check if feature is alive or dead (level 2 required)
  
  \param Map pointer to Map_info structure
  \param line feature id
  
  \return 1 feature alive
  \return 0 feature is dead
 */
int Vect_line_alive(const struct Map_info *Map, int line)
{
    if (Map->plus.Line[line] != NULL)
	return 1;
    
    return 0;
}

/*!
  \brief Check if node is alive or dead (level 2 required)
  
  \param Map pointer to Map_info structure
  \param node node id
  
  \return 1 node alive
  \return 0 node is dead
 */
int Vect_node_alive(const struct Map_info *Map, int node)
{
    if (Map->plus.Node[node] != NULL)
	return 1;

    return 0;
}

/*!
  \brief Check if area is alive or dead (level 2 required)
  
  \param Map pointer to Map_info structure
  \param area area id
  
  \return 1 area alive
  \return 0 area is dead
*/
int Vect_area_alive(const struct Map_info *Map, int area)
{
    if (Map->plus.Area[area] != NULL)
	return 1;

    return 0;
}

/*!
  \brief Check if isle is alive or dead (level 2 required)

  \param Map pointer to Map_info structure
  \param isle isle id
  
  \return 1 isle alive
  \return 0 isle is dead
*/
int Vect_isle_alive(const struct Map_info *Map, int isle)
{
    if (Map->plus.Isle[isle] != NULL)
	return 1;

    return 0;
}

/*!
  \brief Get feature offset
  
  Used for Vect_restore_line().
  
  \param Map pointer to Map_info structure
  \param line feature id

  \return feature offset
  \return -1 on error
*/
off_t Vect_get_line_offset(const struct Map_info *Map, int line)
{
    if (line < 1 || line > Map->plus.n_lines)
	return -1;
    
    if (Map->plus.Line[line] != NULL) {
	return Map->plus.Line[line]->offset;
    }
    
    return -1;
}
