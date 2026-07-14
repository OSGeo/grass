/*!
   \file lib/db/dbmi_base/whoami.c

   \brief DBMI Library (base) - who am i

   SPDX-FileCopyrightText: 1999-2009, 2011 by the GRASS Development Team

   SPDX-License-Identifier: GPL-2.0-or-later.

   \author Joel Jones (CERL/UIUC), Radim Blazek
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
 */

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>

/*!
   \brief Who am i?

   Check environmental variable LOGNAME

   \return string buffer with logname
 */
const char *db_whoami(void)
{
    return G_store(getenv("LOGNAME"));
}
