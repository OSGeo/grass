/*!
  \file lib/gis/get_projinfo.c
  
  \brief GIS Library - Get projection info
  
  (C) 1999-2014 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
*/

#include <unistd.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define PERMANENT "PERMANENT"

/*!
  \brief Gets units information for location

  Note: Allocated Key_Value structure should be freed by
  G_free_key_value().
  
  Prints a warning if no units information available.

  \return pointer to Key_Value structure with key/value pairs
  \return NULL on failure
*/
struct Key_Value *G_get_projunits(void)
{
    struct Key_Value *in_units_keys;
    char path[GPATH_MAX];

    G_file_name(path, "", UNIT_FILE, PERMANENT);
    if (access(path, 0) != 0) {
	if (G_projection() != PROJECTION_XY) {
	    G_warning(_("<%s> file not found for location <%s>"),
		      UNIT_FILE, G_location());
	}
	return NULL;
    }
    in_units_keys = G_read_key_value_file(path);

    return in_units_keys;
}

/*!
  \brief Gets projection information for location
  
  Note: Allocated Key_Value structure should be freed by
  G_free_key_value().
  
  Prints a warning if no projection information available.

  \return pointer to Key_Value structure with key/value pairs
  \return NULL on failure
*/
struct Key_Value *G_get_projinfo(void)
{
    struct Key_Value *in_proj_keys, *in_epsg_keys;
    char path[GPATH_MAX];

    G_file_name(path, "", PROJECTION_FILE, PERMANENT);
    if (access(path, 0) != 0) {
	if (G_projection() != PROJECTION_XY) {
	    G_warning(_("<%s> file not found for location <%s>"),
		      PROJECTION_FILE, G_location());
	}
	return NULL;
    }
    in_proj_keys = G_read_key_value_file(path);
    
    /* TODO: do not restrict to EPSG as the only authority */
    if ((in_epsg_keys = G_get_projepsg()) != NULL) {
	const char *epsgstr = G_find_key_value("epsg", in_epsg_keys);
	char buf[4096];
	
	sprintf(buf, "EPSG:%s", epsgstr);
	G_set_key_value("init", buf, in_proj_keys);
	G_free_key_value(in_epsg_keys);
    }

    return in_proj_keys;
}

/*!
  \brief Gets EPSG information for the current location
  
  Note: Allocated Key_Value structure should be freed by
  G_free_key_value().
  
  \return pointer to Key_Value structure with key/value pairs
  \return NULL when EPSG code is not defined for location
*/
struct Key_Value *G_get_projepsg(void)
{
    struct Key_Value *in_epsg_keys;
    char path[GPATH_MAX];

    G_file_name(path, "", EPSG_FILE, PERMANENT);
    if (access(path, 0) != 0) {
	if (G_projection() != PROJECTION_XY) {
            G_debug(1, "<%s> file not found for location <%s>",
                    EPSG_FILE, G_location());
	}
	return NULL;
    }
    in_epsg_keys = G_read_key_value_file(path);

    return in_epsg_keys;
}
