
/****************************************************************************
*
* MODULE:       Vector library 
*   	    	
* AUTHOR(S):    Original author CERL, probably Dave Gerdes or Mike Higgins.
*               Update to GRASS 5.7 Radim Blazek and David D. Gray.
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
#include <grass/Vect.h>
#include <stdlib.h>

/* 
 ** return 0 on success
 **        non-zero on error
 */
int V1_close_nat(struct Map_info *Map)
{
    struct Coor_info CInfo;

    G_debug(1, "V1_close_nat(): name = %s mapset= %s", Map->name,
	    Map->mapset);
    if (!VECT_OPEN(Map))
	return 1;

    if (Map->mode == GV_MODE_WRITE || Map->mode == GV_MODE_RW) {
	Vect_coor_info(Map, &CInfo);
	Map->head.size = CInfo.size;
	dig__write_head(Map);

	Vect__write_head(Map);
	Vect_write_dblinks(Map);
    }

    /* close coor file */
    fclose(Map->dig_fp.file);
    dig_file_free(&(Map->dig_fp));

    return 0;
}
