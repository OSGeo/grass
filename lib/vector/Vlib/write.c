/*!
   \file lib/vector/Vlib/write.c

   \brief Vector library - write vector features

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2010 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
   \author Updated by Martin Landa <landa.martin gmail.com> (restore lines, OGR support)
 */

#include <sys/types.h>
#include <grass/glocale.h>
#include <grass/vector.h>

static off_t write_dummy()
{
    G_warning("Vect_write_line() %s",
	      _("for this format/level not supported"));
    return -1;
}
static off_t rewrite_dummy()
{
    G_warning("Vect_rewrite_line() %s",
	      _("for this format/level not supported"));
    return -1;
}
static int delete_dummy()
{
    G_warning("Vect_delete_line() %s",
	      _("for this format/level not supported"));
    return -1;
}

static int restore_dummy()
{
    G_warning("Vect_restore_line() %s",
	      _("for this format/level not supported"));
    return -1;
}

#ifndef HAVE_OGR
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}

static off_t format_l()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}

#endif

static off_t (*Vect_write_line_array[][3]) () = {
    {
    write_dummy, V1_write_line_nat, V2_write_line_nat}
#ifdef HAVE_OGR
    , {
    write_dummy, V1_write_line_ogr, V2_write_line_ogr}
    , {
    write_dummy, V1_write_line_ogr, V2_write_line_ogr}
#else
    , {
    write_dummy, format_l, format_l}
    , {
    write_dummy, format_l, format_l}
#endif
};

static off_t (*Vect_rewrite_line_array[][3]) () = {
    {
    rewrite_dummy, V1_rewrite_line_nat, V2_rewrite_line_nat}
#ifdef HAVE_OGR
    , {
    rewrite_dummy, V1_rewrite_line_ogr, V2_rewrite_line_ogr}
    , {
    rewrite_dummy, V1_rewrite_line_ogr, V2_rewrite_line_ogr}
#else
    , {
    rewrite_dummy, format, format}
    , {
    rewrite_dummy, format, format}
#endif
};

static int (*Vect_delete_line_array[][3]) () = {
    {
    delete_dummy, V1_delete_line_nat, V2_delete_line_nat}
#ifdef HAVE_OGR
    , {
    delete_dummy, V1_delete_line_ogr, V2_delete_line_nat}
    , {
    delete_dummy, V1_delete_line_ogr, V2_delete_line_nat}
#else
    , {
    delete_dummy, format, format}
    , {
    delete_dummy, format, format}
#endif
};

static int (*Vect_restore_line_array[][3]) () = {
    {
    restore_dummy, restore_dummy, V2_restore_line_nat}
#ifdef HAVE_OGR
    , {
    restore_dummy, restore_dummy, restore_dummy}
    , {
    restore_dummy, restore_dummy, restore_dummy}
#else
    , {
    restore_dummy, format, format}
    , {
    restore_dummy, format, format}
#endif
};

/*!
   \brief Writes new feature to the end of file

   Vector map topology is not required.

   The function calls G_fatal_error() on error.

   \param Map pointer to vector map
   \param type feature type
   \param points feature geometry
   \param cats feature categories

   \return new feature id (level 2)
   \return offset into file where the feature starts (level 1)
 */
off_t
Vect_write_line(struct Map_info *Map, int type,
		const struct line_pnts *points, const struct line_cats *cats)
{
    off_t offset;

    G_debug(3, "Vect_write_line(): name = %s, format = %d, level = %d",
	    Map->name, Map->format, Map->level);

    if (!VECT_OPEN(Map))
	G_fatal_error(_("Unable to write feature, vector map is not opened"));

    dig_line_reset_updated(&(Map->plus));
    dig_node_reset_updated(&(Map->plus));
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    offset =
	(*Vect_write_line_array[Map->format][Map->level]) (Map, type, points,
						      cats);

    if (offset == -1)
	G_fatal_error(_("Unable to write feature (negative offset)"));

    /* NOTE: returns new line id on level 2 and file offset on level 1 */
    return offset;
}

/*!
   \brief Rewrites feature info at the given offset.

   Vector map must be opened with full topology (level 2).

   The number of points or cats or type may change. If necessary, the
   old feature is deleted and new is written.

   This function calls G_fatal_error() on error.

   \param Map pointer to vector map
   \param line feature id
   \param type feature type
   \param points feature geometry
   \param cats feature categories

   \return feature offset
   \return -1 on error
 */
off_t Vect_rewrite_line(struct Map_info *Map, int line, int type,
			const struct line_pnts *points, const struct line_cats *cats)
{
    off_t ret, offset;
    
    G_debug(3, "Vect_rewrite_line(): name = %s, line = %d", Map->name, line);
    
    if (!VECT_OPEN(Map))
	G_fatal_error(_("Unable to rewrite feature, vector map is not opened"));

    dig_line_reset_updated(&(Map->plus));
    dig_node_reset_updated(&(Map->plus));
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    offset = Map->plus.Line[line]->offset;
    G_debug(3, "   offset=%lu", Map->plus.Line[line]->offset);
    ret =
	(*Vect_rewrite_line_array[Map->format][Map->level]) (Map, line, type, offset,
							     points, cats);

    if (ret == -1)
	G_fatal_error(_("Unable to rewrite feature %d"), line);

    return ret;
}

/*!
   \brief Delete feature

   Vector map must be opened with full topology (level 2).

   This function calls G_fatal_error() on error.

   \param Map pointer to vector map
   \param line feature id

   \return 0 on success
   \return -1 on error
 */
int Vect_delete_line(struct Map_info *Map, int line)
{
    int ret;

    G_debug(3, "Vect_delete_line(): name = %s, line = %d", Map->name, line);

    if (Map->level < 2) {
	G_fatal_error(_("Unable to delete feature %d, "
			"vector map <%s> is not opened on topology level"),
		      line, Map->name);
    }

    if (Map->mode != GV_MODE_RW && Map->mode != GV_MODE_WRITE) {
	G_fatal_error(_("Unable to delete feature %d, "
			"vector map <%s> is not opened in 'write' mode"),
		      line, Map->name);
    }

    dig_line_reset_updated(&(Map->plus));
    dig_node_reset_updated(&(Map->plus));
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    ret = (*Vect_delete_line_array[Map->format][Map->level]) (Map, line);

    if (ret == -1)
	G_fatal_error(_("Unable to delete feature id %d from vector map <%s>"),
		      line, Vect_get_full_name(Map));

    return ret;
}

/*!
   \brief Restore previously deleted feature

   Vector map must be opened with full topology (level 2).

   This function calls G_fatal_error() on error.

   \param Map pointer to vector map
   \param line feature id to be restored

   \return 0 on success
   \return -1 on error
 */
int Vect_restore_line(struct Map_info *Map, int line, off_t offset)
{
    int ret;

    G_debug(3, "Vect_restore_line(): name = %s, line = %d", Map->name, line);

    if (Map->level < 2) {
	G_fatal_error(_("Unable to restore feature %d, "
			"vector map <%s> is not opened on topology level"),
		      line, Map->name);
    }

    if (Map->mode != GV_MODE_RW && Map->mode != GV_MODE_WRITE) {
	G_fatal_error(_("Unable to restore feature %d, "
			"vector map <%s> is not opened in 'write' mode"),
		      line, Map->name);
    }

    dig_line_reset_updated(&(Map->plus));
    dig_node_reset_updated(&(Map->plus));
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    ret = (*Vect_restore_line_array[Map->format][Map->level]) (Map, line, offset);

    if (ret == -1)
	G_fatal_error(_("Unable to restore feature %d from vector map <%s>"),
		      line, Map->name);
    
    return ret;
}
