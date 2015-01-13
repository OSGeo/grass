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

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static const char NULL_STRING[] = "null";
static int reclass_type(FILE *, char **, char **);
static FILE *fopen_cellhd_old(const char *, const char *);
static FILE *fopen_cellhd_new(const char *);
static int get_reclass_table(FILE *, struct Reclass *);

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
int Rast_is_reclass(const char *name, const char *mapset, char *rname,
		    char *rmapset)
{
    FILE *fd;
    int type;

    fd = fopen_cellhd_old(name, mapset);
    if (fd == NULL)
	return -1;

    type = reclass_type(fd, &rname, &rmapset);
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

   \return -1 on error
   \return type code
 */
int Rast_get_reclass(const char *name, const char *mapset,
		     struct Reclass *reclass)
{
    FILE *fd;
    int stat;

    fd = fopen_cellhd_old(name, mapset);
    if (fd == NULL)
	return -1;
    reclass->name = NULL;
    reclass->mapset = NULL;
    reclass->type = reclass_type(fd, &reclass->name, &reclass->mapset);
    if (reclass->type <= 0) {
	fclose(fd);
	return reclass->type;
    }

    switch (reclass->type) {
    case RECLASS_TABLE:
	stat = get_reclass_table(fd, reclass);
	break;
    default:
	stat = -1;
    }

    fclose(fd);
    if (stat < 0) {
	if (stat == -2)
	    G_warning(_("Too many reclass categories for <%s@%s>"),
		      name, mapset);
	else
	    G_warning(_("Illegal reclass format in header file for <%s@%s>"),
		      name, mapset);
	stat = -1;
    }
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

static int reclass_type(FILE * fd, char **rname, char **rmapset)
{
    char buf[128];
    char label[128], arg[128];
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
	if (fgets(buf, sizeof buf, fd) == NULL)
	    return -1;
	if (sscanf(buf, "%[^:]:%s", label, arg) != 2)
	    return -1;
	if (strncmp(label, "maps", 4) == 0) {
	    if (*rmapset)
		strcpy(*rmapset, arg);
	    else
		*rmapset = G_store(arg);
	}
	else if (strncmp(label, "name", 4) == 0) {
	    if (*rname)
		strcpy(*rname, arg);
	    else
		*rname = G_store(arg);
	}
	else
	    return -1;
    }
    if (**rmapset && **rname)
	return type;
    else
	return -1;
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
	G_warning(_("Unable to create header file for <%s@%s>"),
		  name, G_mapset());
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

static int get_reclass_table(FILE * fd, struct Reclass *reclass)
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
    while (fgets(buf, sizeof buf, fd)) {
	if (first) {
	    first = 0;
	    if (sscanf(buf, "#%d", &cat) == 1) {
		reclass->min = cat;
		continue;
	    }
	}
	if (strncmp(buf, NULL_STRING, null_str_size) == 0)
	    Rast_set_c_null_value(&cat, 1);
	else {
	    if (sscanf(buf, "%d", &cat) != 1)
		return -1;
	}
	n++;
	len = (long)n *sizeof(CELL);

	if (len != (int)len) {	/* check for int overflow */
	    if (reclass->table != NULL)
		G_free(reclass->table);
	    return -2;
	}
	reclass->table = (CELL *) G_realloc((char *)reclass->table, (int)len);
	reclass->table[n - 1] = cat;
    }
    reclass->max = reclass->min + n - 1;
    reclass->num = n;
    return 1;
}
