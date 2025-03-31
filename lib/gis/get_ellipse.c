/*!
   \file lib/gis/get_ellipse.c

   \brief GIS Library - Getting ellipsoid parameters from the database.

   This routine returns the ellipsoid parameters from the database.
   If the PROJECTION_FILE exists in the PERMANENT mapset, read info
   from that file, otherwise return WGS 84 values.

   New 05/2000 by al: for datum shift the f parameter is needed too.
   This all is not a clean design, but it keeps backward-
   compatibility.
   Looks up ellipsoid in ellipsoid table and returns the
   a, e2 and f parameters for the ellipsoid

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author CERL
 */

#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h> /* for sqrt() */
#include <grass/gis.h>
#include <grass/glocale.h>

static const char PERMANENT[] = "PERMANENT";

static struct table {
    struct ellipse {
        char *name;
        char *descr;
        double a;
        double e2;
        double f;
    } *ellipses;
    int count;
    int size;
    int initialized;
} table;

/* static int get_a_e2 (char *, char *, double *,double *); */
static int get_a_e2_f(const char *, const char *, double *, double *, double *);
static int compare_ellipse_names(const void *, const void *);
static int get_ellipsoid_parameters(struct Key_Value *, double *, double *);

/*!
 * \brief get ellipsoid parameters
 *
 * This routine returns the semi-major axis <b>a</b> (in meters) and
 * the eccentricity squared <b>e2</b> for the ellipsoid associated
 * with the database. If there is no ellipsoid explicitly associated
 * with the database, it returns the values for the WGS 84 ellipsoid.
 *
 * \param[out] a semi-major axis
 * \param[out] e2 eccentricity squared
 *
 * \return 1 success
 * \return 0 default values used
 */
int G_get_ellipsoid_parameters(double *a, double *e2)
{
    int stat;
    char ipath[GPATH_MAX];
    struct Key_Value *proj_keys;

    proj_keys = NULL;

    G_file_name(ipath, "", PROJECTION_FILE, PERMANENT);

    if (access(ipath, 0) != 0) {
        *a = 6378137.0;
        *e2 = .006694385;
        return 0;
    }

    proj_keys = G_read_key_value_file(ipath);

    stat = get_ellipsoid_parameters(proj_keys, a, e2);

    G_free_key_value(proj_keys);

    return stat;
}

/*!
 * \brief Get ellipsoid parameters by name
 *
 * This routine returns the semi-major axis <i>a</i> (in meters) and
 * eccentricity squared <i>e2</i> for the named ellipsoid.
 *
 * \param  name ellipsoid name
 * \param[out] a    semi-major axis
 * \param[out] e2   eccentricity squared
 *
 * \return 1 on success
 * \return 0 if ellipsoid not found
 */
int G_get_ellipsoid_by_name(const char *name, double *a, double *e2)
{
    int i;

    G_read_ellipsoid_table(0);

    for (i = 0; i < table.count; i++) {
        if (G_strcasecmp(name, table.ellipses[i].name) == 0) {
            *a = table.ellipses[i].a;
            *e2 = table.ellipses[i].e2;
            return 1;
        }
    }
    return 0;
}

/*!
 * \brief Get ellipsoid name
 *
 * This function returns a pointer to the short name for the
 * <i>n</i><i>th</i> ellipsoid.  If <i>n</i> is less than 0 or greater
 * than the number of known ellipsoids, it returns a NULL pointer.
 *
 * \param n ellipsoid identificator
 *
 * \return ellipsoid name
 * \return NULL if no ellipsoid found
 */
const char *G_ellipsoid_name(int n)
{
    G_read_ellipsoid_table(0);
    return n >= 0 && n < table.count ? table.ellipses[n].name : NULL;
}

/*!
 * \brief Get spheroid parameters by name
 *
 * This function returns the semi-major axis <i>a</i> (in meters), the
 * eccentricity squared <i>e2</i> and the inverse flattening <i>f</i>
 * for the named ellipsoid.
 *
 * \param name spheroid name
 * \param[out] a   semi-major axis
 * \param[out] e2  eccentricity squared
 * \param[out] f   inverse flattening
 *
 * \return 1 on success
 * \return 0 if no spheroid found
 */
int G_get_spheroid_by_name(const char *name, double *a, double *e2, double *f)
{
    int i;

    G_read_ellipsoid_table(0);

    for (i = 0; i < table.count; i++) {
        if (G_strcasecmp(name, table.ellipses[i].name) == 0) {
            *a = table.ellipses[i].a;
            *e2 = table.ellipses[i].e2;
            *f = table.ellipses[i].f;
            return 1;
        }
    }
    return 0;
}

/*!
 * \brief Get description for nth ellipsoid
 *
 * This function returns a pointer to the description text for the
 * <i>n</i>th ellipsoid. If <i>n</i> is less than 0 or greater
 * than the number of known ellipsoids, it returns a NULL pointer.
 *
 * \param n ellipsoid identificator
 *
 * \return pointer to ellipsoid description
 * \return NULL if no ellipsoid found
 */
const char *G_ellipsoid_description(int n)
{
    G_read_ellipsoid_table(0);
    return n >= 0 && n < table.count ? table.ellipses[n].descr : NULL;
}

static int get_a_e2_f(const char *s1, const char *s2, double *a, double *e2,
                      double *f)
{
    double b, recipf;

    if (sscanf(s1, "a=%lf", a) != 1)
        return 0;

    if (*a <= 0.0)
        return 0;

    if (sscanf(s2, "e=%lf", e2) == 1) {
        *f = (double)1.0 / -sqrt(((double)1.0 - *e2)) + (double)1.0;
        return (*e2 >= 0.0);
    }

    if (sscanf(s2, "f=1/%lf", f) == 1) {
        if (*f <= 0.0)
            return 0;
        recipf = (double)1.0 / (*f);
        *e2 = recipf + recipf - recipf * recipf;
        return (*e2 >= 0.0);
    }

    if (sscanf(s2, "b=%lf", &b) == 1) {
        if (b <= 0.0)
            return 0;
        if (b == *a) {
            *f = 0.0;
            *e2 = 0.0;
        }
        else {
            recipf = ((*a) - b) / (*a);
            *f = (double)1.0 / recipf;
            *e2 = recipf + recipf - recipf * recipf;
        }
        return (*e2 >= 0.0);
    }
    return 0;
}

static int compare_ellipse_names(const void *pa, const void *pb)
{
    const struct ellipse *a = pa;
    const struct ellipse *b = pb;

    return G_strcasecmp(a->name, b->name);
}

/*!
   \brief Read ellipsoid table

   \param fatal non-zero value for G_fatal_error(), otherwise
   G_warning() is used

   \return 1 on success
   \return 0 on error
 */
int G_read_ellipsoid_table(int fatal)
{
    FILE *fd;
    char file[GPATH_MAX];
    char buf[1024];
    char badlines[256];
    int line;
    int err;

    if (G_is_initialized(&table.initialized))
        return 1;

    sprintf(file, "%s/etc/proj/ellipse.table", G_gisbase());
    fd = fopen(file, "r");

    if (fd == NULL) {
        (fatal ? G_fatal_error : G_warning)(
            _("Unable to open ellipsoid table file <%s>"), file);
        G_initialize_done(&table.initialized);
        return 0;
    }

    err = 0;
    *badlines = 0;
    for (line = 1; G_getl2(buf, sizeof buf, fd); line++) {
        char name[100], descr[100], buf1[100], buf2[100];
        struct ellipse *e;

        G_strip(buf);
        if (*buf == 0 || *buf == '#')
            continue;

        if (sscanf(buf, "%s  \"%99[^\"]\" %s %s", name, descr, buf1, buf2) !=
            4) {
            err++;
            sprintf(buf, " %d", line);
            if (*badlines)
                G_strlcat(badlines, ",", sizeof(badlines));
            G_strlcat(badlines, buf, sizeof(badlines));
            continue;
        }

        if (table.count >= table.size) {
            table.size += 60;
            table.ellipses =
                G_realloc(table.ellipses, table.size * sizeof(struct ellipse));
        }

        e = &table.ellipses[table.count];

        e->name = G_store(name);
        e->descr = G_store(descr);

        if (get_a_e2_f(buf1, buf2, &e->a, &e->e2, &e->f) ||
            get_a_e2_f(buf2, buf1, &e->a, &e->e2, &e->f))
            table.count++;
        else {
            err++;
            sprintf(buf, " %d", line);
            if (*badlines)
                G_strlcat(badlines, ",", sizeof(badlines));
            G_strlcat(badlines, buf, sizeof(badlines));
            continue;
        }
    }

    fclose(fd);

    if (!err) {
        /* over correct typed version */
        qsort(table.ellipses, table.count, sizeof(struct ellipse),
              compare_ellipse_names);
        G_initialize_done(&table.initialized);
        return 1;
    }

    (fatal ? G_fatal_error : G_warning)(
        n_(("Line%s of ellipsoid table file <%s> is invalid"),
           ("Lines%s of ellipsoid table file <%s> are invalid"), err),
        badlines, file);

    G_initialize_done(&table.initialized);

    return 0;
}

static int get_ellipsoid_parameters(struct Key_Value *proj_keys, double *a,
                                    double *e2)
{
    const char *str, *str1;

    if (!proj_keys) {
        return -1;
    }

    if ((str = G_find_key_value("ellps", proj_keys)) != NULL) {
        if (strncmp(str, "sphere", 6) == 0) {
            str = G_find_key_value("a", proj_keys);
            if (str != NULL) {
                if (sscanf(str, "%lf", a) != 1)
                    G_fatal_error(_("Invalid a: field '%s' in file %s in <%s>"),
                                  str, PROJECTION_FILE, PERMANENT);
            }
            else
                *a = 6370997.0;

            *e2 = 0.0;

            return 0;
        }
        else {
            if (G_get_ellipsoid_by_name(str, a, e2) == 0)
                G_fatal_error(_("Invalid ellipsoid '%s' in file %s in <%s>"),
                              str, PROJECTION_FILE, PERMANENT);
            else
                return 1;
        }
    }
    else {
        str = G_find_key_value("a", proj_keys);
        str1 = G_find_key_value("es", proj_keys);
        if ((str != NULL) && (str1 != NULL)) {
            if (sscanf(str, "%lf", a) != 1)
                G_fatal_error(_("Invalid a: field '%s' in file %s in <%s>"),
                              str, PROJECTION_FILE, PERMANENT);
            if (sscanf(str1, "%lf", e2) != 1)
                G_fatal_error(_("Invalid es: field '%s' in file %s in <%s>"),
                              str, PROJECTION_FILE, PERMANENT);

            return 1;
        }
        else {
            str = G_find_key_value("proj", proj_keys);
            if ((str == NULL) || (strcmp(str, "ll") == 0)) {
                *a = 6378137.0;
                *e2 = .006694385;
                return 0;
            }
            else
                G_fatal_error(_("No ellipsoid info given in file %s in <%s>"),
                              PROJECTION_FILE, PERMANENT);
        }
    }

    return 1;
}
