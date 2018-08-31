/*!
   \file lib/vector/Vlib/level.c

   \brief Vector library - level info

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Returns level that Map is opened at

   - 1: no topology
   - 2: topology support

   \param Map vector map

   \return open level 
   \return -1 on error
 */
int Vect_level(const struct Map_info *Map)
{
    if (Map->open != VECT_OPEN_CODE) {
	if (Map->open != VECT_CLOSED_CODE)
	    G_warning("Vect_level(): %s",
		      _("Map structure was never initialized"));
	else
	    G_warning("Vect_level(): %s", _("Map structure has been closed"));
	return (-1);
    }
    return (Map->level);
}
