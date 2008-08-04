
/**
 * \file store.c
 *
 * \brief GIS Library - String storage functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <grass/gis.h>
#include <string.h>


/**
 * \brief Copy string to allocated memory.
 *
 * This routine allocates enough memory to hold the string <b>s</b>,
 * copies <b>s</b> to the allocated memory, and returns a pointer
 * to the allocated memory.
 * 
 *  \param[in] s string
 *  \return pointer to newly allocated string
 */

char *G_store(const char *s)
{
    char *buf;

    buf = G_malloc(strlen(s) + 1);
    strcpy(buf, s);

    return buf;
}
