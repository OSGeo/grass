/*!
 * \file lib/gis/progrm_nme.c
 *
 * \brief GIS Library - Program name
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>

static const char *name = "?";
static const char *original_name = "?";

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
 * \brief Return original path of the executed program
 *
 * This function returns the name of the program as set by the call to
 * G_gisinit().
 *
 * Unlike G_program_name() which returns name of the module
 * this function return original path which was used to execute
 * the program. For standard GRASS modules, it will be the same as
 * the result from G_program_name() function.
 *
 * \return pointer to string with program name or full path
 */
const char *G_original_program_name(void)
{
    return original_name;
}

/*!
  \brief Set program name

  Program name set to name (name will be returned by
  G_program_name*())

  Extension like .exe or .py is stripped from program name.

  \param s program name
*/
void G_set_program_name(const char *s)
{
    int i;
    char *temp;

    original_name = G_store(s);

    i = strlen(s);
    while (--i >= 0) {
	if (G_is_dirsep(s[i])) {
	    s += i + 1;
	    break;
	}
    }

    /* strip extension from program name */
    temp = G_store(s);
    G_basename(temp, "exe");
    G_basename(temp, "py");
    name = G_store(temp);

    G_debug(1, "G_set_program_name(): %s", name);
    
    G_free(temp);
}
