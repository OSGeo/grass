/*!
 * \file get_projinfo.c
 * 
 * \brief GRASS GIS Library
 *
 * (C) 1999-2007 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 */

#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define PERMANENT "PERMANENT"

/*!
 * \fn struct Key_Value* G_get_projunits(void) 
 *
 * \brief Gets unit information for location
 *
 * \return pointer to Key_Value structure with key/value pairs
 */
struct Key_Value* G_get_projunits(void) 
{
	int stat;
        struct Key_Value *in_units_keys;
	char path[GPATH_MAX];

         G__file_name (path, "", UNIT_FILE, PERMANENT);
         if (access(path,0) != 0)
         {
           G_warning (_("<%s> file not found for location <%s>"),
		      UNIT_FILE, G_location());
           return NULL;
         }
         in_units_keys = G_read_key_value_file(path,&stat);
         if (stat != 0)
         { 
             G_warning (_("ERROR in reading <%s> file for location <%s>"),
			UNIT_FILE, G_location());
             return NULL;
         }

         return in_units_keys;
}

/*!
 * \fn struct Key_Value* G_get_projinfo(void) 
 *
 * \brief Gets projection information for location
 *
 * \return pointer to Key_Value structure with key/value pairs
 */
struct Key_Value* G_get_projinfo(void) 
{
	int stat;
        struct Key_Value *in_proj_keys;
	char path[GPATH_MAX];

         G__file_name (path, "", PROJECTION_FILE, PERMANENT);
         if (access(path,0) != 0)
         {
	     G_warning (_("<%s> file not found for location <%s>"),
			PROJECTION_FILE, G_location());
           return NULL;
         }
         in_proj_keys = G_read_key_value_file(path,&stat);
         if (stat != 0)
         { 
             G_warning (_("ERROR in reading <%s> file for location <%s>"),
			PROJECTION_FILE, G_location());
           return NULL;
         }
         return in_proj_keys;
}
