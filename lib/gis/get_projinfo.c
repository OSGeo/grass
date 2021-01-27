/*!
  \file lib/gis/get_projinfo.c

  \brief GIS Library - Get projection info

  (C) 1999-2014 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
*/

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define PERMANENT "PERMANENT"

/*!
  \brief Gets units information for location

  Note: Allocated Key_Value structure should be freed by
  G_free_key_value().

  Prints a warning if no units information available.

  \return pointer to Key_Value structure with key/value pairs
  \return NULL on failure
*/
struct Key_Value *G_get_projunits(void)
{
    struct Key_Value *in_units_keys;
    char path[GPATH_MAX];

    G_file_name(path, "", UNIT_FILE, PERMANENT);
    if (access(path, 0) != 0) {
	if (G_projection() != PROJECTION_XY) {
	    G_warning(_("<%s> file not found for location <%s>"),
		      UNIT_FILE, G_location());
	}
	return NULL;
    }
    in_units_keys = G_read_key_value_file(path);

    return in_units_keys;
}

/*!
  \brief Gets projection information for location

  Note: Allocated Key_Value structure should be freed by
  G_free_key_value().

  Prints a warning if no projection information available.

  \return pointer to Key_Value structure with key/value pairs
  \return NULL on failure
*/
struct Key_Value *G_get_projinfo(void)
{
    struct Key_Value *in_proj_keys, *in_epsg_keys;
    char path[GPATH_MAX];

    G_file_name(path, "", PROJECTION_FILE, PERMANENT);
    if (access(path, 0) != 0) {
	if (G_projection() != PROJECTION_XY) {
	    G_warning(_("<%s> file not found for location <%s>"),
		      PROJECTION_FILE, G_location());
	}
	return NULL;
    }
    in_proj_keys = G_read_key_value_file(path);

    /* TODO: do not restrict to EPSG as the only authority */
    if ((in_epsg_keys = G_get_projepsg()) != NULL) {
	const char *epsgstr = G_find_key_value("epsg", in_epsg_keys);
	char buf[4096];

	sprintf(buf, "EPSG:%s", epsgstr);
	G_set_key_value("init", buf, in_proj_keys);
	G_free_key_value(in_epsg_keys);
    }

    return in_proj_keys;
}

/*!
  \brief Gets EPSG information for the current location

  DEPRECATED: Use G_get_projsrid() instead.

  Note: Allocated Key_Value structure should be freed by
  G_free_key_value().

  \return pointer to Key_Value structure with key/value pairs
  \return NULL when EPSG code is not defined for location
*/

/* superseded by G_get_projsrid(), keep for backwards compatibility */
struct Key_Value *G_get_projepsg(void)
{
    struct Key_Value *in_epsg_keys;
    char path[GPATH_MAX];

    G_file_name(path, "", EPSG_FILE, PERMANENT);
    if (access(path, 0) != 0) {
	if (G_projection() != PROJECTION_XY) {
            G_debug(1, "<%s> file not found for location <%s>",
                    EPSG_FILE, G_location());
	}
	return NULL;
    }
    in_epsg_keys = G_read_key_value_file(path);

    return in_epsg_keys;
}

/*!
  \brief Get WKT information for the current location

  \return pointer to WKT string
  \return NULL when WKT is not available for the current location
*/

char *G_get_projwkt(void)
{
    char *wktstring = NULL;
    char path[GPATH_MAX];
    FILE *fp;
    int n, nalloc;
    int c;

    G_file_name(path, "", WKT_FILE, "PERMANENT");
    if (access(path, 0) != 0) {
	if (G_projection() != PROJECTION_XY) {
	    G_debug(1, "<%s> file not found for location <%s>",
		      WKT_FILE, G_location());
	}
	return NULL;
    }

    fp = fopen(path, "r");
    if (!fp)
	G_fatal_error(_("Unable to open input file <%s>: %s"), path, strerror(errno));

    wktstring = G_malloc(1024 * sizeof(char));
    nalloc = 1024;

    n = 0;
    while (1) {
	c = fgetc(fp);

	if (c == EOF) {
	    break;
	}

	if (c == '\r') {	/* DOS or MacOS9 */
	    c = fgetc(fp);
	    if (c != EOF) {
		if (c != '\n') {	/* MacOS9 - we have to return the char to stream */
		    ungetc(c, fp);
		    c = '\n';
		}
	    }
	    else {	/* MacOS9 - we have to return the char to stream */
		ungetc(c, fp);
		c = '\n';
	    }
	}

	if (n == nalloc) {
	    wktstring = G_realloc(wktstring, nalloc + 1024);
	    nalloc += 1024;
	}

	wktstring[n] = c;

	n++;
    }

    if (n > 0) {
	if (n == nalloc) {
	    wktstring = G_realloc(wktstring, nalloc + 1);
	    nalloc += 1;
	}
	wktstring[n] = '\0';
    }
    else {
	G_free(wktstring);
	wktstring = NULL;
    }

    if (fclose(fp) != 0)
	G_fatal_error(_("Error closing output file <%s>: %s"), path, strerror(errno));

    return G_chop(wktstring);
}

/*!
  \brief Get srid (spatial reference id) for the current location

  Typically an srid will be of the form authority NAME:CODE,
  e.g. EPSG:4326

  This srid is passed to proj_create() using PROJ or
  OSRSetFromUserInput() using GDAL. Therefore various other forms of
  srid are possible, e.g. in OSRSetFromUserInput():

   1. Well Known Text definition - passed on to importFromWkt().
   2. "EPSG:n" - number passed on to importFromEPSG().
   3. "EPSGA:n" - number passed on to importFromEPSGA().
   4. "AUTO:proj_id,unit_id,lon0,lat0" - WMS auto projections.
   5. "urn:ogc:def:crs:EPSG::n" - ogc urns
   6. PROJ.4 definitions - passed on to importFromProj4().
   7. filename - file read for WKT, XML or PROJ.4 definition.
   8. well known name accepted by SetWellKnownGeogCS(), such as NAD27, NAD83, WGS84 or WGS72.
   9. "IGNF:xxxx", "ESRI:xxxx", etc. from definitions from the PROJ database;
  10. PROJJSON (PROJ >= 6.2)

  \return pointer to srid string
  \return NULL when srid is not available for the current location
*/

char *G_get_projsrid(void)
{
    char *sridstring = NULL;
    char path[GPATH_MAX];
    FILE *fp;
    int n, nalloc;
    int c;

    G_file_name(path, "", SRID_FILE, "PERMANENT");
    if (access(path, 0) != 0) {
	if (G_projection() != PROJECTION_XY) {
	    struct Key_Value *projepsg;
	    const char *epsg_num;

	    G_debug(1, "<%s> file not found for location <%s>",
		      SRID_FILE, G_location());

	    /* for backwards compatibility, check if PROJ_EPSG exists */
	    if ((projepsg = G_get_projepsg()) != NULL) {
		epsg_num = G_find_key_value("epsg", projepsg);
		if (*epsg_num) {
		    G_debug(1, "Using <%s> file instead for location <%s>",
			    EPSG_FILE, G_location());
		    G_asprintf(&sridstring, "EPSG:%s", epsg_num);
		    G_free_key_value(projepsg);

		    return sridstring;
		}
	    }
	}
	return NULL;
    }

    fp = fopen(path, "r");
    if (!fp)
	G_fatal_error(_("Unable to open input file <%s>: %s"), path, strerror(errno));

    sridstring = G_malloc(1024 * sizeof(char));
    nalloc = 1024;

    n = 0;
    while (1) {
	c = fgetc(fp);

	if (c == EOF) {
	    break;
	}

	if (c == '\r') {	/* DOS or MacOS9 */
	    c = fgetc(fp);
	    if (c != EOF) {
		if (c != '\n') {	/* MacOS9 - we have to return the char to stream */
		    ungetc(c, fp);
		    c = '\n';
		}
	    }
	    else {	/* MacOS9 - we have to return the char to stream */
		ungetc(c, fp);
		c = '\n';
	    }
	}

	if (n == nalloc) {
	    sridstring = G_realloc(sridstring, nalloc + 1024);
	    nalloc += 1024;
	}

	sridstring[n] = c;

	n++;
    }

    if (n > 0) {
	if (n == nalloc) {
	    sridstring = G_realloc(sridstring, nalloc + 1);
	    nalloc += 1;
	}
	sridstring[n] = '\0';
    }
    else {
	G_free(sridstring);
	sridstring = NULL;
    }

    if (fclose(fp) != 0)
	G_fatal_error(_("Error closing output file <%s>: %s"), path, strerror(errno));

    return G_chop(sridstring);
}
