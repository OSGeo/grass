/*
 **  Written by David Gerdes  US Army Construction Engineering Research Lab
 **     April 1992
 **  SPDX-FileCopyrightText: 1992 USA-CERL
 **  SPDX-FileCopyrightText: Other GRASS authors
 **  SPDX-License-Identifier: GPL-2.0-or-later
 **
 */

#include <grass/linkm.h>

void link_dispose(struct link_head *Head, VOID_T *ptr)
{
    if (NULL == ptr)
        return;

    link__set_next(ptr, Head->Unused); /* ptr->next = Unused */
    Head->Unused = ptr;                /* Unused = ptr */
}
