/*!
   \file lib/vector/Vlib/read.c

   \brief Vector library - read vector features

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <sys/types.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static int read_next_dummy()
{
    return -1;
}

#ifndef HAVE_OGR
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}
#endif

static int (*Read_next_line_array[][3]) () = {
    {
    read_next_dummy, V1_read_next_line_nat, V2_read_next_line_nat}
#ifdef HAVE_OGR
    , {
    read_next_dummy, V1_read_next_line_ogr, V2_read_next_line_ogr}
    , {
    read_next_dummy, V1_read_next_line_ogr, V2_read_next_line_ogr}
#else
    , {
    read_next_dummy, format, format}
    , {
    read_next_dummy, format, format}
#endif
};

static int (*V2_read_line_array[]) () = {
    V2_read_line_nat
#ifdef HAVE_OGR
	, V2_read_line_ogr
	, V2_read_line_ogr
#else
	, format
	, format
#endif
};

/*!
   \brief Read next vector feature (level 1 and 2)

   This function implements sequential access.
     
   \param Map pointer vector map
   \param[out] line_p feature geometry
   \param[out] line_c feature categories

   \return feature type,
   \return -1 out of memory
   \return -2 EOF   
 */
int Vect_read_next_line(const struct Map_info *Map,
			struct line_pnts *line_p, struct line_cats *line_c)
{

    G_debug(3, "Vect_read_next_line()");

    if (!VECT_OPEN(Map))
	return -1;

    return (*Read_next_line_array[Map->format][Map->level]) (Map, line_p,
							     line_c);
}

/*!
   \brief Read vector feature  (level 2 only)

   This function implements random access.

   \param Map pointer to vector map
   \param[out] line_p feature geometry
   \param[out] line_c feature categories
   \param line feature id 
   
   \return feature type
   \return -1 out of memory,
   \return -2 EOF   
 */
int Vect_read_line(const struct Map_info *Map,
		   struct line_pnts *line_p, struct line_cats *line_c, int line)
{

    G_debug(3, "Vect_read_line() line=%d", line);

    if (!VECT_OPEN(Map))
	G_fatal_error("Vect_read_line(): %s", _("vector map is not opened"));

    if (line < 1 || line > Map->plus.n_lines)
	G_fatal_error(_("Vect_read_line(): feature id %d is not reasonable "
			"(max features in vector map <%s>: %d)"),
		      line, Vect_get_full_name(Map), Map->plus.n_lines);

    return (*V2_read_line_array[Map->format]) (Map, line_p, line_c, line);
}

/*!
   \brief Check if feature is alive or dead

   \param Map pointer to vector map
   \param line feature id

   \return 1 if feature alive
   \return 0 if feature is dead
 */
int Vect_line_alive(const struct Map_info *Map, int line)
{
    if (Map->plus.Line[line] != NULL)
	return 1;
    
    return 0;
}

/*!
   \brief Check if node is alive or dead

   \param Map pointer to vector map
   \param node node id

   \return 1 if node alive
   \return 0 if node is dead
 */
int Vect_node_alive(const struct Map_info *Map, int node)
{
    if (Map->plus.Node[node] != NULL)
	return 1;

    return 0;
}

/*!
   \brief Check if area is alive or dead

   \param Map pointer to vector map
   \param area area id

   \return 1 if area alive
   \return 0 if area is dead
 */
int Vect_area_alive(const struct Map_info *Map, int area)
{
    if (Map->plus.Area[area] != NULL)
	return 1;

    return 0;
}

/*!
   \brief Check if isle is alive or dead

   \param Map pointer to vector map
   \param isle isle id

   \return 1 if isle alive
   \return 0 if isle is dead
 */
int Vect_isle_alive(const struct Map_info *Map, int isle)
{
    if (Map->plus.Isle[isle] != NULL)
	return 1;

    return 0;
}

/*!
  \brief Get feature offset

  Can be used for Vect_restore_line().

  \param Map pointer to vector map
  \param line feature id

  \return feature offset
  \return -1 on error
*/
off_t Vect_get_line_offset(const const struct Map_info *Map, int line)
{
    if (line < 1 || line > Map->plus.n_lines)
	return -1;
    
    if (Map->plus.Line[line] != NULL) {
	return Map->plus.Line[line]->offset;
    }
    
    return -1;
}
