/**
 * \file basename.c
 *
 * \brief Program basename routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2007
 */

#include <grass/gis.h>

#include <ctype.h>
#include <string.h>


/**
 * \fn char * G_basename (char *filename, const char *desired_ext)
 *
 * \brief Truncates filename to the base part (before the last '.')
 * if it matches the extension, otherwise leaves it unchanged.
 * 
 * Checks if a filename matches a certain file extension
 * (case insensitive) and if so, truncates the string to the
 * base file name (cf. basename Unix command)
 *
 * \param[in] filename string containing filename
 * \param[in] desired_ext string containing extension to look for (case
 * insensitive)
 * \return Pointer to filename
 */

char * G_basename(char *filename, const char *desired_ext)
{
    /* Find the last . in the filename */
    char *dot = strrchr(filename, '.');

    if(dot && G_strcasecmp(dot + 1, desired_ext) == 0)
        *dot = '\0';

    return filename;
}
