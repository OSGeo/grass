
/*!
   \file lib/proj/convert.c

   \brief GProj Library - Functions for manipulating co-ordinate
   system representations

   (C) 2003-2018 by the GRASS Development Team
 
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Paul Kelly, Frank Warmerdam, Markus Metz 
*/

#include <grass/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <cpl_csv.h>
#include "local_proto.h"

/* GRASS relative location of OGR co-ordinate system lookup tables */
#define CSVDIR "/etc/proj/ogr_csv"

static void DatumNameMassage(char **);
#endif

/* from proj-5.0.0/src/pj_units.c */
struct gpj_units {
    char    *id;        /* units keyword */
    char    *to_meter;  /* multiply by value to get meters */
    char    *name;      /* comments */
    double   factor;    /* to_meter factor in actual numbers */
};

struct gpj_units
gpj_units[] = {
    {"km",      "1000.",                "Kilometer",                    1000.0},
    {"m",       "1.",                   "Meter",                        1.0},
    {"dm",      "1/10",                 "Decimeter",                    0.1},
    {"cm",      "1/100",                "Centimeter",                   0.01},
    {"mm",      "1/1000",               "Millimeter",                   0.001},
    {"kmi",     "1852.0",               "International Nautical Mile",  1852.0},
    {"in",      "0.0254",               "International Inch",           0.0254},
    {"ft",      "0.3048",               "International Foot",           0.3048},
    {"yd",      "0.9144",               "International Yard",           0.9144},
    {"mi",      "1609.344",             "International Statute Mile",   1609.344},
    {"fath",    "1.8288",               "International Fathom",         1.8288},
    {"ch",      "20.1168",              "International Chain",          20.1168},
    {"link",    "0.201168",             "International Link",           0.201168},
    {"us-in",   "1./39.37",             "U.S. Surveyor's Inch",         0.0254},
    {"us-ft",   "0.304800609601219",    "U.S. Surveyor's Foot",         0.304800609601219},
    {"us-yd",   "0.914401828803658",    "U.S. Surveyor's Yard",         0.914401828803658},
    {"us-ch",   "20.11684023368047",    "U.S. Surveyor's Chain",        20.11684023368047},
    {"us-mi",   "1609.347218694437",    "U.S. Surveyor's Statute Mile", 1609.347218694437},
    {"ind-yd",  "0.91439523",           "Indian Yard",                  0.91439523},
    {"ind-ft",  "0.30479841",           "Indian Foot",                  0.30479841},
    {"ind-ch",  "20.11669506",          "Indian Chain",                 20.11669506},
    {NULL,      NULL,                   NULL,                           0.0}
};

static char *grass_to_wkt(const struct Key_Value *proj_info,
                          const struct Key_Value *proj_units,
                          const struct Key_Value *proj_epsg,
                          int esri_style, int prettify)
{
#ifdef HAVE_OGR
    OGRSpatialReferenceH hSRS;
    char *wkt, *local_wkt;

    hSRS = GPJ_grass_to_osr2(proj_info, proj_units, proj_epsg);

    if (hSRS == NULL)
	return NULL;

    if (esri_style)
	OSRMorphToESRI(hSRS);

    if (prettify)
	OSRExportToPrettyWkt(hSRS, &wkt, 0);
    else
	OSRExportToWkt(hSRS, &wkt);

    local_wkt = G_store(wkt);
    CPLFree(wkt);
    OSRDestroySpatialReference(hSRS);

    return local_wkt;
#else
    G_warning(_("GRASS is not compiled with OGR support"));
    return NULL;
#endif
}

/*!
 * \brief Converts a GRASS co-ordinate system representation to WKT style.
 * 
 * Takes a GRASS co-ordinate system as specified by two sets of
 * key/value pairs derived from the PROJ_INFO and PROJ_UNITS files,
 * and converts it to the 'Well Known Text' format.
 * 
 * \param proj_info Set of GRASS PROJ_INFO key/value pairs
 * \param proj_units Set of GRASS PROJ_UNIT key/value pairs
 * \param esri_style boolean Output ESRI-style WKT (Use OSRMorphToESRI() 
 *                   function provided by OGR library)
 * \param prettify boolean Use linebreaks and indents to 'prettify' output
 *                 WKT string (Use OSRExportToPrettyWkt() function in OGR)
 *
 * \return Pointer to a string containing the co-ordinate system in
 *         WKT format
 * \return NULL on error
 */
char *GPJ_grass_to_wkt(const struct Key_Value *proj_info,
                       const struct Key_Value *proj_units,
                       int esri_style, int prettify)
{
    return grass_to_wkt(proj_info, proj_units, NULL, esri_style, prettify);
}

/*!
 * \brief Converts a GRASS co-ordinate system representation to WKT
 * style. EPSG code is preferred if available.
 * 
 * Takes a GRASS co-ordinate system as specified key/value pairs
 * derived from the PROJ_EPSG file. TOWGS84 parameter is scanned
 * from PROJ_INFO file and appended to co-ordinate system definition
 * imported from EPSG code by GDAL library. PROJ_UNITS file is
 * ignored. The function converts it to the 'Well Known Text' format.
 * 
 * \todo Merge with GPJ_grass_to_wkt() in GRASS 8.
 *
 * \param proj_info Set of GRASS PROJ_INFO key/value pairs
 * \param proj_units Set of GRASS PROJ_UNIT key/value pairs
 * \param proj_epsg Set of GRASS PROJ_EPSG key/value pairs
 * \param esri_style boolean Output ESRI-style WKT (Use OSRMorphToESRI() 
 *                   function provided by OGR library)
 * \param prettify boolean Use linebreaks and indents to 'prettify' output
 *                 WKT string (Use OSRExportToPrettyWkt() function in OGR)
 *
 * \return Pointer to a string containing the co-ordinate system in
 *         WKT format
 * \return NULL on error
 */
char *GPJ_grass_to_wkt2(const struct Key_Value *proj_info,
                       const struct Key_Value *proj_units,
                       const struct Key_Value *proj_epsg,
                       int esri_style, int prettify)
{
    return grass_to_wkt(proj_info, proj_units, proj_epsg, esri_style, prettify);
}

#ifdef HAVE_OGR
/*!
 * \brief Converts a GRASS co-ordinate system to an OGRSpatialReferenceH object.
 * 
 * \param proj_info Set of GRASS PROJ_INFO key/value pairs
 * \param proj_units Set of GRASS PROJ_UNIT key/value pairs
 * 
 * \return OGRSpatialReferenceH object representing the co-ordinate system
 *         defined by proj_info and proj_units or NULL if it fails
 */
OGRSpatialReferenceH GPJ_grass_to_osr(const struct Key_Value * proj_info,
				      const struct Key_Value * proj_units)
{
    struct pj_info pjinfo;
    char *proj4, *proj4mod, *wkt, *modwkt, *startmod, *lastpart;
    OGRSpatialReferenceH hSRS, hSRS2;
    OGRErr errcode;
    struct gpj_datum dstruct;
    struct gpj_ellps estruct;
    size_t len;
    const char *ellpskv, *unit, *unfact;
    char *ellps, *ellpslong, *datum, *params, *towgs84, *datumlongname,
	*start, *end;
    const char *sysname, *osrunit, *osrunfact;
    double a, es, rf;
    int haveparams = 0;

    if ((proj_info == NULL) || (proj_units == NULL))
	return NULL;

    hSRS = OSRNewSpatialReference(NULL);

    /* create PROJ structure from GRASS key/value pairs */
    if (pj_get_kv(&pjinfo, proj_info, proj_units) < 0) {
	G_warning(_("Unable parse GRASS PROJ_INFO file"));
	return NULL;
    }

    /* fetch the PROJ definition */
    /* TODO: get the PROJ definition as used by pj_get_kv() */
    if ((proj4 = pjinfo.def) == NULL) {
	G_warning(_("Unable get PROJ.4-style parameter string"));
	return NULL;
    }
#ifdef HAVE_PROJ_H
    proj_destroy(pjinfo.pj);
#else
    pj_free(pjinfo.pj);
#endif

    unit = G_find_key_value("unit", proj_units);
    unfact = G_find_key_value("meters", proj_units);
    if (unfact != NULL && (strcmp(pjinfo.proj, "ll") != 0))
	G_asprintf(&proj4mod, "%s +to_meter=%s", proj4, unfact);
    else
	proj4mod = G_store(proj4);

    /* create GDAL OSR from proj string */
    if ((errcode = OSRImportFromProj4(hSRS, proj4mod)) != OGRERR_NONE) {
	G_warning(_("OGR can't parse PROJ.4-style parameter string: "
		    "%s (OGR Error code was %d)"), proj4mod, errcode);
	return NULL;
    }
    G_free(proj4mod);

    /* this messes up PROJCS versus GEOGCS!
    sysname = G_find_key_value("name", proj_info);
    if (sysname)
	OSRSetProjCS(hSRS, sysname);
    */

    if ((errcode = OSRExportToWkt(hSRS, &wkt)) != OGRERR_NONE) {
	G_warning(_("OGR can't get WKT-style parameter string "
		    "(OGR Error code was %d)"), errcode);
	return NULL;
    }

    ellpskv = G_find_key_value("ellps", proj_info);
    GPJ__get_ellipsoid_params(proj_info, &a, &es, &rf);
    haveparams = GPJ__get_datum_params(proj_info, &datum, &params);

    if (ellpskv != NULL)
	ellps = G_store(ellpskv);
    else
	ellps = NULL;

    if ((datum == NULL) || (GPJ_get_datum_by_name(datum, &dstruct) < 0)) {
	datumlongname = G_store("unknown");
	if (ellps == NULL)
	    ellps = G_store("unnamed");
    }
    else {
	datumlongname = G_store(dstruct.longname);
	if (ellps == NULL)
	    ellps = G_store(dstruct.ellps);
	GPJ_free_datum(&dstruct);
    }
    G_debug(3, "GPJ_grass_to_osr: datum: <%s>", datum);
    G_free(datum);
    if (GPJ_get_ellipsoid_by_name(ellps, &estruct) > 0) {
	ellpslong = G_store(estruct.longname);
	DatumNameMassage(&ellpslong);
	GPJ_free_ellps(&estruct);
    }
    else
	ellpslong = G_store(ellps);

    startmod = strstr(wkt, "GEOGCS");
    lastpart = strstr(wkt, "PRIMEM");
    len = strlen(wkt) - strlen(startmod);
    wkt[len] = '\0';
    if (haveparams == 2) {
	/* Only put datum params into the WKT if they were specifically
	 * specified in PROJ_INFO */
	char *paramkey, *paramvalue;

	paramkey = strtok(params, "=");
	paramvalue = params + strlen(paramkey) + 1;
	if (G_strcasecmp(paramkey, "towgs84") == 0)
	    G_asprintf(&towgs84, ",TOWGS84[%s]", paramvalue);
	else
	    towgs84 = G_store("");
	G_free(params);
    }
    else
	towgs84 = G_store("");

    sysname = OSRGetAttrValue(hSRS, "PROJCS", 0);
    if (sysname == NULL) {
	/* Not a projected co-ordinate system */
	start = G_store("");
	end = G_store("");
    }
    else {
	if ((strcmp(sysname, "unnamed") == 0) &&
	    (G_find_key_value("name", proj_info) != NULL))
	    G_asprintf(&start, "PROJCS[\"%s\",",
		       G_find_key_value("name", proj_info));
	else
	    start = G_store(wkt);

	osrunit = OSRGetAttrValue(hSRS, "UNIT", 0);
	osrunfact = OSRGetAttrValue(hSRS, "UNIT", 1);

	if ((unfact == NULL) || (G_strcasecmp(osrunit, "unknown") != 0))
	    end = G_store("");
	else {
	    char *buff;
	    double unfactf = atof(unfact);

	    G_asprintf(&buff, ",UNIT[\"%s\",", osrunit);

	    startmod = strstr(lastpart, buff);
	    len = strlen(lastpart) - strlen(startmod);
	    lastpart[len] = '\0';
	    G_free(buff);

	    if (unit == NULL)
		unit = "unknown";
	    G_asprintf(&end, ",UNIT[\"%s\",%.16g]]", unit, unfactf);
	}

    }
    OSRDestroySpatialReference(hSRS);
    G_asprintf(&modwkt,
	       "%sGEOGCS[\"%s\",DATUM[\"%s\",SPHEROID[\"%s\",%.16g,%.16g]%s],%s%s",
	       start, ellps, datumlongname, ellpslong, a, rf, towgs84,
	       lastpart, end);
    hSRS2 = OSRNewSpatialReference(modwkt);
    G_free(modwkt);

    CPLFree(wkt);
    G_free(start);
    G_free(ellps);
    G_free(datumlongname);
    G_free(ellpslong);
    G_free(towgs84);
    G_free(end);

    return hSRS2;
}

/*!
 * \brief Converts a GRASS co-ordinate system to an
 * OGRSpatialReferenceH object. EPSG code is preferred if available.
 * 
 * The co-ordinate system definition is imported from EPSG (by GDAL)
 * definition if available. TOWGS84 parameter is scanned from
 * PROJ_INFO file and appended to co-ordinate system definition. If
 * EPSG code is not available, PROJ_INFO file is used as
 * GPJ_grass_to_osr() does.

 * \todo Merge with GPJ_grass_to_osr() in GRASS 8.
 *
 * \param proj_info Set of GRASS PROJ_INFO key/value pairs
 * \param proj_units Set of GRASS PROJ_UNIT key/value pairs
 * \param proj_epsg Set of GRASS PROJ_EPSG key/value pairs
 * 
 * \return OGRSpatialReferenceH object representing the co-ordinate system
 *         defined by proj_info and proj_units or NULL if it fails
 */
OGRSpatialReferenceH GPJ_grass_to_osr2(const struct Key_Value * proj_info,
                                       const struct Key_Value * proj_units,
                                       const struct Key_Value * proj_epsg)
{
    int epsgcode = 0;
    
    if (proj_epsg) {
        const char *epsgstr = G_find_key_value("epsg", proj_epsg);
        if (epsgstr)
            epsgcode = atoi(epsgstr);
    }

    if (epsgcode) {
        const char *towgs84;
        OGRSpatialReferenceH hSRS;
        
        hSRS = OSRNewSpatialReference(NULL);

        OSRImportFromEPSG(hSRS, epsgcode);

        /* take +towgs84 from projinfo file if defined */
        towgs84 = G_find_key_value("towgs84", proj_info);
        if (towgs84) {
            char **tokens;
            int i;
            double df[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            
            tokens = G_tokenize(towgs84, ",");

            for (i = 0; i < G_number_of_tokens(tokens); i++)
                df[i] = atof(tokens[i]);
            G_free_tokens(tokens);
            
            OSRSetTOWGS84(hSRS, df[0], df[1], df[2], df[3], df[4], df[5], df[6]);
        }
        
        return hSRS;
    }

    return GPJ_grass_to_osr(proj_info, proj_units);
}

/*!
 * \brief Converts an OGRSpatialReferenceH object to a GRASS co-ordinate system.
 * 
 * \param cellhd      Pointer to a GRASS Cell_head structure that will have its
 *                    projection-related members populated with appropriate values
 * \param projinfo   Pointer to a pointer which will have a GRASS Key_Value 
 *                    structure allocated containing a set of GRASS PROJ_INFO values
 * \param projunits  Pointer to a pointer which will have a GRASS Key_Value 
 *                    structure allocated containing a set of GRASS PROJ_UNITS values
 * \param hSRS        OGRSpatialReferenceH object containing the co-ordinate 
 *                    system to be converted
 * \param datumtrans  Index number of datum parameter set to use, 0 to leave
 *                    unspecified
 * 
 * \return            2 if a projected or lat/long co-ordinate system has been
 *                    defined
 * \return            1 if an unreferenced XY co-ordinate system has
 *                    been defined
 */
int GPJ_osr_to_grass(struct Cell_head *cellhd, struct Key_Value **projinfo,
		     struct Key_Value **projunits, OGRSpatialReferenceH hSRS1,
		     int datumtrans)
{
    struct Key_Value *temp_projinfo, *temp_projinfo_ext;
    char *pszProj4 = NULL, *pszRemaining;
    char *pszProj = NULL;
    const char *pszProjCS = NULL;
    char *datum = NULL;
    char *proj4_unit = NULL;
    struct gpj_datum dstruct;
    const char *ograttr;
    OGRSpatialReferenceH hSRS;
    int use_proj_extension;

    *projinfo = NULL;
    *projunits = NULL;

    hSRS = hSRS1;

    if (hSRS == NULL)
	goto default_to_xy;

    /* Set finder function for locating OGR csv co-ordinate system tables */
    /* SetCSVFilenameHook(GPJ_set_csv_loc); */

    /* Hopefully this doesn't do any harm if it wasn't in ESRI format
     * to start with... */
    OSRMorphFromESRI(hSRS);

    *projinfo = G_create_key_value();
    use_proj_extension = 0;

    /* use proj4 definition from EXTENSION attribute if existing */
    ograttr = OSRGetAttrValue(hSRS, "EXTENSION", 0);
    if (ograttr && *ograttr && strcmp(ograttr, "PROJ4") == 0) {
	ograttr = OSRGetAttrValue(hSRS, "EXTENSION", 1);
	G_debug(3, "proj4 extension:");
	G_debug(3, "%s", ograttr);
	
	if (ograttr && *ograttr) {
	    char *proj4ext;
	    OGRSpatialReferenceH hSRS2;

	    hSRS2 = OSRNewSpatialReference(NULL);
	    proj4ext = G_store(ograttr);

	    /* test */
	    if (OSRImportFromProj4(hSRS2, proj4ext) != OGRERR_NONE) {
		G_warning(_("Updating spatial reference with embedded proj4 definition failed. "
		            "Proj4 definition: <%s>"), proj4ext);
		OSRDestroySpatialReference(hSRS2);
	    }
	    else {
		/* use new OGR spatial reference defined with embedded proj4 string */
		/* TODO: replace warning with important_message once confirmed working */
		G_warning(_("Updating spatial reference with embedded proj4 definition"));

		/* -------------------------------------------------------------------- */
		/*      Derive the user name for the coordinate system.                 */
		/* -------------------------------------------------------------------- */
		pszProjCS = OSRGetAttrValue(hSRS, "PROJCS", 0);
		if (!pszProjCS)
		    pszProjCS = OSRGetAttrValue(hSRS, "GEOGCS", 0);

		if (pszProjCS) {
		    G_set_key_value("name", pszProjCS, *projinfo);
		}
		else if (pszProj) {
		    char path[4095];
		    char name[80];
		    
		    /* use name of the projection as name for the coordinate system */

		    sprintf(path, "%s/etc/proj/projections", G_gisbase());
		    if (G_lookup_key_value_from_file(path, pszProj, name, sizeof(name)) >
			0)
			G_set_key_value("name", name, *projinfo);
		    else
			G_set_key_value("name", pszProj, *projinfo);
		}

		/* the original hSRS1 is left as is, ok? */
		hSRS = hSRS2;
		use_proj_extension = 1;
	    }
	    G_free(proj4ext);
	}
    }

    /* -------------------------------------------------------------------- */
    /*      Set cellhd for well known coordinate systems.                   */
    /* -------------------------------------------------------------------- */
    if (!OSRIsGeographic(hSRS) && !OSRIsProjected(hSRS))
	goto default_to_xy;

    if (cellhd) {
	int bNorth;

	if (OSRIsGeographic(hSRS)) {
	    cellhd->proj = PROJECTION_LL;
	    cellhd->zone = 0;
	}
	else if (OSRGetUTMZone(hSRS, &bNorth) != 0) {
	    cellhd->proj = PROJECTION_UTM;
	    cellhd->zone = OSRGetUTMZone(hSRS, &bNorth);
	    if (!bNorth)
		cellhd->zone *= -1;
	}
	else {
	    cellhd->proj = PROJECTION_OTHER;
	    cellhd->zone = 0;
	}
    }

    /* -------------------------------------------------------------------- */
    /*      Get the coordinate system definition in PROJ.4 format.          */
    /* -------------------------------------------------------------------- */
    if (OSRExportToProj4(hSRS, &pszProj4) != OGRERR_NONE)
	goto default_to_xy;

    /* -------------------------------------------------------------------- */
    /*      Parse the PROJ.4 string into key/value pairs.  Do a bit of      */
    /*      extra work to "GRASSify" the result.                            */
    /* -------------------------------------------------------------------- */
    temp_projinfo = G_create_key_value();
    temp_projinfo_ext = G_create_key_value();

    /* Create "local" copy of proj4 string so we can modify and free it
     * using GRASS functions */
    pszRemaining = G_store(pszProj4);
    CPLFree(pszProj4);
    pszProj4 = pszRemaining;
    while ((pszRemaining = strstr(pszRemaining, "+")) != NULL) {
	char *pszToken, *pszValue;

	pszRemaining++;

	/* Advance pszRemaining to end of this token[=value] pair */
	pszToken = pszRemaining;
	while (*pszRemaining != ' ' && *pszRemaining != '\0')
	    pszRemaining++;

	if (*pszRemaining == ' ') {
	    *pszRemaining = '\0';
	    pszRemaining++;
	}

	/* parse token, and value */
	if (strstr(pszToken, "=") != NULL) {
	    pszValue = strstr(pszToken, "=");
	    *pszValue = '\0';
	    pszValue++;
	}
	else
	    pszValue = "defined";

	/* projection name */
	if (G_strcasecmp(pszToken, "proj") == 0) {
	    /* The ll projection is known as longlat in PROJ.4 */
	    if (G_strcasecmp(pszValue, "longlat") == 0)
		pszValue = "ll";

	    pszProj = pszValue;
	}

	/* Ellipsoid and datum handled separately below */
	if (G_strcasecmp(pszToken, "ellps") == 0
	    || G_strcasecmp(pszToken, "a") == 0
	    || G_strcasecmp(pszToken, "b") == 0
	    || G_strcasecmp(pszToken, "es") == 0
	    || G_strcasecmp(pszToken, "rf") == 0
	    || G_strcasecmp(pszToken, "datum") == 0) {
	    G_set_key_value(pszToken, pszValue, temp_projinfo_ext);
	    continue;
	}

	/* We will handle units separately */
	if (G_strcasecmp(pszToken, "to_meter") == 0)
	    continue;

	if (G_strcasecmp(pszToken, "units") == 0) {
	    proj4_unit = G_store(pszValue);
	    continue;
	}

	G_set_key_value(pszToken, pszValue, temp_projinfo);
    }
    if (!pszProj)
	G_warning(_("No projection name! Projection parameters likely to be meaningless."));

    /* -------------------------------------------------------------------- */
    /*      Derive the user name for the coordinate system.                 */
    /* -------------------------------------------------------------------- */
    if (!G_find_key_value("name", *projinfo)) {
	pszProjCS = OSRGetAttrValue(hSRS, "PROJCS", 0);
	if (!pszProjCS)
	    pszProjCS = OSRGetAttrValue(hSRS, "GEOGCS", 0);

	if (pszProjCS) {
	    G_set_key_value("name", pszProjCS, *projinfo);
	}
	else if (pszProj) {
	    char path[4095];
	    char name[80];
	    
	    /* use name of the projection as name for the coordinate system */

	    sprintf(path, "%s/etc/proj/projections", G_gisbase());
	    if (G_lookup_key_value_from_file(path, pszProj, name, sizeof(name)) >
		0)
		G_set_key_value("name", name, *projinfo);
	    else
		G_set_key_value("name", pszProj, *projinfo);
	}
    }

    /* -------------------------------------------------------------------- */
    /*      Find the GRASS datum name and choose parameters either          */
    /*      interactively or not.                                           */
    /* -------------------------------------------------------------------- */

    {
	const char *pszDatumNameConst;
	struct datum_list *list, *listhead;
	char *dum1, *dum2, *pszDatumName;
	int paramspresent =
	    GPJ__get_datum_params(temp_projinfo, &dum1, &dum2);

	if (!use_proj_extension)
	    pszDatumNameConst = OSRGetAttrValue(hSRS, "DATUM", 0);
	else
	    pszDatumNameConst = G_find_key_value("datum", temp_projinfo_ext);

	if (pszDatumNameConst) {
	    /* Need to make a new copy of the string so we don't mess
	     * around with the memory inside the OGRSpatialReferenceH? */

	    pszDatumName = G_store(pszDatumNameConst);
	    DatumNameMassage(&pszDatumName);
	    G_debug(3, "GPJ_osr_to_grass: pszDatumNameConst: <%s>", pszDatumName);

	    list = listhead = read_datum_table();

	    while (list != NULL) {
		if (G_strcasecmp(pszDatumName, list->longname) == 0) {
		    datum = G_store(list->name);
		    break;
		}
		list = list->next;
	    }
	    free_datum_list(listhead);

	    if (datum == NULL) {
		if (paramspresent < 2)
		    /* Only give warning if no parameters present */
		    G_debug(1, "Datum <%s> not recognised by GRASS and no parameters found",
			      pszDatumName);
	    }
	    else {
		G_set_key_value("datum", datum, *projinfo);

		if (paramspresent < 2) {
		    /* If no datum parameters were imported from the OSR
		     * object then we should use the set specified by datumtrans */
		    char *params, *chosenparams = NULL;
		    int paramsets;

		    paramsets =
			GPJ_get_default_datum_params_by_name(datum, &params);

		    if (paramsets < 0)
			G_debug(1, "Datum <%s> apparently recognised by GRASS but no parameters found. "
				   "You may want to look into this.", datum);
		    else if (datumtrans > paramsets) {

			G_debug(1, "Invalid transformation number %d; valid range is 1 to %d. "
				   "Leaving datum transform parameters unspecified.",
				  datumtrans, paramsets);
			datumtrans = 0;
		    }

		    if (paramsets > 0) {
			struct gpj_datum_transform_list *tlist, *old;

			tlist = GPJ_get_datum_transform_by_name(datum);

			if (tlist != NULL) {
			    do {
				if (tlist->count == datumtrans)
				    chosenparams = G_store(tlist->params);
				old = tlist;
				tlist = tlist->next;
				GPJ_free_datum_transform(old);
			    } while (tlist != NULL);
			}
		    }

		    if (chosenparams != NULL) {
			char *paramkey, *paramvalue;

			paramkey = strtok(chosenparams, "=");
			paramvalue = chosenparams + strlen(paramkey) + 1;
			G_set_key_value(paramkey, paramvalue, *projinfo);
			G_free(chosenparams);
		    }

		    if (paramsets > 0)
			G_free(params);
		}

	    }
	    G_free(pszDatumName);
	}
    }

    /* -------------------------------------------------------------------- */
    /*   Determine an appropriate GRASS ellipsoid name if possible, or      */
    /*   else just put a and es values into PROJ_INFO                       */
    /* -------------------------------------------------------------------- */

    if ((datum != NULL) && (GPJ_get_datum_by_name(datum, &dstruct) > 0)) {
	/* Use ellps name associated with datum */
	G_set_key_value("ellps", dstruct.ellps, *projinfo);
	GPJ_free_datum(&dstruct);
	G_free(datum);
    }
    else if (!use_proj_extension) {
	/* If we can't determine the ellipsoid from the datum, derive it
	 * directly from "SPHEROID" parameters in WKT */
	const char *pszSemiMajor = OSRGetAttrValue(hSRS, "SPHEROID", 1);
	const char *pszInvFlat = OSRGetAttrValue(hSRS, "SPHEROID", 2);

	if (pszSemiMajor != NULL && pszInvFlat != NULL) {
	    char *ellps = NULL;
	    struct ellps_list *list, *listhead;
	    double a = atof(pszSemiMajor), invflat = atof(pszInvFlat), flat;
	    double es;

	    /* Allow for incorrect WKT describing a sphere where InvFlat 
	     * is given as 0 rather than inf */
	    if (invflat > 0)
		flat = 1 / invflat;
	    else
		flat = 0;

	    es = flat * (2.0 - flat);

	    list = listhead = read_ellipsoid_table(0);

	    while (list != NULL) {
		/* Try and match a and es against GRASS defined ellipsoids;
		 * accept first one that matches. These numbers were found
		 * by trial and error and could be fine-tuned, or possibly
		 * a direct comparison of IEEE floating point values used. */
		if ((a == list->a || fabs(a - list->a) < 0.1 || fabs(1 - a / list->a) < 0.0000001) &&
		    ((es == 0 && list->es == 0) ||
		    /* Special case for sphere */
		    (invflat == list->rf || fabs(invflat - list->rf) < 0.0000001))) {
		    ellps = G_store(list->name);
		    break;
		}
		list = list->next;
	    }
	    if (listhead != NULL)
		free_ellps_list(listhead);

	    if (ellps == NULL) {
		/* If we weren't able to find a matching ellps name, set
		 * a and es values directly from WKT-derived data */
		char es_str[100];

		G_set_key_value("a", (char *)pszSemiMajor, *projinfo);

		sprintf(es_str, "%.16g", es);
		G_set_key_value("es", es_str, *projinfo);
	    }
	    else {
		/* else specify the GRASS ellps name for readability */
		G_set_key_value("ellps", ellps, *projinfo);
		G_free(ellps);
	    }

	}

    }
    else if (use_proj_extension) {
	double a, es, rf;

	if (GPJ__get_ellipsoid_params(temp_projinfo_ext, &a, &es, &rf)) {
	    char parmstr[100];

	    sprintf(parmstr, "%.16g", a);
	    G_set_key_value("a", parmstr, *projinfo);
	    sprintf(parmstr, "%.16g", es);
	    G_set_key_value("es", parmstr, *projinfo);
	}
    }

    /* -------------------------------------------------------------------- */
    /*      Finally append the detailed projection parameters to the end    */
    /* -------------------------------------------------------------------- */

    {
	int i;

	for (i = 0; i < temp_projinfo->nitems; i++)
	    G_set_key_value(temp_projinfo->key[i],
			    temp_projinfo->value[i], *projinfo);

	G_free_key_value(temp_projinfo);
    }
    G_free_key_value(temp_projinfo_ext);

    G_free(pszProj4);

    /* -------------------------------------------------------------------- */
    /*      Set the linear units.                                           */
    /* -------------------------------------------------------------------- */
    *projunits = G_create_key_value();

    if (OSRIsGeographic(hSRS)) {
	/* We assume degrees ... someday we will be wrong! */
	G_set_key_value("unit", "degree", *projunits);
	G_set_key_value("units", "degrees", *projunits);
	G_set_key_value("meters", "1.0", *projunits);
    }
    else {
	char szFormatBuf[256];
	char *pszUnitsName = NULL;
	double dfToMeters;
	char *pszUnitsPlural, *pszStringEnd;

	dfToMeters = OSRGetLinearUnits(hSRS, &pszUnitsName);

	/* the unit name can be arbitrary: the following can be the same
	 * us-ft			(proj.4 keyword)
	 * U.S. Surveyor's Foot		(proj.4 name)
	 * US survey foot		(WKT)
	 * Foot_US			(WKT)
	 */

	/* Workaround for the most obvious case when unit name is unknown */
	if ((G_strcasecmp(pszUnitsName, "unknown") == 0) &&
	    (dfToMeters == 1.))
	    G_asprintf(&pszUnitsName, "meter");

	if ((G_strcasecmp(pszUnitsName, "metre") == 0))
	    G_asprintf(&pszUnitsName, "meter");
	if ((G_strcasecmp(pszUnitsName, "kilometre") == 0))
	    G_asprintf(&pszUnitsName, "kilometer");
	
	if (dfToMeters != 1. && proj4_unit) {
	    int i;
	    
	    i = 0;
	    while (gpj_units[i].id != NULL) {
		if (strcmp(proj4_unit, gpj_units[i].id) == 0) {
		    G_asprintf(&pszUnitsName, "%s", gpj_units[i].name);
		    break;
		}
		i++;
	    }
	}

	G_set_key_value("unit", pszUnitsName, *projunits);

	/* Attempt at plural formation (WKT format doesn't store plural
	 * form of unit name) */
	pszUnitsPlural = G_malloc(strlen(pszUnitsName) + 3);
	strcpy(pszUnitsPlural, pszUnitsName);
	pszStringEnd = pszUnitsPlural + strlen(pszUnitsPlural) - 4;
	if (G_strcasecmp(pszStringEnd, "foot") == 0) {
	    /* Special case for foot - change two o's to e's */
	    pszStringEnd[1] = 'e';
	    pszStringEnd[2] = 'e';
	}
	else if (G_strcasecmp(pszStringEnd, "inch") == 0) {
	    /* Special case for inch - add es */
	    pszStringEnd[4] = 'e';
	    pszStringEnd[5] = 's';
	    pszStringEnd[6] = '\0';
	}
	else {
	    /* For anything else add an s at the end */
	    pszStringEnd[4] = 's';
	    pszStringEnd[5] = '\0';
	}

	G_set_key_value("units", pszUnitsPlural, *projunits);
	G_free(pszUnitsPlural);

	sprintf(szFormatBuf, "%.16g", dfToMeters);
	G_set_key_value("meters", szFormatBuf, *projunits);

    }

    if (hSRS != hSRS1)
	OSRDestroySpatialReference(hSRS);

    return 2;

    /* -------------------------------------------------------------------- */
    /*      Fallback to returning an ungeoreferenced definition.            */
    /* -------------------------------------------------------------------- */
  default_to_xy:
    if (cellhd != NULL) {
	cellhd->proj = PROJECTION_XY;
	cellhd->zone = 0;
    }
    if (*projinfo)
	G_free_key_value(*projinfo);

    *projinfo = NULL;
    *projunits = NULL;

    if (hSRS != NULL && hSRS != hSRS1)
	OSRDestroySpatialReference(hSRS);

    return 1;
}
#endif

/*!
 * \brief Converts a WKT projection description to a GRASS co-ordinate system.
 * 
 * \param cellhd      Pointer to a GRASS Cell_head structure that will have its
 *                    projection-related members populated with appropriate values
 * \param projinfo    Pointer to a pointer which will have a GRASS Key_Value 
 *                    structure allocated containing a set of GRASS PROJ_INFO values
 * \param projunits   Pointer to a pointer which will have a GRASS Key_Value 
 *                    structure allocated containing a set of GRASS PROJ_UNITS values
 * \param wkt         Well-known Text (WKT) description of the co-ordinate 
 *                    system to be converted
 * \param datumtrans  Index number of datum parameter set to use, 0 to leave
 *                    unspecified
 * 
 * \return            2 if a projected or lat/long co-ordinate system has been
 *                    defined
 * \return            1 if an unreferenced XY co-ordinate system has
 *                    been defined
 * \return            -1 on error
 */
int GPJ_wkt_to_grass(struct Cell_head *cellhd, struct Key_Value **projinfo,
		     struct Key_Value **projunits, const char *wkt,
		     int datumtrans)
{
#ifdef HAVE_OGR
    int retval;

    if (wkt == NULL)
	retval =
	    GPJ_osr_to_grass(cellhd, projinfo, projunits, NULL, datumtrans);
    else {
	OGRSpatialReferenceH hSRS;

	/* Set finder function for locating OGR csv co-ordinate system tables */
	/* SetCSVFilenameHook(GPJ_set_csv_loc); */

	hSRS = OSRNewSpatialReference(wkt);
	retval =
	    GPJ_osr_to_grass(cellhd, projinfo, projunits, hSRS, datumtrans);
	OSRDestroySpatialReference(hSRS);
    }

    return retval;
#else
    return -1;
#endif
}

#ifdef HAVE_OGR
/* GPJ_set_csv_loc()
 * 'finder function' for use with OGR SetCSVFilenameHook() function */

const char *GPJ_set_csv_loc(const char *name)
{
    const char *gisbase = G_gisbase();
    static char *buf = NULL;

    if (buf != NULL)
	G_free(buf);

    G_asprintf(&buf, "%s%s/%s", gisbase, CSVDIR, name);

    return buf;
}


/* The list below is only for files that use a non-standard name for a 
 * datum that is already supported in GRASS. The number of entries must be even;
 * they are all in pairs. The first one in the pair is the non-standard name;
 * the second is the GRASS/GDAL name. If a name appears more than once (as for
 * European_Terrestrial_Reference_System_1989) then it means there was more
 * than one non-standard name for it that needs to be accounted for. 
 *
 * N.B. The order of these pairs is different from that in 
 * ogr/ogrfromepsg.cpp in the GDAL source tree! GRASS uses the EPSG
 * names in its WKT representation except WGS_1984 and WGS_1972 as
 * these shortened versions seem to be standard.
 * Below order:
 * the equivalent name comes first in the pair, and
 * the EPSG name (as used in the GRASS datum.table file) comes second.
 *
 * The datum parameters are stored in
 *   ../gis/datum.table           # 3 parameters
 *   ../gis/datumtransform.table  # 7 parameters (requires entry in datum.table)
 *
 * Hint: use GDAL's "testepsg" to identify the canonical name, e.g.
 *       testepsg epsg:4674
 */

static const char *papszDatumEquiv[] = {
    "Militar_Geographische_Institute",
    "Militar_Geographische_Institut",
    "World_Geodetic_System_1984",
    "WGS_1984",
    "World_Geodetic_System_1972",
    "WGS_1972",
    "European_Terrestrial_Reference_System_89",
    "European_Terrestrial_Reference_System_1989",
    "European_Reference_System_1989",
    "European_Terrestrial_Reference_System_1989",
    "ETRS_1989",
    "European_Terrestrial_Reference_System_1989",
    "ETRS89",
    "European_Terrestrial_Reference_System_1989",
    "ETRF_1989",
    "European_Terrestrial_Reference_System_1989",
    "NZGD_2000",
    "New_Zealand_Geodetic_Datum_2000",
    "Monte_Mario_Rome",
    "Monte_Mario",
    "MONTROME",
    "Monte_Mario",
    "Campo_Inchauspe_1969",
    "Campo_Inchauspe",
    "S_JTSK",
    "System_Jednotne_Trigonometricke_Site_Katastralni",
    "S_JTSK_Ferro",
    "Militar_Geographische_Institut",
    "Potsdam_Datum_83",
    "Deutsches_Hauptdreiecksnetz",
    "Rauenberg_Datum_83",
    "Deutsches_Hauptdreiecksnetz",
    "South_American_1969",
    "South_American_Datum_1969",
    "International_Terrestrial_Reference_Frame_1992",
    "ITRF92",
    "ITRF_1992",
    "ITRF92",
    NULL
};

/************************************************************************/
/*                      OGREPSGDatumNameMassage()                       */
/*                                                                      */
/*      Massage an EPSG datum name into WMT format.  Also transform     */
/*      specific exception cases into WKT versions.                     */

/************************************************************************/

static void DatumNameMassage(char **ppszDatum)
{
    int i, j;
    char *pszDatum = *ppszDatum;

    G_debug(3, "DatumNameMassage: Raw string found <%s>", (char *)pszDatum);
    /* -------------------------------------------------------------------- */
    /*      Translate non-alphanumeric values to underscores.               */
    /* -------------------------------------------------------------------- */
    for (i = 0; pszDatum[i] != '\0'; i++) {
	if (!(pszDatum[i] >= 'A' && pszDatum[i] <= 'Z')
	    && !(pszDatum[i] >= 'a' && pszDatum[i] <= 'z')
	    && !(pszDatum[i] >= '0' && pszDatum[i] <= '9')) {
	    pszDatum[i] = '_';
	}
    }

    /* -------------------------------------------------------------------- */
    /*      Remove repeated and trailing underscores.                       */
    /* -------------------------------------------------------------------- */
    for (i = 1, j = 0; pszDatum[i] != '\0'; i++) {
	if (pszDatum[j] == '_' && pszDatum[i] == '_')
	    continue;

	pszDatum[++j] = pszDatum[i];
    }
    if (pszDatum[j] == '_')
	pszDatum[j] = '\0';
    else
	pszDatum[j + 1] = '\0';

    /* -------------------------------------------------------------------- */
    /*      Search for datum equivalences.  Specific massaged names get     */
    /*      mapped to OpenGIS specified names.                              */
    /* -------------------------------------------------------------------- */
    G_debug(3, "DatumNameMassage: Search for datum equivalences of <%s>", (char *)pszDatum);
    for (i = 0; papszDatumEquiv[i] != NULL; i += 2) {
	if (EQUAL(*ppszDatum, papszDatumEquiv[i])) {
	    G_free(*ppszDatum);
	    *ppszDatum = G_store(papszDatumEquiv[i + 1]);
	    break;
	}
    }
}

#endif /* HAVE_OGR */
