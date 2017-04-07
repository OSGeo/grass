/*!
   \file lib/vector/Vlib/build_ogr.c

   \brief Vector library - Building topology for OGR

   Higher level functions for reading/writing/manipulating vectors.
   
   Category: FID, not all layer have FID, OGRNullFID is defined
   (5/2004) as -1, so FID should be only >= 0

   (C) 2001-2010, 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek, Piero Cavalieri
   \author Various updates for GRASS 7 by Martin Landa <landa.martin gmail.com>
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>
#include <cpl_error.h>
#endif

#include "local_proto.h"

/*!
   \brief Build pseudo-topology (simple features) for OGR layer

   Build levels:
    - GV_BUILD_NONE
    - GV_BUILD_BASE
    - GV_BUILD_ATTACH_ISLES
    - GV_BUILD_CENTROIDS
    - GV_BUILD_ALL
   
   \param Map pointer to Map_info structure
   \param build build level

   \return 1 on success
   \return 0 on error
 */
int Vect_build_ogr(struct Map_info *Map, int build)
{
#ifdef HAVE_OGR
    struct Plus_head *plus;
    struct Format_info_ogr *ogr_info;

    plus     = &(Map->plus);
    ogr_info = &(Map->fInfo.ogr);
    
    G_debug(1, "Vect_build_ogr(): dsn='%s' layer='%s', build=%d",
	    ogr_info->dsn, ogr_info->layer_name, build);
    
    if (build == plus->built)
	return 1;		/* do nothing */
    
    /* TODO move this init to better place (Vect_open_ ?), because in
       theory build may be reused on level2 */
    if (build >= plus->built && build > GV_BUILD_BASE) {
	G_free((void *) ogr_info->offset.array);
	G_zero(&(ogr_info->offset), sizeof(struct Format_info_offset));
    }

    if (!ogr_info->layer) {
	G_warning(_("Empty OGR layer, nothing to build"));
	return 0;
    }
    
    if (OGR_L_TestCapability(ogr_info->layer, OLCTransactions)) {
        CPLPushErrorHandler(CPLQuietErrorHandler); 
	if (OGR_L_CommitTransaction(ogr_info->layer) != OGRERR_NONE)
            G_debug(1, "Unable to commit transaction");
        CPLPushErrorHandler(CPLDefaultErrorHandler); 
    }

    /* test layer capabilities */
    if (!OGR_L_TestCapability(ogr_info->layer, OLCRandomRead)) {
	if (strcmp(OGR_Dr_GetName(OGR_DS_GetDriver(Map->fInfo.ogr.ds)),
		   "PostgreSQL") == 0)
	    G_warning(_("Feature table <%s> has no primary key defined"),
		      ogr_info->layer_name);
	G_warning(_("Random read is not supported by OGR for this layer. "
		    "Unable to build topology."));
	return 0;
    }

    if (build > GV_BUILD_NONE)
	G_message(_("Using external data format '%s' (feature type '%s')"),
		  Vect_get_finfo_format_info(Map),
		  Vect_get_finfo_geometry_type(Map));
    
    return Vect__build_sfa(Map, build);
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return 0;
#endif
}

/*!
   \brief Save feature index file for vector map

   \param Map pointer to Map_info structure
   \param offset pointer to Format_info_offset struct
   (see Format_info_ogr and Format_info_pg struct for implementation issues)

   \return 1 on success
   \return 0 on error
 */
int Vect_save_fidx(struct Map_info *Map,
		   struct Format_info_offset *offset)
{
#ifdef HAVE_OGR
    char fname[GPATH_MAX], elem[GPATH_MAX];
    char buf[5];
    long length;
    struct gvfile fp;
    struct Port_info port;

    if (strcmp(Map->mapset, G_mapset()) != 0 ||
	Map->support_updated == FALSE ||
	Map->plus.built != GV_BUILD_ALL)
	return 1;
    
    length = 9;

    sprintf(elem, "%s/%s", GV_DIRECTORY, Map->name);
    Vect__get_element_path(fname, Map, GV_FIDX_ELEMENT);
    G_debug(4, "Open fidx: %s", fname);
    dig_file_init(&fp);
    fp.file = fopen(fname, "w");
    if (fp.file == NULL) {
	G_warning(_("Unable to open fidx file for write <%s>"), fname);
	return 0;
    }
    
    dig_init_portable(&port, dig__byte_order_out());
    dig_set_cur_port(&port);
    
    /* Header */
    /* bytes 1 - 5 */
    buf[0] = 5;
    buf[1] = 0;
    buf[2] = 5;
    buf[3] = 0;
    buf[4] = (char)dig__byte_order_out();
    if (0 >= dig__fwrite_port_C(buf, 5, &fp))
	return 0;
    
    /* bytes 6 - 9 : header size */
    if (0 >= dig__fwrite_port_L(&length, 1, &fp))
	return 0;
    
    /* Body */
    /* number of records  */
    if (0 >= dig__fwrite_port_I(&(offset->array_num), 1, &fp))
	return 0;
    
    /* offsets */
    if (0 >= dig__fwrite_port_I(offset->array,
				offset->array_num, &fp))
	return 0;
    
    G_debug(3, "Vect_save_fidx(): offset_num = %d", offset->array_num);
    
    fclose(fp.file);

    return 1;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return 0;
#endif
}
