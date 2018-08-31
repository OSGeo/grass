/*
 ****************************************************************************
 *
 * MODULE:       gis library
 * AUTHOR(S):    Andreas Lange - andreas.lange@rhein-main.de
 *               Paul Kelly - paul-grass@stjohnspoint.co.uk
 * PURPOSE:      provide functions for reading datum parameters from the
 *               location database.     
 * COPYRIGHT:    (C) 2000, 2003 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define DATUMTABLE "/etc/proj/datum.table"

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static struct table
{
    struct datum
    {
	char *name;		/* Short Name / acronym of map datum */
	char *descr;		/* Long Name for map datum */
	char *ellps;		/* acronym for ellipsoid used with this datum */
	double dx;		/* delta x */
	double dy;		/* delta y */
	double dz;		/* delta z */
    } *datums;
    int size;
    int count;
    int initialized;
} table;

static int compare_table_names(const void *, const void *);

int G_get_datum_by_name(const char *name)
{
    int i;

    G_read_datum_table();

    for (i = 0; i < table.count; i++)
	if (G_strcasecmp(name, table.datums[i].name) == 0)
	    return i;

    return -1;
}

const char *G_datum_name(int n)
{
    G_read_datum_table();

    if (n < 0 || n >= table.count)
	return NULL;

    return table.datums[n].name;
}

const char *G_datum_description(int n)
{
    G_read_datum_table();

    if (n < 0 || n >= table.count)
	return NULL;

    return table.datums[n].descr;
}

const char *G_datum_ellipsoid(int n)
{
    G_read_datum_table();

    if (n < 0 || n >= table.count)
	return NULL;

    return table.datums[n].ellps;
}

/***********************************************************
 *  G_get_datumparams_from_projinfo(projinfo, datumname, params)
 *     struct Key_Value *projinfo Set of key_value pairs containing
 *                       projection information in PROJ_INFO file
 *                       format
 *     char *datumname   Pointer into which a string containing
 *                       the datum name (if present) will be
 *                       placed.
 *     char *params      Pointer into which a string containing
 *                       the datum parameters (if present) will
 *                       be placed.
 *
 *  Extract the datum transformation-related parameters from a 
 *  set of general PROJ_INFO parameters.
 *  This function can be used to test if a location set-up 
 *  supports datum transformation.
 *  
 *  returns: -1 error or no datum information found, 
 *           1 only datum name found, 2 params found
 ************************************************************/

int G_get_datumparams_from_projinfo(const struct Key_Value *projinfo,
				    char *datumname, char *params)
{
    int returnval = -1;

    if (NULL != G_find_key_value("datum", projinfo)) {
	sprintf(datumname, "%s", G_find_key_value("datum", projinfo));
	returnval = 1;
    }

    if (G_find_key_value("datumparams", projinfo) != NULL) {
	sprintf(params, "%s", G_find_key_value("datumparams", projinfo));
	returnval = 2;
    }
    else if (G_find_key_value("nadgrids", projinfo) != NULL) {
	sprintf(params, "nadgrids=%s",
		G_find_key_value("nadgrids", projinfo));
	returnval = 2;
    }
    else if (G_find_key_value("towgs84", projinfo) != NULL) {
	sprintf(params, "towgs84=%s", G_find_key_value("towgs84", projinfo));
	returnval = 2;
    }
    else if (G_find_key_value("dx", projinfo) != NULL
	     && G_find_key_value("dy", projinfo) != NULL
	     && G_find_key_value("dz", projinfo) != NULL) {
	sprintf(params, "towgs84=%s,%s,%s",
		G_find_key_value("dx", projinfo),
		G_find_key_value("dy", projinfo),
		G_find_key_value("dz", projinfo));
	returnval = 2;
    }

    return returnval;

}

void G_read_datum_table(void)
{
    FILE *fd;
    char file[GPATH_MAX];
    char buf[1024];
    int line;

    if (G_is_initialized(&table.initialized))
	return;

    sprintf(file, "%s%s", G_gisbase(), DATUMTABLE);

    fd = fopen(file, "r");
    if (!fd) {
	G_warning(_("unable to open datum table file: %s"), file);
	G_initialize_done(&table.initialized);
	return;
    }

    for (line = 1; G_getl2(buf, sizeof(buf), fd); line++) {
	char name[100], descr[100], ellps[100];
	struct datum *t;

	G_strip(buf);
	if (*buf == '\0' || *buf == '#')
	    continue;

	if (table.count >= table.size) {
	    table.size += 50;
	    table.datums = G_realloc(table.datums, table.size * sizeof(struct datum));
	}

	t = &table.datums[table.count];

	if (sscanf(buf, "%s \"%99[^\"]\" %s dx=%lf dy=%lf dz=%lf",
		   name, descr, ellps, &t->dx, &t->dy, &t->dz) != 6) {
	    G_warning(_("error in datum table file, line %d"), line);
	    continue;
	}

	t->name = G_store(name);
	t->descr = G_store(descr);
	t->ellps = G_store(ellps);

	table.count++;
    }

    qsort(table.datums, table.count, sizeof(struct datum), compare_table_names);

    G_initialize_done(&table.initialized);
}

static int compare_table_names(const void *aa, const void *bb)
{
    const struct datum *a = aa;
    const struct datum *b = bb;

    return G_strcasecmp(a->name, b->name);
}
