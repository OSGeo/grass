/*!
  \file db/drivers/listtab.c
  
  \brief Low level OGR SQL driver
 
  (C) 2004-2009 by the GRASS Development Team
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
  \author Some updates by Martin Landa <landa.martin gmail.com>
*/

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <ogr_api.h>

#include "globals.h"
#include "proto.h"

/*!
  \brief List tables

  \param[out] tlist list of tables
  \param[out] tcount number of tables
  \param system list also system tables (unused)

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db__driver_list_tables(dbString ** tlist, int *tcount, int system)
{
    int i, nlayers;
    dbString *list;
    OGRLayerH hLayer;
    OGRFeatureDefnH hFeatureDefn;

    *tlist = NULL;
    *tcount = 0;

    nlayers = OGR_DS_GetLayerCount(hDs);
    G_debug(3, "%d layers found", nlayers);

    list = db_alloc_string_array(nlayers);

    if (list == NULL) {
	db_d_append_error(_("Unable to db_alloc_string_array()"));
	db_d_report_error();
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
