
/*****************************************************************************
*
* MODULE:       OGR driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      DB driver for OGR sources     
*
* COPYRIGHT:    (C) 2004 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "ogr_api.h"
#include "globals.h"
#include "proto.h"

int db__driver_list_tables(dbString ** tlist, int *tcount, int system)
{
    int i, nlayers;
    dbString *list;
    OGRLayerH hLayer;
    OGRFeatureDefnH hFeatureDefn;

    init_error();
    *tlist = NULL;
    *tcount = 0;

    nlayers = OGR_DS_GetLayerCount(hDs);
    G_debug(3, "%d layers found", nlayers);

    list = db_alloc_string_array(nlayers);

    if (list == NULL) {
	append_error("Cannot db_alloc_string_array()");
	report_error();
	return DB_FAILED;
    }

    for (i = 0; i < nlayers; i++) {
	hLayer = OGR_DS_GetLayer(hDs, i);
	hFeatureDefn = OGR_L_GetLayerDefn(hLayer);
	db_set_string(&(list[i]), (char *)OGR_FD_GetName(hFeatureDefn));
    }

    *tlist = list;
    *tcount = nlayers;
    return DB_OK;
}
