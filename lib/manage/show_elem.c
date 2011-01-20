/*!
  \file lib/manage/show_elem.c
  
  \brief Manage Library - Show elements
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
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
