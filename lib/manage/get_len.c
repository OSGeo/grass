/*!
   \file lib/manage/get_len.c

   \brief Manage Library - Get max length of element's description

   (C) 2001-2011 by the GRASS Development Team

    SPDX-License-Identifier: GPL-2.0-or-later
\author Original author CERL
 */

#include <string.h>

#include "manage_local_proto.h"

/*!
   \brief Get max length of element's description

   \param n element id

   \return buffer length
 */
int M__get_description_len(int n)
{
    int len;
    int l;
    int i;

    len = 1;
    for (i = 0; i < list[n].nelem; i++) {
        l = strlen(list[n].desc[i]);
        if (l > len)
            len = l;
    }
    return len;
}
