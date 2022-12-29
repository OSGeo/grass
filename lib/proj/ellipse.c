/*!
   \file lib/proj/ellipse.c

   \brief GProj library - Functions for reading datum parameters from the location database

   \author Paul Kelly <paul-grass stjohnspoint.co.uk>

   (C) 2003-2008 by the GRASS Development Team
 
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
*/

#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>		/* for sqrt() */
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>
#include "local_proto.h"

static int get_a_e2_rf(const char *, const char *, double *, double *,
		       double *);
  
/*!
 * \brief Get the ellipsoid parameters from the database.
 *
 * If the PROJECTION_FILE exists in the PERMANENT mapset, read info from
 * that file, otherwise return WGS 84 values.
 *
 * Dies with diagnostic if there is an error.
 *
 * \param[out] a semi-major axis 
 * \param[out] e2 first eccentricity squared
 * \param[out] rf reciprocal of the ellipsoid flattening term
 *
 * \return 1 on success
 * \return 0 default values used.
 */
int GPJ_get_ellipsoid_params(double *a, double *e2, double *rf)
{
    int ret;
    struct Key_Value *proj_keys = G_get_projinfo();

    if (proj_keys == NULL)
	proj_keys = G_create_key_value();

    ret = GPJ__get_ellipsoid_params(proj_keys, a, e2, rf);
    G_free_key_value(proj_keys);

    return ret;
}

/*!
 * \brief Get the ellipsoid parameters from proj keys structure.
 *
 * If the PROJECTION_FILE exists in the PERMANENT mapset, read info from
 * that file, otherwise return WGS 84 values.
 *
 * Dies with diagnostic if there is an error.
 *
 * \param proj_keys proj definition
 * \param[out] a semi-major axis 
 * \param[out] e2 first eccentricity squared
 * \param[out] rf reciprocal of the ellipsoid flattening term
 *
 * \return 1 on success
 * \return 0 default values used.
 */
int GPJ__get_ellipsoid_params(const struct Key_Value *proj_keys,
                              double *a, double *e2, double *rf)
{
    struct gpj_ellps estruct;
    struct gpj_datum dstruct;
    const char *str, *str3;
    char *str1, *ellps;

    str = G_find_key_value("datum", proj_keys);

    if ((str != NULL) && (GPJ_get_datum_by_name(str, &dstruct) > 0)) {
	/* If 'datum' key is present, look up correct ellipsoid
	 * from datum.table */

	ellps = G_store(dstruct.ellps);
	GPJ_free_datum(&dstruct);

    }
    else
	/* else use ellipsoid defined in PROJ_INFO */
	ellps = G_store(G_find_key_value("ellps", proj_keys));

    if (ellps != NULL && *ellps) {
	if (GPJ_get_ellipsoid_by_name(ellps, &estruct) < 0)
	    G_fatal_error(_("Invalid ellipsoid <%s> in file"), ellps);

	*a = estruct.a;
	*e2 = estruct.es;
	*rf = estruct.rf;
	GPJ_free_ellps(&estruct);
	G_free(ellps);

	return 1;
    }
    else {
	if (ellps)    /* *ellps = '\0' */
	    G_free(ellps);

	str3 = G_find_key_value("a", proj_keys);
	if (str3 != NULL) {
	    char *str4;
	    G_asprintf(&str4, "a=%s", str3);
	    if ((str3 = G_find_key_value("es", proj_keys)) != NULL)
		G_asprintf(&str1, "e=%s", str3);
	    else if ((str3 = G_find_key_value("f", proj_keys)) != NULL)
		G_asprintf(&str1, "f=1/%s", str3);
	    else if ((str3 = G_find_key_value("rf", proj_keys)) != NULL)
		G_asprintf(&str1, "f=1/%s", str3);
	    else if ((str3 = G_find_key_value("b", proj_keys)) != NULL)
		G_asprintf(&str1, "b=%s", str3);
	    else
		G_fatal_error(_("No secondary ellipsoid descriptor "
				"(rf, es or b) in file"));

	    if (get_a_e2_rf(str4, str1, a, e2, rf) == 0)
		G_fatal_error(_("Invalid ellipsoid descriptors "
				"(a, rf, es or b) in file"));
	    return 1;
	}
	else {
	    str = G_find_key_value("proj", proj_keys);
	    if ((str == NULL) || (strcmp(str, "ll") == 0)) {
		*a = 6378137.0;
		*e2 = .006694385;
		*rf = 298.257223563;
		return 0;
	    }
	    else {
		G_fatal_error(_("No ellipsoid info given in file"));
	    }
	}
    }
    return 1;
}


/*!
 * \brief Looks up ellipsoid in ellipsoid table and returns the a, e2
 * parameters for the ellipsoid.
 *
 * \param name ellipsoid name
 * \param[out] estruct ellipsoid
 *
 * \return 1 on success
 * \return -1 if not found in table
 */

int GPJ_get_ellipsoid_by_name(const char *name, struct gpj_ellps *estruct)
{
    struct ellps_list *list, *listhead;

    list = listhead = read_ellipsoid_table(0);

    while (list != NULL) {
	if (G_strcasecmp(name, list->name) == 0) {
	    estruct->name = G_store(list->name);
	    estruct->longname = G_store(list->longname);
	    estruct->a = list->a;
	    estruct->es = list->es;
	    estruct->rf = list->rf;
	    free_ellps_list(listhead);
	    return 1;
	}
	list = list->next;
    }
    free_ellps_list(listhead);
    return -1;
}

int get_a_e2_rf(const char *s1, const char *s2, double *a, double *e2,
                double *recipf)
{
    double b, f;

    if (sscanf(s1, "a=%lf", a) != 1)
	return 0;

    if (*a <= 0.0)
	return 0;

    if (sscanf(s2, "e=%lf", e2) == 1) {
	f = 1.0 - sqrt(1.0 - *e2);
	*recipf = 1.0 / f;
	return (*e2 >= 0.0);
    }

    if (sscanf(s2, "f=1/%lf", recipf) == 1) {
	if (*recipf <= 0.0)
	    return 0;
	f = 1.0 / *recipf;
	*e2 = f * (2 - f);
	return (*e2 >= 0.0);
    }

    if (sscanf(s2, "b=%lf", &b) == 1) {
	if (b <= 0.0)
	    return 0;
	if (b == *a) {
	    f = 0.0;
	    *e2 = 0.0;
	}
	else {
	    f = (*a - b) / *a;
	    *e2 = f * (2 - f);
	}
	*recipf = 1.0 / f;
	return (*e2 >= 0.0);
    }
    return 0;
}

struct ellps_list *read_ellipsoid_table(int fatal)
{
    FILE *fd;
    char file[GPATH_MAX];
    char buf[4096];
    char name[100], descr[1024], buf1[1024], buf2[1024];
    char badlines[1024];
    int line;
    int err;
    struct ellps_list *current = NULL, *outputlist = NULL;
    double a, e2, rf;

    int count = 0;

    sprintf(file, "%s%s", G_gisbase(), ELLIPSOIDTABLE);
    fd = fopen(file, "r");

    if (!fd) {
	(fatal ? G_fatal_error : G_warning)(
	    _("Unable to open ellipsoid table file <%s>"), file);
	return NULL;
    }

    err = 0;
    *badlines = 0;
    for (line = 1; G_getl2(buf, sizeof buf, fd); line++) {
	G_strip(buf);
	if (*buf == 0 || *buf == '#')
	    continue;

	if (sscanf(buf, "%s  \"%1023[^\"]\" %s %s", name, descr, buf1, buf2)
	    != 4) {
	    err++;
	    sprintf(buf, " %d", line);
	    if (*badlines)
		strcat(badlines, ",");
	    strcat(badlines, buf);
	    continue;
	}


	if (get_a_e2_rf(buf1, buf2, &a, &e2, &rf)
	    || get_a_e2_rf(buf2, buf1, &a, &e2, &rf)) {
	    if (current == NULL)
		current = outputlist = G_malloc(sizeof(struct ellps_list));
	    else
		current = current->next = G_malloc(sizeof(struct ellps_list));
	    current->name = G_store(name);
	    current->longname = G_store(descr);
	    current->a = a;
	    current->es = e2;
	    current->rf = rf;
	    current->next = NULL;
	    count++;
	}
	else {
	    err++;
	    sprintf(buf, " %d", line);
	    if (*badlines)
		strcat(badlines, ",");
	    strcat(badlines, buf);
	    continue;
	}
    }

    fclose(fd);

    if (!err)
	return outputlist;

    (fatal ? G_fatal_error : G_warning)(
	n_(
        ("Line%s of ellipsoid table file <%s> is invalid"),
	("Lines%s of ellipsoid table file <%s> are invalid"),
        err), 
	badlines, file);

    return outputlist;
}

/*!
  \brief Free ellipsoid data structure.

  \param estruct data structure to be freed
*/
void GPJ_free_ellps(struct gpj_ellps *estruct)
{
    G_free(estruct->name);
    G_free(estruct->longname);
    return;
}

void free_ellps_list(struct ellps_list *elist)
{
    struct ellps_list *old;

    while (elist != NULL) {
	G_free(elist->name);
	G_free(elist->longname);
	old = elist;
	elist = old->next;
	G_free(old);
    }

    return;
}
