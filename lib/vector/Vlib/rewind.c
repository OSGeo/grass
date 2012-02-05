/*!
   \file lib/vector/Vlib/rewind.c

   \brief Vector library - rewind data

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
*/

#include <grass/vector.h>
#include <grass/glocale.h>

static int rew_dummy()
{
    return -1;
}

#if !defined HAVE_OGR || !defined HAVE_POSTGRES
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}
#endif


static int (*Rewind_array[][3]) () = {
    {
	rew_dummy, V1_rewind_nat, V2_rewind_nat}
#ifdef HAVE_OGR
    , {
	rew_dummy, V1_rewind_ogr, V2_rewind_ogr}
    , {
	rew_dummy, V1_rewind_ogr, V2_rewind_ogr}
#else
    , {
	rew_dummy, format, format}
    , {
	rew_dummy, format, format}
#endif
#ifdef HAVE_POSTGRES
    , {
	rew_dummy, V1_rewind_pg, V2_rewind_pg}
#else
    , {
	rew_dummy, format, format}
#endif
};

/*!
   \brief Rewind vector map to cause reads to start at beginning

   \param Map pointer to Map_info structure

   \return 0 on success
   \return -1 on error
 */
int Vect_rewind(struct Map_info *Map)
{
    if (!VECT_OPEN(Map))
	return -1;

    G_debug(1, "Vect_Rewind(): name = %s", Map->name);

    return (*Rewind_array[Map->format][Map->level]) (Map);
}
