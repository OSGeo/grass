/*  
 ****************************************************************************
 *
 * MODULE:       g.proj 
 * AUTHOR(S):    Paul Kelly - paul-grass@stjohnspoint.co.uk
 *               Frank Warmerdam
 *               Radim Blazek
 * PURPOSE:      Provides a means of reporting the contents of GRASS
 *               projection information files and creating
 *               new projection information files.
 * COPYRIGHT:    (C) 2001-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <grass/config.h>

#ifdef HAVE_OGR
#  include <gdal.h>
#  include <ogr_api.h>
#  include <cpl_csv.h>
#endif

#include "local_proto.h"

static void set_default_region(void);

#ifdef HAVE_OGR
static void set_gdal_region(GDALDatasetH);
#endif

/**
 * \brief Read projection and region information from current location
 * 
 * Reads projection and region information from current location and
 * stores in global structs cellhd, projinfo and projunits.
 **/

void input_currloc(void)
{
    G_get_default_window(&cellhd);
    if (cellhd.proj != PROJECTION_XY) {
	projinfo = G_get_projinfo();
	projunits = G_get_projunits();
        projepsg = G_get_projepsg();
    }

    return;
}

#ifdef HAVE_OGR

/**
 * \brief Read projection information in WKT format from stdin or a file
 * 
 * Reads projection information from a file or stdin and stores in global 
 * structs projinfo and projunits.
 * Populates global cellhd with default region information.
 * 
 * \param wktfile File to read WKT co-ordinate system description from; -
 *                for stdin
 * 
 * \return        2 if a projected or lat/long co-ordinate system has been
 *                defined; 1 if an unreferenced XY co-ordinate system has
 *                been defined
 **/

int input_wkt(char *wktfile)
{
    FILE *infd;
    char buff[8000];
    OGRSpatialReferenceH hSRS;
    int ret;

    if (strcmp(wktfile, "-") == 0)
	infd = stdin;
    else
	infd = fopen(wktfile, "r");

    if (infd) {
	fread(buff, sizeof(buff), 1, infd);
	if (ferror(infd))
	    G_fatal_error(_("Error reading WKT projection description"));
	else
	    fclose(infd);
	/* Get rid of newlines */
	G_squeeze(buff);
    }
    else
	G_fatal_error(_("Unable to open file '%s' for reading"), wktfile);

    ret = GPJ_wkt_to_grass(&cellhd, &projinfo, &projunits, buff, 0);
    set_default_region();

    hSRS = OSRNewSpatialReference(buff);
    if (hSRS) {
	const char *authkey, *authname, *authcode;

	authkey = NULL;
	if (OSRIsProjected(hSRS))
	    authkey = "PROJCS";
	else if (OSRIsGeographic(hSRS))
	    authkey = "GEOGCS";

	if (authkey) {
	    authname = OSRGetAuthorityName(hSRS, authkey);
	    if (authname && *authname && strcmp(authname, "EPSG") == 0) {
		authcode = OSRGetAuthorityCode(hSRS, authkey);
		if (authcode && *authcode) {
		    projepsg = G_create_key_value();
		    G_set_key_value("epsg", authcode, projepsg);
		}
	    }
	}
    }

    return ret;
}

/**
 * \brief Read projection information in PROJ.4 format from a string 
 *        or stdin
 * 
 * Reads projection information from a string or stdin and stores in global 
 * structs projinfo and projunits.
 * Populates global cellhd with default region information.
 * 
 * \param proj4params String representation of PROJ.4 co-ordinate system 
 *                    description, or - to read it from stdin
 * 
 * \return        2 if a projected or lat/long co-ordinate system has been
 *                defined; 1 if an unreferenced XY co-ordinate system has
 *                been defined
 **/

int input_proj4(char *proj4params)
{
    FILE *infd;
    char buff[8000];
    char *proj4string;
    OGRSpatialReferenceH hSRS;
    int ret = 0;

    if (strcmp(proj4params, "-") == 0) {
	infd = stdin;
	fgets(buff, sizeof(buff), infd);
	G_asprintf(&proj4string, "%s +no_defs", buff);
    }
    else
	G_asprintf(&proj4string, "%s +no_defs", proj4params);

    /* Set finder function for locating OGR csv co-ordinate system tables */
    /* SetCSVFilenameHook(GPJ_set_csv_loc); */

    hSRS = OSRNewSpatialReference(NULL);
    if (OSRImportFromProj4(hSRS, proj4string) != OGRERR_NONE)
	G_fatal_error(_("Can't parse PROJ.4-style parameter string"));

    G_free(proj4string);

    ret = GPJ_osr_to_grass(&cellhd, &projinfo, &projunits, hSRS, 0);

    OSRDestroySpatialReference(hSRS);

    set_default_region();

    return ret;
}

/**
 * \brief Read projection information corresponding to an EPSG co-ordinate 
 *        system number
 * 
 * Determines projection information corresponding to an EPSG co-ordinate 
 * system number and stores in global structs projinfo and projunits.
 * Populates global cellhd with default region information.
 * 
 * \param epsg_num    EPSG number for co-ordinate system
 * 
 * \return        2 if a projected or lat/long co-ordinate system has been
 *                defined; 1 if an unreferenced XY co-ordinate system has
 *                been defined
 **/

int input_epsg(int epsg_num)
{
    OGRSpatialReferenceH hSRS;
    char epsgstr[100];
    int ret = 0;

    /* Set finder function for locating OGR csv co-ordinate system tables */
    /* SetCSVFilenameHook(GPJ_set_csv_loc); */

    hSRS = OSRNewSpatialReference(NULL);
    if (OSRImportFromEPSG(hSRS, epsg_num) != OGRERR_NONE)
	G_fatal_error(_("Unable to translate EPSG code"));

    ret = GPJ_osr_to_grass(&cellhd, &projinfo, &projunits, hSRS, 0);

    sprintf(epsgstr, "%d", epsg_num);
    projepsg = G_create_key_value();
    G_set_key_value("epsg", epsgstr, projepsg);

    OSRDestroySpatialReference(hSRS);

    set_default_region();

    return ret;
}

/**
 * \brief Read projection and region information associated with a 
 *        georeferenced file
 * 
 * Reads projection information associated with a georeferenced file, if
 * available. Attempts are made to open the file with OGR and GDAL in turn.
 * (GDAL conventionally supports raster formats, and OGR vector formats.)
 * 
 * If successful, projection and region information are read from the file
 * using GDAL/OGR functions and stored in global structs cellhd, projinfo
 * and projunits.
 * 
 * \param geofile Path to georeferenced file
 * 
 * \return        2 if a projected or lat/long co-ordinate system has been
 *                defined; 1 if an unreferenced XY co-ordinate system has
 *                been defined
 **/

int input_georef(char *geofile)
{
    OGRDataSourceH ogr_ds;
    OGRSpatialReferenceH hSRS;
    int ret = 0;

    /* Try opening file with OGR first because it doesn't output a
     * (potentially confusing) error message if it can't open the file */
    G_debug(1, "Trying to open <%s> with OGR...", geofile);
    OGRRegisterAll();

    hSRS = NULL;
    if ((ogr_ds = OGROpen(geofile, FALSE, NULL))
	&& (OGR_DS_GetLayerCount(ogr_ds) > 0)) {
	OGRLayerH ogr_layer;

	G_debug(1, "...succeeded.");
	/* Get the first layer */
	ogr_layer = OGR_DS_GetLayer(ogr_ds, 0);
	hSRS = OGR_L_GetSpatialRef(ogr_layer);
	ret = GPJ_osr_to_grass(&cellhd, &projinfo, &projunits, hSRS, 0);
	set_default_region();

	OGR_DS_Destroy(ogr_ds);
    }
    else {
	/* Try opening with GDAL */
	GDALDatasetH gdal_ds;

	G_debug(1, "Trying to open with GDAL...");
	GDALAllRegister();

	if ((gdal_ds = GDALOpen(geofile, GA_ReadOnly))) {
	    char *wktstring;

	    G_debug(1, "...succeeded.");
	    wktstring = (char *)GDALGetProjectionRef(gdal_ds);
	    ret =
		GPJ_wkt_to_grass(&cellhd, &projinfo, &projunits, wktstring,
				 0);

	    set_gdal_region(gdal_ds);
	    hSRS = OSRNewSpatialReference(wktstring);
	}
	else {
	    int namelen;

	    namelen = strlen(geofile);
	    if (namelen > 4 && G_strcasecmp(geofile + (namelen - 4), ".prj") == 0) {
		G_warning(_("<%s> is not a GDAL dataset, trying to open it as ESRI WKT"),
			  geofile);

		return input_wkt(geofile);
	    }
	    else {
		G_fatal_error(_("Unable to read georeferenced file <%s> using "
				"GDAL library"), geofile);
	    }
	}
    }

    if (cellhd.proj == PROJECTION_XY)
	G_warning(_("Read of file %s was successful, but it did not contain "
		    "projection information. 'XY (unprojected)' will be used"),
		  geofile);

    if (hSRS) {
	const char *authkey, *authname, *authcode;

	authkey = NULL;
	if (OSRIsProjected(hSRS))
	    authkey = "PROJCS";
	else if (OSRIsGeographic(hSRS))
	    authkey = "GEOGCS";

	if (authkey) {
	    authname = OSRGetAuthorityName(hSRS, authkey);
	    if (authname && *authname && strcmp(authname, "EPSG") == 0) {
		authcode = OSRGetAuthorityCode(hSRS, authkey);
		if (authcode && *authcode) {
		    projepsg = G_create_key_value();
		    G_set_key_value("epsg", authcode, projepsg);
		}
	    }
	}
    }

    return ret;

}
#endif /* HAVE_OGR */

/**
 * \brief Populates global cellhd with "default" region settings
 **/

static void set_default_region(void)
{
    /* If importing projection there will be no region information, so
     * set some default values */
    cellhd.rows = 1;
    cellhd.rows3 = 1;
    cellhd.cols = 1;
    cellhd.cols3 = 1;
    cellhd.depths = 1.;
    cellhd.north = 1.;
    cellhd.ns_res = 1.;
    cellhd.ns_res3 = 1.;
    cellhd.south = 0.;
    cellhd.west = 0.;
    cellhd.ew_res = 1.;
    cellhd.ew_res3 = 1.;
    cellhd.east = 1.;
    cellhd.top = 1.;
    cellhd.tb_res = 1.;
    cellhd.bottom = 0.;

    return;
}

#ifdef HAVE_OGR

/**
 * \brief Populates global cellhd with region settings based on 
 *        georeferencing information in a GDAL dataset
 * 
 * \param hDS GDAL dataset object to retrieve georeferencing information from
 **/

static void set_gdal_region(GDALDatasetH hDS)
{
    double adfGeoTransform[6];

    /* Populate with initial values in case we can't set everything */
    set_default_region();

    /* Code below originally from r.in.gdal */
    cellhd.rows = GDALGetRasterYSize(hDS);
    cellhd.cols = GDALGetRasterXSize(hDS);

    cellhd.rows3 = cellhd.rows;
    cellhd.cols3 = cellhd.cols;

    if (GDALGetGeoTransform(hDS, adfGeoTransform) == CE_None
	&& adfGeoTransform[5] < 0.0) {
	if (adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0) {
	    /* Map is rotated. Calculation of north/south extents and
	     * resolution more complicated.
	     * TODO: do it anyway */

	    return;
	}

	cellhd.north = adfGeoTransform[3];
	cellhd.ns_res = fabs(adfGeoTransform[5]);
	cellhd.south = cellhd.north - cellhd.ns_res * cellhd.rows;
	cellhd.west = adfGeoTransform[0];
	cellhd.ew_res = adfGeoTransform[1];
	cellhd.east = cellhd.west + cellhd.cols * cellhd.ew_res;

	cellhd.ns_res3 = cellhd.ns_res;
	cellhd.ew_res3 = cellhd.ew_res;
    }
    else {
	cellhd.north = cellhd.rows;
	cellhd.east = cellhd.cols;
    }

    return;
}

#endif /* HAVE_OGR */
