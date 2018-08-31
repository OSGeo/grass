/*!
  \file lib/manage/read_list.c
  
  \brief Manage Library - Read list of elements
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "manage_local_proto.h"

int nlist;
struct list *list;

static void format_error(char *, int, char *);

/*!
  \brief Read list of elements

  Format:

  \code
   # ... comments
   main element:alias:description:menu text
      sub element:description
      sub element:description
	  .
	  .
	  .
  \endcode

  \param check_if_empty TRUE for check if element is empty

  \return 0
  \return 1
*/
int M_read_list(int check_if_empty, int *num)
{
    FILE *fd;
    char element_list[GPATH_MAX];
    char buf[1024];
    char elem[100];
    char alias[100];
    char desc[100];
    char text[100];
    int any;
    int line;
    char *env;

    nlist = 0;
    list = 0;
    any = 0;

    env = getenv("ELEMENT_LIST");
    if (env)
	strcpy(element_list, env);
    else
	sprintf(element_list, "%s/etc/element_list", G_gisbase());
    fd = fopen(element_list, "r");

    if (!fd)
	G_fatal_error(_("Unable to open data base element list '%s'"), element_list);

    line = 0;
    while (G_getl(buf, sizeof(buf), fd)) {
	line++;
	if (*buf == '#')
	    continue;
	if (*buf == ' ' || *buf == '\t') {	/* support element */
	    *desc = 0;
	    if (sscanf(buf, "%[^:]:%[^\n]", elem, desc) < 1)
		continue;
	    if (*elem == '#')
		continue;
	    if (nlist == 0)
		format_error(element_list, line, buf);

	    G_strip(elem);
	    G_strip(desc);
	    M__add_element(elem, desc);
	}
	else {			/* main element */

	    if (sscanf
		(buf, "%[^:]:%[^:]:%[^:]:%[^\n]", elem, alias, desc,
		 text) != 4)
		format_error(element_list, line, buf);

	    G_strip(elem);
	    G_strip(alias);
	    G_strip(desc);
	    G_strip(text);

	    list =
		(struct list *)G_realloc(list, (nlist + 1) * sizeof(*list));
	    list[nlist].mainelem = G_store(elem);
	    list[nlist].alias = G_store(alias);
	    list[nlist].maindesc = G_store(desc);
	    list[nlist].text = G_store(text);
	    list[nlist].nelem = 0;
	    list[nlist].element = 0;
	    list[nlist].desc = 0;
	    list[nlist].status = 0;
	    if (!check_if_empty || !M__empty(elem)) {
		list[nlist].status = 1;
		any = 1;
	    }
	    nlist++;
	    M__add_element(elem, desc);
	}
    }

    if (num)
	*num = nlist;

    fclose(fd);

    return any;
}

void format_error(char *element_list, int line, char *buf)
{
    G_fatal_error(_("Format error: file ('%s') line (%d) - %s"), element_list, line,
		  buf);
}
