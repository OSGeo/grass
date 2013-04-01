/*!
   \file lib/vector/Vlib/write.c

   \brief Vector library - write vector features

   Higher level functions for reading/writing/manipulating vectors.

   Supported operations:
    - Write a new feature
    - Rewrite existing feature
    - Delete existing feature
    - Restore deleted feature

   (C) 2001-2010, 2012-2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
   \author Updated by Martin Landa <landa.martin gmail.com> (restore lines, OGR & PostGIS support)
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

#if !defined HAVE_OGR || !defined HAVE_POSTGRES
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
	write_dummy, V1_write_line_ogr, V2_write_line_sfa}
    , {
	write_dummy, V1_write_line_ogr, V2_write_line_sfa}
#else
    , {
	write_dummy, format_l, format_l}
    , {
	write_dummy, format_l, format_l}
#endif
#ifdef HAVE_POSTGRES
    , {
	write_dummy, V1_write_line_pg, V2_write_line_pg}
#else
    , {
	write_dummy, format_l, format_l}
#endif
};

static off_t (*Vect_rewrite_line_array[][3]) () = {
    {
	rewrite_dummy, V1_rewrite_line_nat, V2_rewrite_line_nat}
#ifdef HAVE_OGR
    , {
	rewrite_dummy, V1_rewrite_line_ogr, V2_rewrite_line_sfa}
    , {
	rewrite_dummy, V1_rewrite_line_ogr, V2_rewrite_line_sfa}
#else
    , {
	rewrite_dummy, format_l, format_l}
    , {
	rewrite_dummy, format_l, format_l}
#endif
#ifdef HAVE_POSTGRES
    , {
	rewrite_dummy, V1_rewrite_line_pg, V2_rewrite_line_pg}
#else
    , {
	rewrite_dummy, format_l, format_l}
#endif
};

static int (*Vect_delete_line_array[][3]) () = {
    {
	delete_dummy, V1_delete_line_nat, V2_delete_line_nat}
#ifdef HAVE_OGR
    , {
	delete_dummy, V1_delete_line_ogr, V2_delete_line_sfa}
    , {
	delete_dummy, V1_delete_line_ogr, V2_delete_line_sfa}
#else
    , {
	delete_dummy, format, format}
    , {
	delete_dummy, format, format}
#endif
#ifdef HAVE_POSTGRES
    , {
	delete_dummy, V1_delete_line_pg, V2_delete_line_pg}
#else
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
#ifdef HAVE_POSTGRES
    , {
	restore_dummy, restore_dummy, restore_dummy}
#else
    , {
	restore_dummy, format, format}
#endif
};

static int check_map(const struct Map_info *);

/*!
   \brief Writes a new feature

   New feature is written to the end of file (in the case of native
   format). Topological level is not required.

   A warning is printed on error.
   
   \param Map pointer to Map_info structure
   \param type feature type (see dig_defines.h for supported types)
   \param points pointer to line_pnts structure (feature geometry)
   \param cats pointer to line_cats structure (feature categories)

   \return new feature id (on level 2) (or 0 when build level < GV_BUILD_BASE)
   \return offset into file where the feature starts (on level 1)
   \return -1 on error
 */
off_t Vect_write_line(struct Map_info *Map, int type,
		      const struct line_pnts *points, const struct line_cats *cats)
{
    off_t offset;

    G_debug(3, "Vect_write_line(): name = %s, format = %d, level = %d",
	    Map->name, Map->format, Map->level);

    if (!VECT_OPEN(Map)) {
	G_warning(_("Vector map <%s> is not opened"), Vect_get_name(Map));
        return -1;
    }
    
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = FALSE; /* category index will be outdated */
    }

    offset = 
	(*Vect_write_line_array[Map->format][Map->level]) (Map, type, points,
							   cats);
    
    if (offset < 0)
	G_warning(_("Unable to write feature in vector map <%s>"), Vect_get_name(Map));
    
    return offset;
}

/*!
   \brief Rewrites existing feature (topological level required)

   Note: Topology must be built at level >= GV_BUILD_BASE
   
   A warning is printed on error.

   The number of points or cats or type may change. If necessary, the
   old feature is deleted and new is written.

   \param Map pointer to Map_info structure
   \param line feature id
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points feature geometry
   \param cats feature categories

   \return new feature offset
   \return -1 on error
 */
off_t Vect_rewrite_line(struct Map_info *Map, int line, int type,
			const struct line_pnts *points, const struct line_cats *cats)
{
    off_t ret, offset;
    
    if (!check_map(Map))
        return -1;

    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }
    
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = FALSE; /* category index will be outdated */
    }

    offset = Map->plus.Line[line]->offset;
    
    G_debug(3, "Vect_rewrite_line(): name = %s, line = %d, offset = %lu",
	    Map->name, line, offset);
    
    ret = (*Vect_rewrite_line_array[Map->format][Map->level]) (Map, line, type,
							       offset,
							       points, cats);
    if (ret == -1)
        G_warning(_("Unable to rewrite feature %d in vector map <%s>"), line, Vect_get_name(Map));

    return ret;
}

/*!
   \brief Delete existing feature (topological level required)

   Note: Topology must be built at level >= GV_BUILD_BASE
   
   A warning is printed on error.

   \param Map pointer to Map_info structure
   \param line feature id

   \return 0 on success
   \return -1 on error
 */
int Vect_delete_line(struct Map_info *Map, int line)
{
    int ret;

    G_debug(3, "Vect_delete_line(): name = %s, line = %d", Map->name, line);

    if (!check_map(Map))
        return -1;
    
    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }

    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = FALSE; /* category index will be outdated */
    }

    ret = (*Vect_delete_line_array[Map->format][Map->level]) (Map, line);

    if (ret == -1)
	G_warning(_("Unable to delete feature %d from vector map <%s>"),
                  line, Vect_get_name(Map));

    return ret;
}

/*!
   \brief Restore previously deleted feature (topological level required)

   Note: Topology must be built at level >= GV_BUILD_BASE
   
   A warning is printed on error.

   \param Map pointer to Map_info structure
   \param line feature id to be restored
   \param offset feature offset

   \return 0 on success
   \return -1 on error
 */
int Vect_restore_line(struct Map_info *Map, int line, off_t offset)
{
    int ret;

    G_debug(3, "Vect_restore_line(): name = %s, line = %d", Map->name, line);

    if (!check_map(Map))
        return -1;

    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }

    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    ret = (*Vect_restore_line_array[Map->format][Map->level]) (Map, line, offset);

    if (ret == -1)
	G_warning(_("Unable to restore feature %d in vector map <%s>"),
                  line, Vect_get_name(Map));

    return ret;
}

int check_map(const struct Map_info *Map)
{
    if (!VECT_OPEN(Map)) {
	G_warning(_("Vector map <%s> is not opened"), Vect_get_name(Map));
        return 0;
    }
    
    if (Map->level < 2) {
	G_warning(_("Vector map <%s> is not opened on topology level"),
                  Vect_get_name(Map));
        return 0;
    }

    if (Map->mode != GV_MODE_RW && Map->mode != GV_MODE_WRITE) {
	G_warning(_("Vector map <%s> is not opened in write mode"),
                  Vect_get_name(Map));
        return 0;
    }
    
    return 1;
}
