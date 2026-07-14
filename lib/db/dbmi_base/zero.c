/*!
   \file lib/db/dbmi_base/zero.c

   \brief DBMI Library (base) - zero

   SPDX-FileCopyrightText: 1999-2009, 2011 by the GRASS Development Team

   SPDX-License-Identifier: GPL-2.0-or-later.

   \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
 */

#include <grass/dbmi.h>

/*!
   \brief Zero allocated space

   \param s pointer to memory
   \param n number of bytes
 */
void db_zero(void *s, int n)
{
    char *c = (char *)s;

    while (n-- > 0)
        *c++ = 0;
}
