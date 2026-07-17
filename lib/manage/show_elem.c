/*!
   \file lib/manage/show_elem.c

   \brief Manage Library - Show elements

   SPDX-FileCopyrightText: 2001-2011 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Original author CERL
 */

#include <stdio.h>
#include <string.h>

#include "manage_local_proto.h"

/*!
   \brief Print element name/desc to stdout
 */
void M_show_elements(void)
{
    int n;
    unsigned int len;

    len = 0;
    for (n = 0; n < nlist; n++)
        if (strlen(list[n].alias) > len)
            len = strlen(list[n].alias);
    for (n = 0; n < nlist; n++)
        fprintf(stdout, "  %-*s (%s)\n", len, list[n].alias, list[n].text);
}
