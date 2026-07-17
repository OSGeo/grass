/*!
   \file lib/gis/gisdbase.c

   \brief GIS library - environment routines (gisdbase)

   SPDX-FileCopyrightText: 2001-2009 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Original author CERL
 */

#include <grass/gis.h>

/*!
 * \brief Get name of top level database directory
 *
 * Returns the full UNIX path name of the directory which holds the
 * database locations.
 *
 *  \return pointer to string containing the base directory
 */
const char *G_gisdbase(void)
{
    return G_getenv("GISDBASE");
}
