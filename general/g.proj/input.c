/*****************************************************************************
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
#include <gdal.h>
#include <ogr_api.h>
#include <cpl_csv.h>
#endif

#include "local_proto.h"

static void set_default_region(void);

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
        projsrid = G_get_projsrid();
        projwkt = G_get_projwkt();
        projinfo = G_get_projinfo();
        projunits = G_get_projunits();
        /* projepsg = G_get_projepsg(); */
    }

    return;
}

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
static void set_gdal_region(GDALDatasetH);
static void set_authnamecode(OGRSpatialReferenceH);

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
    char buff[8192], *tmpwkt;
    OGRSpatialReferenceH hSRS;
    char *papszOptions[3];
    int ret;

    if (strcmp(wktfile, "-") == 0)
        infd = stdin;
    else
        infd = fopen(wktfile, "r");

    if (infd) {
        size_t wktlen;

        wktlen = fread(buff, 1, sizeof(buff), infd);
        if (wktlen == sizeof(buff))
            G_fatal_error(_("Input WKT definition is too long"));
        if (ferror(infd))
            G_fatal_error(_("Error reading WKT definition"));
        else
            fclose(infd);
        /* terminate WKT string */
        buff[wktlen] = '\0';
        /* Get rid of newlines */
        G_squeeze(buff);
    }
    else
        G_fatal_error(_("Unable to open file '%s' for reading"), wktfile);

    projwkt = G_store(buff);

#if PROJ_VERSION_MAJOR >= 6
    /* validate input WKT */
    {
        PROJ_STRING_LIST wkt_warnings, wkt_grammar_errors;
        PJ *obj;

        wkt_warnings = NULL;
        wkt_grammar_errors = NULL;

        /* no strict validation */
        obj = proj_create_from_wkt(NULL, buff, NULL, &wkt_warnings,
                                   &wkt_grammar_errors);

        if (wkt_warnings) {
            int i;

            G_warning(_("WKT validation warnings:"));
            for (i = 0; wkt_warnings[i]; i++)
                G_warning("%s", wkt_warnings[i]);

            proj_string_list_destroy(wkt_warnings);
        }

        if (wkt_grammar_errors) {
            int i;

            G_warning(_("WKT validation grammar errors:"));
            for (i = 0; wkt_grammar_errors[i]; i++)
                G_warning("%s", wkt_grammar_errors[i]);

            proj_string_list_destroy(wkt_grammar_errors);
        }
        proj_destroy(obj);
    }
#endif

    /* get GRASS proj info + units */
    /* NOTE: GPJ_wkt_to_grass() converts any WKT version to WKT1 */
    ret = GPJ_wkt_to_grass(&cellhd, &projinfo, &projunits, buff, 0);
    if (ret < 2)
        G_fatal_error(_("WKT not recognized: %s"), buff);

    set_default_region();

    /* find authname and authcode */
    hSRS = OSRNewSpatialReference(buff);
    /* get clean WKT definition */
#if GDAL_VERSION_MAJOR >= 3
    papszOptions[0] = G_store("MULTILINE=YES");
    papszOptions[1] = G_store("FORMAT=WKT2");
    papszOptions[2] = NULL;
    OSRExportToWktEx(hSRS, &tmpwkt, (const char **)papszOptions);
    G_free(papszOptions[0]);
    G_free(papszOptions[1]);
#else
    OSRExportToPrettyWkt(hSRS, &tmpwkt, FALSE);
#endif
    G_free(projwkt);
    projwkt = G_store(tmpwkt);
    CPLFree(tmpwkt);
    set_authnamecode(hSRS);
    OSRDestroySpatialReference(hSRS);

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

    /* TEST: use PROJ proj_create(), convert to WKT,
     *       OSRImportFromWkt  */
    if (strcmp(proj4params, "-") == 0) {
        infd = stdin;
        if (fgets(buff, sizeof(buff), infd) == NULL)
            G_warning(_("Failed to read PROJ.4 parameter from stdin"));
    }
    else {
        if (G_strlcpy(buff, proj4params, sizeof(buff)) >= sizeof(buff)) {
            G_fatal_error(_("PROJ.4 parameter string is too long: %s"),
                          proj4params);
        }
    }

#if PROJ_VERSION_MAJOR >= 6
    if (!strstr(buff, "+type=crs"))
        G_asprintf(&proj4string, "%s +no_defs +type=crs", buff);
    else
        G_asprintf(&proj4string, "%s +no_defs", buff);
#else
    G_asprintf(&proj4string, "%s +no_defs", buff);
#endif

    /* Set finder function for locating OGR csv co-ordinate system tables */
    /* SetCSVFilenameHook(GPJ_set_csv_loc); */

    hSRS = OSRNewSpatialReference(NULL);
    if (OSRImportFromProj4(hSRS, proj4string) != OGRERR_NONE)
        G_fatal_error(_("Can't parse PROJ.4-style parameter string"));

    G_free(proj4string);

    ret = GPJ_osr_to_grass(&cellhd, &projinfo, &projunits, hSRS, 0);

    /* authority name and code not available in PROJ definitions */

    OSRDestroySpatialReference(hSRS);

    set_default_region();

    return ret;
}

/**
 * \brief Read projection information corresponding to a spatial
 *        reference id (srid)
 *
 * Determines projection information corresponding to a srid
 * composed of authority name and code and stores in global structs
 * projinfo and projunits. Populates global cellhd with default region
 * information.
 *
 * Examples: "EPSG:4326", "urn:ogc:def:crs:EPSG::4326"
 *
 * \param srid    spatial reference id
 *
 * \return        2 if a projected or lat/long co-ordinate system has been
 *                defined; 1 if an unreferenced XY co-ordinate system has
 *                been defined
 **/

int input_srid(char *srid)
{
#if PROJ_VERSION_MAJOR >= 6
    OGRSpatialReferenceH hSRS;
    int ret = 0;
    char *papszOptions[3];
    PJ *obj;
    const char *tmpwkt;

    /* GDAL alternative: OSRSetFromUserInput() */
    obj = proj_create(NULL, srid);
    if (!obj)
        G_fatal_error(_("SRID <%s> not recognized by PROJ"), srid);

    tmpwkt = proj_as_wkt(NULL, obj, PJ_WKT2_LATEST, NULL);
    hSRS = OSRNewSpatialReference(tmpwkt);
    if (!hSRS)
        G_fatal_error(_("WKT for SRID <%s> not recognized by GDAL"), srid);

    projsrid = G_store(srid);

    /* WKT */
    papszOptions[0] = G_store("MULTILINE=YES");
    papszOptions[1] = G_store("FORMAT=WKT2");
    papszOptions[2] = NULL;
    if (OSRExportToWktEx(hSRS, &projwkt, (const char **)papszOptions) !=
        OGRERR_NONE)
        G_warning(_("Unable to convert srid to WKT"));
    G_free(papszOptions[0]);
    G_free(papszOptions[1]);
    /* GRASS proj info + units */
    ret = GPJ_osr_to_grass(&cellhd, &projinfo, &projunits, hSRS, 0);

    proj_destroy(obj);
    OSRDestroySpatialReference(hSRS);

    set_default_region();
    return ret;
#else
    G_fatal_error(_("Input srid requires GDAL 3+ and PROJ 6+"));

    return 0;
#endif
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
    char *papszOptions[3];

    /* Set finder function for locating OGR csv co-ordinate system tables */
    /* SetCSVFilenameHook(GPJ_set_csv_loc); */

    hSRS = OSRNewSpatialReference(NULL);
    if (OSRImportFromEPSG(hSRS, epsg_num) != OGRERR_NONE)
        G_fatal_error(_("Unable to translate EPSG code"));

    /* GRASS proj info + units */
    ret = GPJ_osr_to_grass(&cellhd, &projinfo, &projunits, hSRS, 0);

    /* EPSG code */
    sprintf(epsgstr, "%d", epsg_num);
    projepsg = G_create_key_value();
    G_set_key_value("epsg", epsgstr, projepsg);
    /* srid as AUTHORITY:CODE */
    G_asprintf(&projsrid, "EPSG:%s", epsgstr);

#if GDAL_VERSION_MAJOR >= 3
    /* WKT */
    papszOptions[0] = G_store("MULTILINE=YES");
    papszOptions[1] = G_store("FORMAT=WKT2");
    papszOptions[2] = NULL;
    if (OSRExportToWktEx(hSRS, &projwkt, (const char **)papszOptions) !=
        OGRERR_NONE)
        G_warning(_("Unable to convert EPSG code to WKT"));

    G_free(papszOptions[0]);
    G_free(papszOptions[1]);
#endif

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
    /* GDAL >= 3 */
#if GDAL_VERSION_MAJOR >= 3
    GDALDatasetH hDS = NULL;
    OGRSpatialReferenceH hSRS = NULL;
    int ret = 0;

    GDALAllRegister();

    /* Try opening file as vector first because it doesn't output a
     * (potentially confusing) error message if it can't open the file */

    /* Try opening as vector */
    G_debug(1, "Trying to open <%s> as vector...", geofile);
    if ((hDS = GDALOpenEx(geofile, GDAL_OF_VECTOR, NULL, NULL, NULL)) &&
        GDALDatasetGetLayerCount(hDS) > 0) {

        OGRLayerH ogr_layer;

        ogr_layer = GDALDatasetGetLayer(hDS, 0);
        hSRS = OGR_L_GetSpatialRef(ogr_layer);

        if (hSRS)
            set_default_region();
    }
    else {
        /* Try opening as raster */
        G_debug(1, "Trying to open <%s> as raster...", geofile);

        if ((hDS = GDALOpen(geofile, GA_ReadOnly))) {
            char **sds;

            /* does the dataset include subdatasets? */
            sds = GDALGetMetadata(hDS, "SUBDATASETS");
            if (sds && *sds) {
                G_warning(_("Input dataset <%s> contains subdatasets. "
                            "Please select a subdataset."),
                          geofile);
            }
            else {
                hSRS = GDALGetSpatialRef(hDS);

                if (hSRS)
                    set_gdal_region(hDS);
            }
        }
        else {
            int namelen;

            namelen = strlen(geofile);
            if (namelen > 4 &&
                G_strcasecmp(geofile + (namelen - 4), ".prj") == 0) {
                G_warning(_("<%s> is not a GDAL dataset, trying to open it as "
                            "ESRI WKT"),
                          geofile);

                return input_wkt(geofile);
            }
            else {
                G_fatal_error(_("Unable to read georeferenced file <%s> using "
                                "GDAL library"),
                              geofile);
            }
        }
    }
    if (hSRS) {
        char **papszOptions = NULL;

        ret = GPJ_osr_to_grass(&cellhd, &projinfo, &projunits, hSRS, 0);

        if (cellhd.proj == PROJECTION_XY)
            G_warning(
                _("Read of file %s was successful, but it did not contain "
                  "projection information. 'XY (unprojected)' will be used"),
                geofile);

        papszOptions = G_calloc(3, sizeof(char *));
        papszOptions[0] = G_store("MULTILINE=YES");
        papszOptions[1] = G_store("FORMAT=WKT2");
        OSRExportToWktEx(hSRS, &projwkt, (const char **)papszOptions);
        G_free(papszOptions[0]);
        G_free(papszOptions[1]);
        G_free(papszOptions);

        set_authnamecode(hSRS);
    }
    if (hDS)
        GDALClose(hDS);

    return ret;

    /* GDAL < 3 */
#else
    OGRDataSourceH ogr_ds;
    OGRSpatialReferenceH hSRS;
    int ret = 0;

    /* Try opening file with OGR first because it doesn't output a
     * (potentially confusing) error message if it can't open the file */
    G_debug(1, "Trying to open <%s> with OGR...", geofile);
    OGRRegisterAll();

    ogr_ds = NULL;
    hSRS = NULL;
    /* Try opening with OGR */
    if ((ogr_ds = OGROpen(geofile, FALSE, NULL)) &&
        (OGR_DS_GetLayerCount(ogr_ds) > 0)) {
        OGRLayerH ogr_layer;

        G_debug(1, "...succeeded.");
        /* Get the first layer */
        ogr_layer = OGR_DS_GetLayer(ogr_ds, 0);
        hSRS = OGR_L_GetSpatialRef(ogr_layer);
        ret = GPJ_osr_to_grass(&cellhd, &projinfo, &projunits, hSRS, 0);
        OSRExportToWkt(hSRS, &projwkt);
        set_default_region();
    }
    else {
        /* Try opening with GDAL */
        GDALDatasetH gdal_ds;

        G_debug(1, "Trying to open with GDAL...");
        GDALAllRegister();

        if ((gdal_ds = GDALOpen(geofile, GA_ReadOnly))) {
            char *wktstring;

            G_debug(1, "...succeeded.");
            /* TODO: change for GDAL 3+ */
            wktstring = (char *)GDALGetProjectionRef(gdal_ds);
            projwkt = G_store(wktstring);
            ret = GPJ_wkt_to_grass(&cellhd, &projinfo, &projunits, projwkt, 0);

            set_gdal_region(gdal_ds);
            hSRS = OSRNewSpatialReference(projwkt);
            GDALClose(gdal_ds);
        }
        else {
            int namelen;

            namelen = strlen(geofile);
            if (namelen > 4 &&
                G_strcasecmp(geofile + (namelen - 4), ".prj") == 0) {
                G_warning(_("<%s> is not a GDAL dataset, trying to open it as "
                            "ESRI WKT"),
                          geofile);

                return input_wkt(geofile);
            }
            else {
                G_fatal_error(_("Unable to read georeferenced file <%s> using "
                                "GDAL library"),
                              geofile);
            }
        }
    }

    if (cellhd.proj == PROJECTION_XY)
        G_warning(_("Read of file %s was successful, but it did not contain "
                    "projection information. 'XY (unprojected)' will be used"),
                  geofile);

    set_authnamecode(hSRS);

    if (ogr_ds)
        OGR_DS_Destroy(ogr_ds);
    else
        OSRDestroySpatialReference(hSRS);

    return ret;
#endif
}

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

    if (GDALGetGeoTransform(hDS, adfGeoTransform) == CE_None &&
        adfGeoTransform[5] < 0.0) {
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

void set_authnamecode(OGRSpatialReferenceH hSRS)
{
    const char *authkey, *authname, *authcode;

    if (!hSRS)
        return;

    authkey = NULL;
    if (OSRIsProjected(hSRS))
        authkey = "PROJCS";
    else if (OSRIsGeographic(hSRS))
        authkey = "GEOGCS";

    if (authkey) {
        authname = OSRGetAuthorityName(hSRS, authkey);
        if (authname && *authname) {
            authcode = OSRGetAuthorityCode(hSRS, authkey);
            if (authcode && *authcode) {
                G_asprintf(&projsrid, "%s:%s", authname, authcode);
                /* for backwards compatibility; remove ? */
                if (strcmp(authname, "EPSG") == 0) {
                    projepsg = G_create_key_value();
                    G_set_key_value("epsg", authcode, projepsg);
                }
            }
        }
    }
}

#endif /* HAVE_OGR */
