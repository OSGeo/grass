/*!
 * \file lib/raster/reclass.c
 *
 * \brief Raster Library - Check if raster map is reclassified
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <string.h>
#include <stdbool.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static const char NULL_STRING[] = "null";
static int reclass_type(FILE *, char **, char **, char **);
static FILE *fopen_cellhd_old(const char *, const char *);
static FILE *fopen_cellhd_new(const char *);
static int get_reclass_table(FILE *, struct Reclass *, char **);

/*!
 * \brief Check if raster map is reclassified
 *
 * This function determines if the raster map <i>name</i> in
 * <i>mapset</i> is a reclass file. If it is, then the name and mapset
 * of the referenced raster map are copied into the <i>rname</i> and
 * <i>rmapset</i> buffers.
 *
 * \param name map name
 * \param mapset mapset name
 * \param[out] rname name of reference map
 * \param[out] rmapset mapset where reference map lives
 *
 * \returns 1 if it is a reclass file
 * \return 0 if it is not
 * \return -1 if there was a problem reading the raster header
 */
int Rast_is_reclass(const char *name, const char *mapset, char rname[GNAME_MAX],
                    char rmapset[GMAPSET_MAX])
{
    FILE *fd;
    int type;

    fd = fopen_cellhd_old(name, mapset);
    if (fd == NULL)
        return -1;

    type = reclass_type(fd, &rname, &rmapset, NULL);
    fclose(fd);
    if (type < 0)
        return -1;
    else
        return type != 0;
}

/*!
 * \brief Get child reclass maps list
 *
 * This function generates a child reclass maps list from the
 * cell_misc/reclassed_to file which stores this list. The
 * cell_misc/reclassed_to file is written by Rast_put_reclass().
 * Rast_is_reclassed_to() is used by <tt>g.rename</tt>, <tt>g.remove</tt>
 * and <tt>r.reclass</tt> to prevent accidentally deleting the parent
 * map of a reclassed raster map.
 *
 * \param name map name
 * \param mapset mapset name
 * \param[out] nrmaps number of reference maps
 * \param[out] rmaps array of names of reference maps
 *
 * \return number of reference maps
 * \return -1 on error
 */
int Rast_is_reclassed_to(const char *name, const char *mapset, int *nrmaps,
                         char ***rmaps)
{
    FILE *fd;
    int i, j, k, l;
    char buf2[256], buf3[256];

    fd = G_fopen_old_misc("cell_misc", "reclassed_to", name, mapset);

    if (fd == NULL) {
        return -1;
    }

    if (rmaps)
        *rmaps = NULL;
    for (i = 0; !feof(fd) && fgets(buf2, 255, fd);) {
        l = strlen(buf2);
        for (j = 0, k = 0; j < l; j++) {
            if (buf2[j] == '#' ||
                ((buf2[j] == ' ' || buf2[j] == '\t' || buf2[j] == '\n') && k))
                break;
            else if (buf2[j] != ' ' && buf2[j] != '\t')
                buf3[k++] = buf2[j];
        }

        if (k) {
            buf3[k] = 0;
            i++;
            if (rmaps) {
                *rmaps = (char **)G_realloc(*rmaps, i * sizeof(char *));
                (*rmaps)[i - 1] = (char *)G_malloc(k + 1);
                strncpy((*rmaps)[i - 1], buf3, k);
                (*rmaps)[i - 1][k] = 0;
            }
        }
    }

    if (nrmaps)
        *nrmaps = i;

    if (i && rmaps) {
        i++;
        *rmaps = (char **)G_realloc(*rmaps, i * sizeof(char *));
        (*rmaps)[i - 1] = NULL;
    }

    fclose(fd);

    return i;
}

/*!
   \brief Get reclass

   \param name map name
   \param mapset mapset name
   \param[out] reclass pointer to Reclass structure

   \return type code (>=1), 0 if no reclass, -1 on error
 */
int Rast_get_reclass(const char *name, const char *mapset,
                     struct Reclass *reclass)
{
    FILE *fd;
    int stat;
    char rname[GNAME_MAX] = {0}, rmapset[GMAPSET_MAX] = {0};
    char *tmp_name = rname, *tmp_mapset = rmapset;

    fd = fopen_cellhd_old(name, mapset);
    if (fd == NULL)
        return -1;
    char *error_message = NULL;
    reclass->type = reclass_type(fd, &tmp_name, &tmp_mapset, &error_message);
    reclass->name = G_store(tmp_name);
    reclass->mapset = G_store(tmp_mapset);
    if (reclass->type == 0) {
        // no reclass
        fclose(fd);
        return reclass->type;
    }
    if (reclass->type < 0) {
        // error
        fclose(fd);
        G_warning(_("Error reading beginning of header file for <%s@%s>: %s"),
                  name, mapset, error_message);
        if (error_message != NULL)
            G_free(error_message);
        return reclass->type;
    }

    switch (reclass->type) {
    case RECLASS_TABLE:
        stat = get_reclass_table(fd, reclass, &error_message);
        break;
    default:
        stat = -1;
    }

    fclose(fd);
    if (stat < 0) {
        if (stat == -2)
            G_warning(_("Too many reclass categories for <%s@%s>"), name,
                      mapset);
        else
            G_warning(
                _("Illegal reclass format in header file for <%s@%s>: %s"),
                name, mapset, error_message);
        stat = -1;
    }
    if (error_message != NULL)
        G_free(error_message);
    return stat;
}

/*!
   \brief Free Reclass structure

   \param reclass pointer to Reclass structure
 */
void Rast_free_reclass(struct Reclass *reclass)
{
    switch (reclass->type) {
    case RECLASS_TABLE:
        if (reclass->num > 0)
            G_free(reclass->table);
        reclass->num = 0;
        if (reclass->name)
            G_free(reclass->name);
        if (reclass->mapset)
            G_free(reclass->mapset);
        reclass->name = NULL;
        reclass->mapset = NULL;
        break;
    default:
        break;
    }
}

/**
 * \brief Get reclass type if it is a reclass file
 *
 * \param fd[in] file descriptor
 * \param rname[out] name of the reclass from raster
 * \param rmapset[out] name of the mapset of the raster
 * \param error_message[out] will be assigned a newly error message if not NULL
 *
 * \returns RECLASS_TABLE if reclass, 0 if not, -1 on error
 */
static int reclass_type(FILE *fd, char **rname, char **rmapset,
                        char **error_message)
{
    char
        buf[GNAME_MAX + 128 + 1]; // name or mapset plus the label and separator
    char label[128], arg[GNAME_MAX];
    int i;
    int type;

    /* Check to see if this is a reclass file */
    if (fgets(buf, sizeof(buf), fd) == NULL)
        return 0;
    if (strncmp(buf, "reclas", 6))
        return 0;
    /* later may add other types of reclass */
    type = RECLASS_TABLE;

    /* Read the mapset and file name of the REAL cell file */
    if (*rname)
        **rname = '\0';
    if (*rmapset)
        **rmapset = '\0';
    for (i = 0; i < 2; i++) {
        if (fgets(buf, sizeof buf, fd) == NULL) {
            if (error_message != NULL) {
                G_asprintf(error_message, _("File too short, reading line %d"),
                           i + 1);
            }
            return -1;
        }
        if (buf[strlen(buf) - 1] != '\n') {
            if (error_message != NULL) {
                G_asprintf(error_message, _("Line too long: %s..."), buf);
            }
            return -1;
        }
        if (sscanf(buf, "%[^:]:%s", label, arg) != 2) {
            if (error_message != NULL) {
                G_asprintf(error_message, _("Format is not key:value: %s"),
                           buf);
            }
            return -1;
        }
        if (strncmp(label, "maps", 4) == 0 && *rmapset) {
            G_strlcpy(*rmapset, arg, GMAPSET_MAX);
        }
        else if (strncmp(label, "name", 4) == 0 && *rname) {
            G_strlcpy(*rname, arg, GNAME_MAX);
        }
        else {
            if (error_message != NULL) {
                G_asprintf(error_message, _("Unknown key at line: %s"), buf);
            }
            return -1;
        }
    }
    if ((*rmapset && **rmapset) || (*rname && **rname))
        return type;
    else {
        // If they do not occur in the two lines we expect them.
        if (**rname && error_message != NULL) {
            G_asprintf(error_message,
                       _("Mapset not read, only raster name: %s"), *rname);
        }
        else if (**rmapset && error_message != NULL) {
            G_asprintf(error_message,
                       _("Raster name not read, only mapset: %s"), *rmapset);
        }
        else if (error_message != NULL) {
            *error_message = G_store(_("Raster name and mapset not read"));
        }
        return -1;
    }
}

static FILE *fopen_cellhd_old(const char *name, const char *mapset)
{
    return G_fopen_old("cellhd", name, mapset);
}

/*!
   \brief Put reclass

   \param name map name
   \param reclass pointer to Reclass structure

   \return -1 on error
   \return 1 on success
 */
int Rast_put_reclass(const char *name, const struct Reclass *reclass)
{
    FILE *fd;
    long min, max;
    int found;
    char buf1[GPATH_MAX], buf2[GNAME_MAX], *p;
    char *xname;

    switch (reclass->type) {
    case RECLASS_TABLE:
        if (reclass->min > reclass->max || reclass->num <= 0) {
            G_fatal_error(_("Illegal reclass request"));
            return -1;
        }
        break;
    default:
        G_fatal_error(_("Illegal reclass type"));
        return -1;
    }

    fd = fopen_cellhd_new(name);
    if (fd == NULL) {
        G_warning(_("Unable to create header file for <%s@%s>"), name,
                  G_mapset());
        return -1;
    }

    fprintf(fd, "reclass\n");
    fprintf(fd, "name: %s\n", reclass->name);
    fprintf(fd, "mapset: %s\n", reclass->mapset);

    /* find first non-null entry */
    for (min = 0; min < reclass->num; min++)
        if (!Rast_is_c_null_value(&reclass->table[min]))
            break;
    /* find last non-zero entry */
    for (max = reclass->num - 1; max >= 0; max--)
        if (!Rast_is_c_null_value(&reclass->table[max]))
            break;

    /*
     * if the resultant table is empty, write out a dummy table
     * else write out the table
     *   first entry is #min
     *   rest are translations for cat min+i
     */
    if (min > max)
        fprintf(fd, "0\n");
    else {
        fprintf(fd, "#%ld\n", (long)reclass->min + min);
        while (min <= max) {
            if (Rast_is_c_null_value(&reclass->table[min]))
                fprintf(fd, "%s\n", NULL_STRING);
            else
                fprintf(fd, "%ld\n", (long)reclass->table[min]);
            min++;
        }
    }
    fclose(fd);

    strcpy(buf2, reclass->name);
    if ((p = strchr(buf2, '@')))
        *p = 0;

    G_file_name_misc(buf1, "cell_misc", "reclassed_to", reclass->name,
                     reclass->mapset);

    fd = fopen(buf1, "a+");
    if (fd == NULL) {
#if 0
        G_warning(_("Unable to create dependency file in <%s@%s>"),
                  buf2, reclass->mapset);
#endif
        return 1;
    }

    G_fseek(fd, 0L, SEEK_SET);

    xname = G_fully_qualified_name(name, G_mapset());
    found = 0;
    for (;;) {
        char buf[GNAME_MAX + GMAPSET_MAX];

        if (!G_getl2(buf, sizeof(buf), fd))
            break;
        if (strcmp(xname, buf) == 0) {
            found = 1;
            break;
        }
    }

    if (!found)
        fprintf(fd, "%s\n", xname);

    G_free(xname);
    fclose(fd);

    return 1;
}

static FILE *fopen_cellhd_new(const char *name)
{
    return G_fopen_new("cellhd", name);
}

/**
 * \brief Get reclass table from header file
 *
 * If there is reading error due to the format, -1 is returned and,
 * if error_message is not NULL, it will be set to a pointer to a newly
 * allocated string containing an error message with the line where error
 * was encountered.
 *
 * \param fd header file
 * \param[out] reclass pointer to Reclass structure
 * \param[out] error_message pointer to error message

 * \return 1 on success, -1 on format error, -2 on too many categories
 */
static int get_reclass_table(FILE *fd, struct Reclass *reclass,
                             char **error_message)
{
    char buf[128];
    int n;
    int first, null_str_size;
    CELL cat;
    long len;

    /*
     * allocate the table, expanding as each entry is read
     * note that G_realloc() will become G_malloc() if ptr in
     * NULL
     */
    reclass->min = 0;
    reclass->table = NULL;
    null_str_size = strlen(NULL_STRING);
    n = 0;
    first = 1;
    bool min_set = false;
    while (fgets(buf, sizeof buf, fd)) {
        if (first) {
            first = 0;
            if (sscanf(buf, "#%d", &cat) == 1) {
                reclass->min = cat;
                min_set = true;
                continue;
            }
        }
        if (strncmp(buf, NULL_STRING, null_str_size) == 0)
            Rast_set_c_null_value(&cat, 1);
        else {
            if (sscanf(buf, "%d", &cat) != 1) {
                if (reclass->table != NULL)
                    G_free(reclass->table);
                if (error_message != NULL) {
                    if (min_set)
                        G_asprintf(error_message,
                                   _("Reading integer failed on line: %s "
                                     "(after reading min: %d)"),
                                   buf, reclass->min);
                    else
                        G_asprintf(error_message,
                                   _("First entry (min) not read yet and "
                                     "reading integer failed on line: %s"),
                                   buf);
                }
                return -1;
            }
        }
        n++;
        len = (long)n * sizeof(CELL);

        if (len != (int)len) { /* check for int overflow */
            if (reclass->table != NULL)
                G_free(reclass->table);
            return -2;
        }
        reclass->table = (CELL *)G_realloc((char *)reclass->table, (int)len);
        reclass->table[n - 1] = cat;
    }
    reclass->max = reclass->min + n - 1;
    reclass->num = n;
    return 1;
}
