#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>

#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Writes the
 * categories stored in the <em>cats</em> structure into the categories file for
 * map <em>name</em> in the current mapset.  See <em>Rast_write_cats</em>
 * (Raster_Category_File) for details and return values.
 *
 *  \param name
 *  \param cats
 *  \return int
 */

int Rast3d_write_cats(const char *name, struct Categories *cats)
 /* adapted from Rast_write_cats */
{
    FILE *fd;
    int i;
    const char *descr;
    DCELL val1, val2;
    char str1[100], str2[100];

    fd = G_fopen_new_misc(RASTER3D_DIRECTORY, RASTER3D_CATS_ELEMENT, name);
    if (!fd)
	return -1;

    /* write # cats - note # indicate 3.0 or later */
    fprintf(fd, "# %ld categories\n", (long)cats->num);

    /* title */
    fprintf(fd, "%s\n", cats->title != NULL ? cats->title : "");

    /* write format and coefficients */
    fprintf(fd, "%s\n", cats->fmt != NULL ? cats->fmt : "");
    fprintf(fd, "%.2f %.2f %.2f %.2f\n",
	    cats->m1, cats->a1, cats->m2, cats->a2);

    /* write the cat numbers:label */
    for (i = 0; i < Rast_quant_nof_rules(&cats->q); i++) {
	descr = Rast_get_ith_d_cat(cats, i, &val1, &val2);
	if ((cats->fmt && cats->fmt[0]) || (descr && descr[0])) {
	    if (val1 == val2) {
		sprintf(str1, "%.10f", val1);
		G_trim_decimal(str1);
		fprintf(fd, "%s:%s\n", str1, descr != NULL ? descr : "");
	    }
	    else {
		sprintf(str1, "%.10f", val1);
		G_trim_decimal(str1);
		sprintf(str2, "%.10f", val2);
		G_trim_decimal(str2);
		fprintf(fd, "%s:%s:%s\n", str1, str2,
			descr != NULL ? descr : "");
	    }
	}
    }
    fclose(fd);
    return 1;
}

/*---------------------------------------------------------------------------*/

static int
read_cats(const char *name, const char *mapset, struct Categories *pcats)
 /* adapted from G__read_cats */
{
    FILE *fd;
    char buff[1024];
    CELL cat;
    DCELL val1, val2;
    int old;
    long num = -1;

    fd = G_fopen_old_misc(RASTER3D_DIRECTORY, RASTER3D_CATS_ELEMENT, name, mapset);
    if (!fd)
	return -2;

    /* Read the number of categories */
    if (G_getl(buff, sizeof(buff), fd) == 0)
	goto error;

    if (sscanf(buff, "# %ld", &num) == 1)
	old = 0;
    else if (sscanf(buff, "%ld", &num) == 1)
	old = 1;

    /* Read the title for the file */
    if (G_getl(buff, sizeof(buff), fd) == 0)
	goto error;
    G_strip(buff);

    Rast_init_cats(buff, pcats);
    if (num >= 0)
	pcats->num = num;

    if (!old) {
	char fmt[256];
	float m1, a1, m2, a2;

	if (G_getl(fmt, sizeof(fmt), fd) == 0)
	    goto error;
	/* next line contains equation coefficients */
	if (G_getl(buff, sizeof(buff), fd) == 0)
	    goto error;
	if (sscanf(buff, "%f %f %f %f", &m1, &a1, &m2, &a2) != 4)
	    goto error;
	Rast_set_cats_fmt(fmt, m1, a1, m2, a2, pcats);
    }

    /* Read all category names */
    for (cat = 0;; cat++) {
	char label[1024];

	if (G_getl(buff, sizeof(buff), fd) == 0)
	    break;

	if (old)
	    Rast_set_c_cat(&cat, &cat, buff, pcats);
	else {
	    *label = 0;
	    if (sscanf(buff, "%1s", label) != 1)
		continue;
	    if (*label == '#')
		continue;
	    *label = 0;

	    /* try to read a range of data */
	    if (sscanf(buff, "%lf:%lf:%[^\n]", &val1, &val2, label) == 3)
		Rast_set_cat(&val1, &val2, label, pcats, DCELL_TYPE);
	    else if (sscanf(buff, "%d:%[^\n]", &cat, label) >= 1)
		Rast_set_cat(&cat, &cat, label, pcats, CELL_TYPE);
	    else if (sscanf(buff, "%lf:%[^\n]", &val1, label) >= 1)
		Rast_set_cat(&val1, &val1, label, pcats, DCELL_TYPE);
	    else
		goto error;
	}
    }

    fclose(fd);
    return 0;

  error:
    fclose(fd);
    return -1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Reads the categories file for map <em>name</em> in <em>mapset</em> and
 * stores the categories in the <em>pcats</em> structure.  See <em>Rast_read_cats</em>
 * (Raster_Category_File) for details and return values.
 *
 *  \param name
 *  \param mapset
 *  \param pcats
 *  \return int
 */

int
Rast3d_read_cats(const char *name, const char *mapset, struct Categories *pcats)
 /* adapted from Rast_read_cats */
{
    const char *type;

    switch (read_cats(name, mapset, pcats)) {
    case -2:
	type = "missing";
	break;
    case -1:
	type = "invalid";
	break;
    default:
	return 0;
    }

    G_warning("category support for [%s] in mapset [%s] %s",
	      name, mapset, type);
    return -1;
}
