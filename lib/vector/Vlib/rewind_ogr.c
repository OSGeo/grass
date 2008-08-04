
/****************************************************************************
*
* MODULE:       Vector library 
*   	    	
* AUTHOR(S):    Radim Blazek, Piero Cavalieri 
*
* PURPOSE:      Higher level functions for reading/writing/manipulating vectors.
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <grass/gis.h>
#include <grass/Vect.h>

#ifdef HAVE_OGR
#include <ogr_api.h>

/* Rewind vector data file to cause reads to start at beginning. 
 ** returns 0 on success
 **        -1 on error
 */
int V1_rewind_ogr(struct Map_info *Map)
{
    G_debug(2, "V1_rewind_ogr(): name = %s", Map->name);

    Map->fInfo.ogr.lines_num = 0;
    Map->fInfo.ogr.lines_next = 0;

    OGR_L_ResetReading(Map->fInfo.ogr.layer);

    return 0;
}

int V2_rewind_ogr(struct Map_info *Map)
{
    G_debug(2, "V2_rewind_ogr(): name = %s", Map->name);

    Map->next_line = 1;

    return 0;
}

#endif
