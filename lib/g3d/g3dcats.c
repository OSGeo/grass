#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <grass/gis.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Writes the
 * categories stored in the <em>cats</em> structure into the categories file for
 * map <em>name</em> in the current mapset.  See <em>G_write_cats</em>
 * (Raster_Category_File) for details and return values.
 *
 *  \param name
 *  \param cats
 *  \return int
 */

int G3d_writeCats(const char *name, struct Categories *cats)
 /* adapted from G_write_cats */
{
    FILE *fd;
    int i;
    const char *descr;
    DCELL val1, val2;
    char str1[100], str2[100], buf[200], buf2[200], xname[GNAME_MAX],
	xmapset[GMAPSET_MAX];

    if (G__name_is_fully_qualified(name, xname, xmapset)) {
	sprintf(buf, "%s/%s", G3D_DIRECTORY, xname);
	sprintf(buf2, "%s@%s", G3D_CATS_ELEMENT, xmapset);	/* == cats@mapset */
    }
    else {
	sprintf(buf, "%s/%s", G3D_DIRECTORY, name);
	sprintf(buf2, "%s", G3D_CATS_ELEMENT);
    }

    if (!(fd = G_fopen_new(buf, buf2)))
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
    for (i = 0; i < G_quant_nof_rules(&cats->q); i++) {
	descr = G_get_ith_d_raster_cat(cats, i, &val1, &val2);
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
    char buff[1024], buf2[200], xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    CELL cat;
    DCELL val1, val2;
    int old;
    long num = -1;

    if (G__name_is_fully_qualified(name, xname, xmapset)) {
	sprintf(buff, "%s/%s", G3D_DIRECTORY, xname);
	sprintf(buf2, "%s@%s", G3D_CATS_ELEMENT, xmapset);	/* == cats@mapset */
    }
    else {
	sprintf(buff, "%s/%s", G3D_DIRECTORY, name);
	sprintf(buf2, "%s", G3D_CATS_ELEMENT);
    }

    if (!(fd = G_fopen_old(buff, buf2, mapset)))
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

    G_init_raster_cats(buff, pcats);
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
	G_set_raster_cats_fmt(fmt, m1, a1, m2, a2, pcats);
    }

    /* Read all category names */
    for (cat = 0;; cat++) {
	char label[1024];

	if (G_getl(buff, sizeof(buff), fd) == 0)
	    break;

	if (old)
	    G_set_cat(cat, buff, pcats);
	else {
	    *label = 0;
	    if (sscanf(buff, "%1s", label) != 1)
		continue;
	    if (*label == '#')
		continue;
	    *label = 0;

	    /* try to read a range of data */
	    if (sscanf(buff, "%lf:%lf:%[^\n]", &val1, &val2, label) == 3)
		G_set_raster_cat(&val1, &val2, label, pcats, DCELL_TYPE);
	    else if (sscanf(buff, "%d:%[^\n]", &cat, label) >= 1)
		G_set_raster_cat(&cat, &cat, label, pcats, CELL_TYPE);
	    else if (sscanf(buff, "%lf:%[^\n]", &val1, label) >= 1)
		G_set_raster_cat(&val1, &val1, label, pcats, DCELL_TYPE);
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
 * stores the categories in the <em>pcats</em> structure.  See <em>G_read_cats</em>
 * (Raster_Category_File) for details and return values.
 *
 *  \param name
 *  \param mapset
 *  \param pcats
 *  \return int
 */

int
G3d_readCats(const char *name, const char *mapset, struct Categories *pcats)
 /* adapted from G_read_cats */
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
