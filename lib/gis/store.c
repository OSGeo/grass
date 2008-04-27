/**
 * \file store.c
 *
 * \brief String storage functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <grass/gis.h>
#include <string.h>


/**
 * \fn char *G_store (const char *s)
 *
 * \brief Copy string to allocated memory.
 *
 * This routine allocates enough memory to hold the string <b>s</b>,
 * copies <b>s</b> to the allocated memory, and returns a pointer
 * to the allocated memory.
 * 
 *  \param[in] s
 *  \return char * 
 */

char *G_store  (const char *s)

{
    char *buf;

    buf = G_malloc (strlen(s) + 1);
    strcpy (buf, s);

    return buf;
}
