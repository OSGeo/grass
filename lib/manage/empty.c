/*!
   \file lib/manage/empty.c

   \brief Manage Library - Check if element is empty

   (C) 2001-2011 by the GRASS Development Team

    SPDX-License-Identifier: GPL-2.0-or-later
\author Original author CERL
 */

/* look for at least one file in the element */
#include <sys/types.h>
#include <dirent.h>

#include <grass/gis.h>

/*!
   \brief Check if element is empty

   \param elem element name

   \return 1 empty
   \return 0 not empty
 */
int M__empty(char *elem)
{
    DIR *dirp;
    struct dirent *dp;
    char dir[GPATH_MAX];
    int any;

    G_file_name(dir, elem, "", G_mapset());

    any = 0;
    if ((dirp = opendir(dir)) != NULL) {
        while (!any && (dp = readdir(dirp)) != NULL) {
            if (dp->d_name[0] != '.')
                any = 1;
        }
        closedir(dirp);
    }

    return any == 0;
}
