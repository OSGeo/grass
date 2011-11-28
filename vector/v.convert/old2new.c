#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "conv.h"
#include "local_proto.h"

int old2new(char *in, char *out, int endian)
{
    int i, j, nlines, ncats, sline, att;
    const char *mapset;
    FILE *Digin, *Attin;
    struct Line *lines;		/* array of points and lines */
    struct Categ *cats;		/* array of categories */
    struct Map_info Mapout;
    double dist, sdist;
    struct line_cats *cat_out;
    struct line_pnts *pnt_out;

    cats = NULL;

    /* open in map */
    if (NULL == (mapset = G_find_file("dig", in, ""))) {
	G_fatal_error(_("Vector map <%s> not found"), in);
    }

    /* open input dig file */
    if ((Digin = G_fopen_old("dig", in, mapset)) == NULL) {
	G_fatal_error(_("Failed opening input dig file."));
    }

    /* open new output file */
    if (Vect_open_new(&Mapout, out, WITHOUT_Z) < 0) {
	fclose(Digin);
	exit(EXIT_FAILURE);
    }

    Vect_hist_command(&Mapout);

    /* open input dig_att file if exists */
    att = FALSE;
    if (NULL == G_find_file("dig_att", in, mapset)) {
	G_warning(_("dig_att file doesn't exist."));
    }
    else {
	if (NULL == (Attin = G_fopen_old("dig_att", in, mapset))) {
	    G_warning(_("Failed opening input dig_att file."));
	}
	else {
	    att = TRUE;
	}
    }

    /* read old dig file */
    nlines = read_dig(Digin, &Mapout, &lines, endian, att);

    /* read old dig_att file */
    ncats = 0;
    if (att) {
	ncats = read_att(Attin, &cats);
	fclose(Attin);
    }

    /* Attach categories to lines and points.
     * Walk through all cats and always find nearest line.
     * If cat is already attached but new one is nearer
     * set cat to new one.  */

    G_message(_("Attaching categories..."));

    for (i = 0; i < ncats; i++) {
	G_percent(i, ncats - 1, 1);
	if (cats[i].type & (GV_POINT | GV_LINE)) {
	    sline = -1;
	    for (j = 0; j < nlines; j++) {
		if (lines[j].type == cats[i].type) {
		    dist = ldist(cats[i].x, cats[i].y, &(lines[j]));

		    if (sline == -1 || dist < sdist) {
			sline = j;
			sdist = dist;
		    }
		}
	    }
	    if (sline == -1) {
		G_warning(_("Failed to attach an attribute (category %d) to a line."),
			  cats[i].cat);
	    }
	    else {
		if (lines[sline].cat > -1) {
		    G_warning(_("Line %d label: %d matched another label: %d."),
			      sline, lines[sline].cat, cats[i].cat);
		}
		lines[sline].cat = cats[i].cat;
	    }
	}
    }

    /* Write to new file */
    G_message(_("Writing new file..."));

    pnt_out = Vect_new_line_struct();
    cat_out = Vect_new_cats_struct();

    j = 0;
    /* Write all points and lines if dig_att exists */
    if (att) {
	for (i = 0; i < nlines; i++) {
	    if (lines[i].cat > 0) {
		Vect_cat_set(cat_out, 1, lines[i].cat);
	    }
	    dig_alloc_points(pnt_out, lines[i].n_points);
	    memcpy((void *)pnt_out->x, (void *)lines[i].x,
		   lines[i].n_points * sizeof(double));
	    memcpy((void *)pnt_out->y, (void *)lines[i].y,
		   lines[i].n_points * sizeof(double));
	    pnt_out->n_points = lines[i].n_points;
	    Vect_write_line(&Mapout, lines[i].type, pnt_out, cat_out);
	    j++;
	    Vect_reset_cats(cat_out);
	}
	G_message(_("[%d] points and lines written to output file."), j);
    }
    /* Write centroids */
    j = 0;
    for (i = 0; i < ncats; i++) {
	if (cats[i].type == GV_CENTROID) {
	    Vect_append_point(pnt_out, cats[i].x, cats[i].y, 0.0);
	    Vect_cat_set(cat_out, 1, cats[i].cat);
	    Vect_write_line(&Mapout, GV_CENTROID, pnt_out, cat_out);
	    j++;
	    Vect_reset_line(pnt_out);
	    Vect_reset_cats(cat_out);
	}
    }
    G_message(_("[%d] centroids written to output file."), j);

    /* Convert dig_cats to table */
    attributes(in, &Mapout);

    Vect_build(&Mapout);
    Vect_close(&Mapout);

    fclose(Digin);

    /* free memory */
    for (i = 0; i < nlines; i++) {
	G_free(lines[i].x);
	G_free(lines[i].y);
    }
    G_free(lines);
    if (att)
	G_free(cats);

    Vect_destroy_cats_struct(cat_out);
    Vect_destroy_line_struct(pnt_out);

    return (1);
}
