/*!
  \file lib/manage/option.c
  
  \brief Manage Library - Define option for parser
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "manage_local_proto.h"

/*!
  \brief Define option for parser

  \param n element id

  \return pointer to Option structure
  \return NULL on error
*/
struct Option* M_define_option(int n, const char *desc, int multiple)
{
    char *str;
    struct Option *p;
    
    if (n >= nlist)
	return NULL;
    
    p = G_define_option();
    p->key = list[n].alias;
    p->type = TYPE_STRING;
    if (multiple)
	p->key_desc = "name";
    else
	p->key_desc = "from,to";
    p->required = NO;
    p->multiple = multiple;
    G_asprintf(&str, "old,%s,%s", list[n].mainelem, list[n].maindesc);
    p->gisprompt = str;
    G_asprintf(&str, _("%s to be %s"),
	       list[n].text, desc);
    p->description = str;
    if (strcmp(p->key, "raster") == 0 || strcmp(p->key, "raster_3d") == 0)
	p->guisection = _("Raster");
    else if (strcmp(p->key, "vector") == 0)
	p->guisection = _("Vector");
    else if (strcmp(p->key, "region") == 0)
	p->guisection = _("Region");
    else if (strcmp(p->key, "group") == 0)
	p->guisection = _("Group");

    return p;
}

/*!
  \brief Get list of element types separated by comma

  String buffer is allocated by G_malloc().
  
  \param do_all TRUE to add "all" to the buffer
  
  \return pointer to allocated buffer with types
*/
const char *M_get_options(int do_all)
{
    int len, n;
    char *str;

    for (len = 0, n = 0; n < nlist; n++)
	len += strlen(list[n].alias) + 1;
    if (do_all)
	len += 4;
    str = G_malloc(len);
    
    for (n = 0; n < nlist; n++) {
	if (n) {
	    strcat(str, ",");
	    strcat(str, list[n].alias);
	}
	else
	    strcpy(str, list[n].alias);
    }

    if (do_all)
	strcat(str, ",all");
    
    return str;
}

/*!
  \brief Get list of element desc separated by comma

  String buffer is allocated by G_malloc().
  
  \param do_all TRUE to add "all" to the buffer
  
  \return pointer to allocated buffer with desc
*/
const char *M_get_option_desc(int do_all)
{
    int len, n;
    char *str;
    const char *str_all = "all;all types";

    for (len = 0, n = 0; n < nlist; n++) {
	len += strlen(list[n].alias) + 1;
	len += strlen(list[n].text) + 1;
    }
    if (do_all)
	len += strlen(str_all) + 1;
    str = G_malloc(len);
    
    for (n = 0; n < nlist; n++) {
	if (n) {
	    strcat(str, ";");
	    strcat(str, list[n].alias);
	    strcat(str, ";");
	    strcat(str, list[n].text);
	}
	else {
	    strcpy(str, list[n].alias);
	    strcat(str, ";");
	    strcat(str, list[n].text);
	}
    }

    if (do_all) {
	strcat(str, ";");
	strcat(str, str_all);
    }

    return str;
}
