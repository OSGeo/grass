/*!
 * \file gis/progrm_nme.c
 *
 * \brief GIS Library - Program name
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>

static const char *name = "?";

/*!
 * \brief Return module name
 *
 * Routine returns the name of the module as set by the call to
 * G_gisinit().
 *
 * \return pointer to string with program name
 */
const char *G_program_name(void)
{
    return name;
}

/*!
  \brief Set program name

  Program name set to name (name will be returned by
  G_program_name*())

  \param s program name
*/
void G_set_program_name(const char *s)
{
    int i;
    char *temp;

    i = strlen(s);
    while (--i >= 0) {
	if (G_is_dirsep(s[i])) {
	    s += i + 1;
	    break;
	}
    }
    temp = G_store(s);
    G_basename(temp, "exe");
    name = G_store(temp);
    G_free(temp);
}
