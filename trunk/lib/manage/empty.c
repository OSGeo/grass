/*!
  \file lib/manage/empty.c
  
  \brief Manage Library - Check if element is empty
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
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
    char dir[1024];
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
