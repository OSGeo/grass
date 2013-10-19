/***************************************************************
 *
 * MODULE:       v.segment
 * 
 * AUTHOR(S):    Radim Blazek
 *               Hamish Bowman (offset bits)
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               Reversed & percent offsets by Huidae Cho <grass4u gmail.com>
 *               
 * PURPOSE:      Generate segments or points from input map and segments read from stdin 
 *               
 * COPYRIGHT:    (C) 2002-2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

int read_point_input(char *buf, char *stype, int *id, int *lcat, double *offset,
		double *side_offset, int *rev, int *pct);
int read_line_input(char *buf, char *stype, int *id, int *lcat,
		double *offset1, double *offset2, double *side_offset,
		int *rev1, int *pct1, int *rev2, int *pct2);
int find_line(struct Map_info *Map, int lfield, int cat);
void offset_pt_90(double *, double *, double, double);

int main(int argc, char **argv)
{
    FILE *in_file;
    int ret, points_written, lines_written, points_read, lines_read;
    int lfield;
    int line;
    int id, lcat;
    double offset1, offset2, side_offset;
    double x, y, z, angle, len;
    char stype;
    struct Option *in_opt, *out_opt;
    struct Option *lfield_opt, *file_opt;
    struct GModule *module;
    char buf[2000];
    struct Map_info In, Out;
    struct line_cats *LCats, *SCats;
    struct line_pnts *LPoints, *SPoints, *PlPoints;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    module->description =
	_("Creates points/segments from input vector lines and positions.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_opt->label = _("Name of input vector lines map");

    lfield_opt = G_define_standard_option(G_OPT_V_FIELD);

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    file_opt = G_define_standard_option(G_OPT_F_INPUT);
    file_opt->key = "file";
    file_opt->label = _("Name of file containing segment rules");
    file_opt->description = _("'-' for standard input");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    LCats = Vect_new_cats_struct();
    SCats = Vect_new_cats_struct();
    LPoints = Vect_new_line_struct();
    SPoints = Vect_new_line_struct();
    PlPoints = Vect_new_line_struct();

    Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				 G_FATAL_EXIT);

    if (strcmp(file_opt->answer, "-")) {
	/* open input file */
	if ((in_file = fopen(file_opt->answer, "r")) == NULL)
	    G_fatal_error(_("Unable to open input file <%s>"),
			  file_opt->answer);
    }
    else
	in_file = stdin;

    /* Open input lines */
    Vect_set_open_level(2);
    Vect_open_old2(&In, in_opt->answer, "", lfield_opt->answer);
    lfield = Vect_get_field_number(&In, lfield_opt->answer);

    /* Open output segments */
    Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In));
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    points_read = 0;
    lines_read = 0;
    points_written = 0;
    lines_written = 0;

    while (1) {
	int rev1, rev2, pct1, pct2;

	if (!file_opt->answer) {
	    if (fgets(buf, sizeof(buf), stdin) == NULL)
		break;
	}
	else {
	    if (G_getl2(buf, sizeof(buf) - 1, in_file) == 0)
		break;
	}

	G_debug(2, "SEGMENT: %s", G_chop(buf));
	side_offset = 0;
	Vect_reset_line(SPoints);
	Vect_reset_cats(SCats);
	Vect_reset_line(PlPoints);

	switch (buf[0]) {
	case 'P':
	    if (!read_point_input(buf, &stype, &id, &lcat, &offset1,
				    &side_offset, &rev1, &pct1)) {
		G_warning(_("Unable to read input: %s"), buf);
		break;
	    }
	    points_read++;
	    G_debug(2, "point: %d %d %s%f%s %f", id, lcat, rev1 ? "-" : "",
			    offset1, pct1 ? "%" : "", side_offset);


	    /* OK, write point */
	    line = find_line(&In, lfield, lcat);
	    if (line == 0) {
		G_warning(_("Unable to find line of cat %d"), lcat);
		break;
	    }

	    Vect_read_line(&In, LPoints, LCats, line);

	    len = Vect_line_length(LPoints);
	    if (pct1)
		offset1 = len * offset1 / 100.0;
	    if (rev1)
		offset1 = len - offset1;

	    ret = Vect_point_on_line(LPoints, offset1, &x, &y, &z, &angle,
			    NULL);
	    if (ret == 0) {
		G_warning(_("Unable to get point on line: cat = %d offset = %f "
			   "(line length = %.15g)\n%s"), lcat, offset1, len,
			  buf);
		break;
	    }

	    if (fabs(side_offset) > 0.0)
		offset_pt_90(&x, &y, angle, side_offset);

	    Vect_append_point(SPoints, x, y, z);
	    Vect_cat_set(SCats, 1, id);

	    Vect_write_line(&Out, GV_POINT, SPoints, SCats);
	    points_written++;
	    break;
	case 'L':
	    if (!read_line_input(buf, &stype, &id, &lcat, &offset1, &offset2,
				    &side_offset, &rev1, &pct1, &rev2, &pct2)) {
		G_warning(_("Unable to read input: %s"), buf);
		break;
	    }
	    lines_read++;
	    G_debug(2, "line: %d %d %s%f%s %s%f%s %f", id, lcat,
			    rev1 ? "-" : "", offset1, pct1 ? "%" : "",
			    rev2 ? "-" : "", offset2, pct2 ? "%" : "",
			    side_offset);

	    line = find_line(&In, lfield, lcat);
	    if (line == 0) {
		G_warning(_("Unable to find line of cat %d"), lcat);
		break;
	    }

	    Vect_read_line(&In, LPoints, LCats, line);

	    len = Vect_line_length(LPoints);
	    if (pct1)
	        offset1 = len * offset1 / 100.0;
	    if (rev1)
	        offset1 = len - offset1;
	    if (pct2)
	        offset2 = len * offset2 / 100.0;
	    if (rev2)
	        offset2 = len - offset2;

	    if (offset1 > offset2) {
		double tmp;

		tmp = offset1;
		offset1 = offset2;
		offset2 = tmp;
	    }

	    if (offset2 > len) {
		G_warning(_("End of segment > line length -> cut"));
		offset2 = len;
	    }

	    ret = Vect_line_segment(LPoints, offset1, offset2, SPoints);
	    if (ret == 0) {
		G_warning(_("Unable to make line segment: "
			    "cat = %d : %f - %f (line length = %.15g)\n%s"),
			  lcat, offset1, offset2, len, buf);
		break;
	    }

	    Vect_cat_set(SCats, 1, id);

	    if (fabs(side_offset) > 0.0) {
		Vect_line_parallel(SPoints, side_offset, side_offset / 10.,
				   TRUE, PlPoints);
		Vect_write_line(&Out, GV_LINE, PlPoints, SCats);
		G_debug(3, "  segment n_points = %d", PlPoints->n_points);
	    }
	    else {
		Vect_write_line(&Out, GV_LINE, SPoints, SCats);
		G_debug(3, "  segment n_points = %d", SPoints->n_points);
	    }

	    lines_written++;
	    break;
	default:
	    G_warning(_("Incorrect segment type: %s"), buf);
	}

    }

    Vect_build(&Out);

    G_message(_("%d points read from input"), points_read);
    G_message(_("%d points written to output map (%d lost)"),
	      points_written, points_read - points_written);
    G_message(_("%d lines read from input"), lines_read);
    G_message(_("%d lines written to output map (%d lost)"),
	      lines_written, lines_read - lines_written);

    /* Free, close ... */
    Vect_close(&In);
    Vect_close(&Out);
    if (file_opt->answer)
	fclose(in_file);

    exit(EXIT_SUCCESS);
}

int read_point_input(char *buf, char *stype, int *id, int *lcat, double *offset,
		double *side_offset, int *rev, int *pct)
{
    char offsetbuf[100];
    int ret;
    char *p;

    *side_offset = 0;
    *rev = 0;
    *pct = 0;

    ret = sscanf(buf, "%c %d %d %[.0-9%-] %lf", stype, id, lcat, offsetbuf,
		    side_offset);
    
    if (ret < 4) {
	return 0;
    }

    p = offsetbuf;
    if (offsetbuf[0] == '-') {
        *rev = 1;
        p++;
    }
    if (offsetbuf[strlen(offsetbuf)-1] == '%') {
        *pct = 1;
        offsetbuf[strlen(offsetbuf)-1] = '\0';
    }
    if (sscanf(p, "%lf", offset) != 1) {
        return 0;
    }

    return 1;
}

int read_line_input(char *buf, char *stype, int *id, int *lcat,
		double *offset1, double *offset2, double *side_offset,
		int *rev1, int *pct1, int *rev2, int *pct2)
{
    char offset1buf[100];
    char offset2buf[100];
    int ret;
    char *p;

    *side_offset = 0;
    *rev1 = 0;
    *pct1 = 0;
    *rev2 = 0;
    *pct2 = 0;

    ret = sscanf(buf, "%c %d %d %[.0-9%-] %[.0-9%-] %lf", stype, id, lcat,
		    offset1buf, offset2buf, side_offset);
    if (ret < 5) {
        return 0;
    }

    p = offset1buf;
    if (offset1buf[0] == '-') {
        *rev1 = 1;
        p++;
    }
    if (offset1buf[strlen(offset1buf)-1] == '%') {
        *pct1 = 1;
        offset1buf[strlen(offset1buf)-1] = '\0';
    }
    if (sscanf(p, "%lf", offset1) != 1) {
        return 0;
    }

    p = offset2buf;
    if (offset2buf[0] == '-') {
        *rev2 = 1;
        p++;
    }
    if (offset2buf[strlen(offset2buf)-1] == '%') {
        *pct2 = 1;
        offset2buf[strlen(offset2buf)-1] = '\0';
    }
    if (sscanf(p, "%lf", offset2) != 1) {
        return 0;
    }

    return 1;
}

/* Find line by cat, returns 0 if not found */
int find_line(struct Map_info *Map, int lfield, int lcat)
{
    int i, nlines, type, cat;
    struct line_cats *Cats;

    G_debug(2, "find_line(): llayer = %d lcat = %d", lfield, lcat);
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(Map);
    for (i = 1; i <= nlines; i++) {
	type = Vect_read_line(Map, NULL, Cats, i);
	if (!(type & GV_LINE))
	    continue;
	Vect_cat_get(Cats, lfield, &cat);
	if (cat == lcat)
	    return i;
    }

    return 0;
}


/* calculate a point perpendicular to the current line angle, offset by a distance
 * works in the x,y plane.
 */
void offset_pt_90(double *x, double *y, double angle, double distance)
{
    *x -= distance * cos(M_PI_2 + angle);
    *y -= distance * sin(M_PI_2 + angle);
}
