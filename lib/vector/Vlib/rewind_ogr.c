/*!
   \file lib/vector/Vlib/rewind.c

   \brief Vector library - rewind data (native format)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek, Piero Cavalieri 
*/

#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>
#endif

/*!
  \brief Rewind vector data file to cause reads to start at
  beginning (level 1)

  \param Map pointer to Map_info structure

  \return 0 on success
  \return -1 on error
 */
int V1_rewind_ogr(struct Map_info *Map)
{
    G_debug(2, "V1_rewind_ogr(): name = %s", Map->name);
#ifdef HAVE_OGR
    Map->fInfo.ogr.lines_num = 0;
    Map->fInfo.ogr.lines_next = 0;

    OGR_L_ResetReading(Map->fInfo.ogr.layer);

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
  \brief Rewind vector data file to cause reads to start at
  beginning (level 2)

  \param Map pointer to Map_info structure

  \return 0 on success
  \return -1 on error
 */
int V2_rewind_ogr(struct Map_info *Map)
{
    G_debug(2, "V2_rewind_ogr(): name = %s", Map->name);
#ifdef HAVE_OGR
    Map->next_line = 1;

    V1_rewind_ogr(Map);

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}
