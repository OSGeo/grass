/*!
   \file lib/vector/Vlib/open_ogr.c

   \brief Vector library - Open OGR layer as vector map layer

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2010 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Update to GRASS 7.0 Martin Landa <landa.martin gmail.com> (2009)
 */

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>

static int sqltype_to_ogrtype(int);
#endif

/*!
   \brief Open existing OGR layer on non-topological level

   Note: Map->name, Map->mapset, Map->fInfo.ogr.dsn and
   Map->fInfo.ogr.layer_name must be set before.

   \param[in,out] Map pointer to Map_info structure
   \param update TRUE for write mode, otherwise read-only
   
   \return 0 success
   \return -1 error
*/
int V1_open_old_ogr(struct Map_info *Map, int update)
{
#ifdef HAVE_OGR
    int i, layer, nLayers;

    struct Format_info_ogr *ogr_info;
    
    OGRDataSourceH Ogr_ds;
    OGRLayerH Ogr_layer;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRwkbGeometryType Ogr_geom_type;
    
    Ogr_layer = NULL;
    Ogr_geom_type = wkbUnknown;

    ogr_info = &(Map->fInfo.ogr);
    if (!ogr_info->dsn) {
	G_fatal_error(_("OGR datasource not defined"));
	return -1;
    }
    
    if (!ogr_info->layer_name) {
	G_fatal_error(_("OGR layer not defined"));
	return -1;
    }
    
    G_debug(2, "V1_open_old_ogr(): dsn = %s layer = %s", ogr_info->dsn,
	    ogr_info->layer_name);

    OGRRegisterAll();

    /* open data source handle */
    Ogr_ds = OGROpen(ogr_info->dsn, FALSE, NULL);
    if (Ogr_ds == NULL)
	G_fatal_error(_("Unable to open OGR data source '%s'"),
		      ogr_info->dsn);
    ogr_info->ds = Ogr_ds;

    /* get layer number */
    layer = -1;
    nLayers = OGR_DS_GetLayerCount(Ogr_ds);
    G_debug(2, "%d layers found in data source", nLayers);

    for (i = 0; i < nLayers; i++) {
	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);	
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
	if (strcmp(OGR_FD_GetName(Ogr_featuredefn), ogr_info->layer_name) == 0) {
	    Ogr_geom_type = OGR_FD_GetGeomType(Ogr_featuredefn);
	    layer = i;
	    break;
	}
    }
    if (layer == -1) {
	OGR_DS_Destroy(Ogr_ds);
	G_fatal_error(_("OGR layer <%s> not found"),
		      ogr_info->layer_name);
    }
    G_debug(2, "OGR layer %d opened", layer);

    ogr_info->layer = Ogr_layer;
    if (update && OGR_L_TestCapability(ogr_info->layer, OLCTransactions))
	OGR_L_StartTransaction(ogr_info->layer);
    
    switch(Ogr_geom_type) {
    case wkbPoint25D: case wkbLineString25D: case wkbPolygon25D:
    case wkbMultiPoint25D: case wkbMultiLineString25D: case wkbMultiPolygon25D:
    case wkbGeometryCollection25D:
	Map->head.with_z = WITH_Z;
	break;
    default:
	Map->head.with_z = WITHOUT_Z;
	break;
    }
    
    ogr_info->cache.fid = -1;	/* FID >= 0 */
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
   \brief Open existing OGR layer on topological level

   This functions reads feature index (fidx) file required for
   pseudo-topology.

   \param[in,out] Map pointer to Map_info structure
   
   \return 0 success
   \return -1 error
*/
int V2_open_old_ogr(struct Map_info *Map)
{
#ifdef HAVE_OGR

    G_debug(3, "V2_open_old_ogr(): name = %s mapset = %s", Map->name,
	    Map->mapset);

    if (Vect_open_fidx(Map, &(Map->fInfo.ogr.offset)) != 0) {
	G_warning(_("Unable to open feature index file for vector map <%s>"),
		  Vect_get_full_name(Map));
	G_zero(&(Map->fInfo.ogr.offset), sizeof(struct Format_info_offset));
    }
    
    Map->fInfo.ogr.next_line = 1; /* reset feature cache */

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
   \brief Prepare OGR datasource for creating new OGR layer (level 1)

   New OGR layer is created by Vect__open_new_ogr().
   
   \param[out] Map pointer to Map_info structure
   \param name name of OGR layer to create
   \param with_z WITH_Z for 3D vector data otherwise WITHOUT_Z

   \return 0 success
   \return -1 error 
*/
int V1_open_new_ogr(struct Map_info *Map, const char *name, int with_z)
{
#ifdef HAVE_OGR
    int            i, nlayers;

    struct Format_info_ogr *ogr_info;
    
    OGRSFDriverH    Ogr_driver;
    OGRDataSourceH  Ogr_ds;
    OGRLayerH       Ogr_layer;
    OGRFeatureDefnH Ogr_featuredefn; 
    
    OGRRegisterAll();
    
    ogr_info = &(Map->fInfo.ogr);
    
    G_debug(1, "V1_open_new_ogr(): name = %s with_z = %d", name, with_z);
    Ogr_driver = OGRGetDriverByName(ogr_info->driver_name);
    if (!Ogr_driver) {
	G_warning(_("Unable to get OGR driver <%s>"), ogr_info->driver_name);
	return -1;
    }
    ogr_info->driver = Ogr_driver;
    
    /* TODO: creation options */
    Ogr_ds = OGR_Dr_CreateDataSource(Ogr_driver, ogr_info->dsn, NULL);
    if (!Ogr_ds) {
	G_warning(_("Unable to create OGR data source '%s'"),
		  ogr_info->dsn);
	return -1;
    }
    ogr_info->ds = Ogr_ds;

    nlayers = OGR_DS_GetLayerCount(Ogr_ds);
    for (i = 0; i < nlayers; i++) {
      	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
	if (strcmp(OGR_FD_GetName(Ogr_featuredefn), name) == 0) {	
	    if (G_get_overwrite()) {
		G_warning(_("OGR layer <%s> already exists and will be overwritten"),
			  ogr_info->layer_name);
		
		if (OGR_DS_DeleteLayer(Ogr_ds, i) != OGRERR_NONE) {
		    G_warning(_("Unable to delete OGR layer <%s>"),
			      ogr_info->layer_name);
		    return -1;
		}
	    }
	    else {
		G_fatal_error(_("OGR layer <%s> already exists in datasource '%s'"),
			      ogr_info->layer_name, ogr_info->dsn);
	    }
	    ogr_info->layer = NULL;
	    break;
	}
    }
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
  \brief Open feature index file
  
  \param[in,out] Map pointer to Map_info struct
  \param[out] offset pointer to Format_info_offset (OGR or PG)
  
  \return 0 on success
  \return -1 on error
*/
int Vect_open_fidx(struct Map_info *Map, struct Format_info_offset *offset)
{
    char elem[GPATH_MAX];
    char buf[5];		/* used for format version */
    long length;
    int Version_Major, Version_Minor, Back_Major, Back_Minor, byte_order;
    
    struct gvfile fp;
    struct Port_info port;
    
    G_debug(1, "Vect_open_fidx(): name = %s mapset = %s format = %d",
	    Map->name, Map->mapset, Map->format);
    
    sprintf(elem, "%s/%s", GV_DIRECTORY, Map->name);
    dig_file_init(&fp);
    fp.file = G_fopen_old(elem, GV_FIDX_ELEMENT, Map->mapset);
    if (fp.file == NULL) {
        G_debug(1, "unable to open fidx file for vector map <%s>",
                Vect_get_full_name(Map));
	return -1;
    }

    /* Header */
    if (0 >= dig__fread_port_C(buf, 5, &fp))
	return -1;
    Version_Major = buf[0];
    Version_Minor = buf[1];
    Back_Major    = buf[2];
    Back_Minor    = buf[3];
    byte_order    = buf[4];
    
    /* check version numbers */
    if (Version_Major > 5 || Version_Minor > 0) {
	if (Back_Major > 5 || Back_Minor > 0) {
	    G_fatal_error(_("Feature index format version %d.%d is not supported by this release."
			   " Try to rebuild topology or upgrade GRASS."),
			  Version_Major, Version_Minor);
	    return -1;
	}
	G_warning(_("Your GRASS version does not fully support feature index format %d.%d of the vector."
		   " Consider to rebuild topology or upgrade GRASS."),
		  Version_Major, Version_Minor);
    }

    dig_init_portable(&port, byte_order);
    dig_set_cur_port(&port);

    /* Body */
    /* bytes 6 - 9 : header size */
    if (0 >= dig__fread_port_L(&length, 1, &fp))
	return -1;
    G_debug(4, "  header size %ld", length);

    G_fseek(fp.file, length, SEEK_SET);

    /* number of records  */
    if (0 >= dig__fread_port_I(&(offset->array_num), 1, &fp))
	return -1;
    
    /* alloc space */
    offset->array = (int *) G_malloc(offset->array_num * sizeof(int));
    offset->array_alloc = offset->array_num;
    
    /* offsets */
    if (0 >= dig__fread_port_I(offset->array,
			       offset->array_num, &fp))
	return -1;

    fclose(fp.file);

    G_debug(3, "%d records read from fidx", offset->array_num);

    return 0;
}

#ifdef HAVE_OGR
int sqltype_to_ogrtype(int sqltype)
{
    int ctype, ogrtype;

    ctype = db_sqltype_to_Ctype(sqltype);
    
    switch(ctype) {
    case DB_C_TYPE_INT:
	ogrtype = OFTInteger;
	break;
    case DB_C_TYPE_DOUBLE:
	ogrtype = OFTReal;
	break;
    case DB_C_TYPE_STRING:
	ogrtype = OFTString;
	break;
    case DB_C_TYPE_DATETIME:
	ogrtype = OFTString;
	break;
    default:
	ogrtype = OFTString;
	break;
    }
    
    return ogrtype;
}
#endif
