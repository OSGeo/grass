/* ****************************************************************************
 *
 * MODULE:       v.label 
 * AUTHOR(S):    Philip Verhagen (original s.label), Radim Blazek, Hamish Bowman 
 * PURPOSE:      Create paint labels    
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#define PI M_PI

struct Option *Xoffset, *Yoffset, *Reference, *Font, *Color, *Size;
struct Option *Width, *Hcolor, *Hwidth, *Bcolor, *Border, *Opaque;
int fontsize;
char ref_pt[24];

void print_label(FILE *, double, double, double, char *);

int main(int argc, char **argv)
{
    int i, cnt, nrows, txtlength, field, more;
    int type, ltype;
    int cat, direction;
    double x, y, linlength, lablength, size, space, ldist;
    double rotate, rot;
    char *txt, buf[2000];
    struct line_pnts *Points;
    struct line_cats *Cats;
    FILE *labels;

    struct Map_info Map;
    struct GModule *module;
    struct Option *Vectfile, *Typopt, *Fieldopt, *Colopt, *whereopt;
    struct Option *Labelfile, *Space, *FontSize, *Rotation;
    struct Flag *Along_flag, *Curl_flag;

    struct field_info *fi;
    dbDriver *driver;
    dbString stmt, valstr;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("paint labels"));
    module->description =
	_("Creates paint labels for a vector map from attached attributes.");


    Labelfile = G_define_option();
    Labelfile->key = "labels";
    Labelfile->label = _("Name for new paint-label file");
    Labelfile->description =
	_("If not given the name of the input map is used");
    Labelfile->type = TYPE_STRING;
    Labelfile->required = NO;
    Labelfile->key_desc = "name";

    Vectfile = G_define_standard_option(G_OPT_V_MAP);

    Colopt = G_define_standard_option(G_OPT_DB_COLUMN);
    Colopt->required = YES;
    Colopt->description = _("Name of attribute column to be used for labels");

    Typopt = G_define_standard_option(G_OPT_V_TYPE);
    Typopt->options = "point,line,boundary,centroid";
    Typopt->answer = "point,line,boundary,centroid";

    Fieldopt = G_define_standard_option(G_OPT_V_FIELD);
    whereopt = G_define_standard_option(G_OPT_DB_WHERE);

    Along_flag = G_define_flag();
    Along_flag->key = 'a';
    Along_flag->description = _("Rotate labels to align with lines");
    Along_flag->guisection = _("Effects");

    Curl_flag = G_define_flag();
    Curl_flag->key = 'c';
    Curl_flag->description = _("Curl labels along lines");
    Curl_flag->guisection = _("Effects");

    Xoffset = G_define_option();
    Xoffset->key = "xoffset";
    Xoffset->description = _("Offset label in x-direction");
    Xoffset->type = TYPE_DOUBLE;
    Xoffset->answer = "0";
    Xoffset->guisection = _("Placement");

    Yoffset = G_define_option();
    Yoffset->key = "yoffset";
    Yoffset->description = _("Offset label in y-direction");
    Yoffset->type = TYPE_DOUBLE;
    Yoffset->answer = "0";
    Yoffset->guisection = _("Placement");

    Reference = G_define_option();
    Reference->key = "reference";
    Reference->description = _("Reference position");
    Reference->type = TYPE_STRING;
    Reference->multiple = YES;
    Reference->answer = "center";
    Reference->options = "center,left,right,upper,lower";
    Reference->guisection = _("Placement");

    Font = G_define_option();
    Font->key = "font";
    Font->description = _("Font name");
    Font->type = TYPE_STRING;
    Font->answer = "standard";
    Font->guisection = _("Font");

    Size = G_define_option();
    Size->key = "size";
    Size->description = _("Label size (in map-units)");
    Size->type = TYPE_DOUBLE;
    Size->answer = "100";
    Size->guisection = _("Font");

    Space = G_define_option();
    Space->key = "space";
    Space->description =
	_("Space between letters for curled labels (in map-units)");
    Space->type = TYPE_DOUBLE;
    Space->required = NO;
    Space->guisection = _("Font");

    FontSize = G_define_option();
    FontSize->key = "fontsize";
    FontSize->description = _("Label size (in points)");
    FontSize->type = TYPE_INTEGER;
    FontSize->required = NO;
    FontSize->options = "1-1000";
    FontSize->guisection = _("Font");

    Color = G_define_standard_option(G_OPT_C);
    Color->label = _("Text color");
    Color->guisection = _("Colors");

    Rotation = G_define_option();
    Rotation->key = "rotation";
    Rotation->description = _("Rotation angle in degrees (counter-clockwise)");
    Rotation->type = TYPE_DOUBLE;
    Rotation->required = NO;
    Rotation->options = "0-360";
    Rotation->answer = "0";
    Rotation->key_desc = "angle";
    Rotation->guisection = _("Placement");

    Width = G_define_option();
    Width->key = "width";
    Width->description = _("Border width");
    Width->type = TYPE_DOUBLE;
    Width->answer = "1";
    Width->options = "0-25";
    Width->guisection = _("Effects");

    Hcolor = G_define_standard_option(G_OPT_CN);
    Hcolor->key = "highlight_color";
    Hcolor->label = _("Highlight color for text");
    Hcolor->answer = "none";
    Hcolor->guisection = _("Colors");

    Hwidth = G_define_option();
    Hwidth->key = "highlight_width";
    Hwidth->description = _("Width of highlight coloring");
    Hwidth->type = TYPE_DOUBLE;
    Hwidth->answer = "0";
    Hwidth->guisection = _("Effects");

    Bcolor = G_define_standard_option(G_OPT_CN);
    Bcolor->key = "bgcolor";
    Bcolor->label = _("Background color");
    Bcolor->answer = "none";
    Bcolor->guisection = _("Colors");

    Border = G_define_standard_option(G_OPT_CN);
    Border->key = "border";
    Border->label = _("Border color");
    Border->answer = "none";
    Border->guisection = _("Colors");

    Opaque = G_define_option();
    Opaque->key = "opaque";
    Opaque->description =
	_("Opaque to vector (only relevant if background color is selected)");
    Opaque->type = TYPE_STRING;
    Opaque->answer = "yes";
    Opaque->options = "yes,no";
    Opaque->key_desc = "yes|no";
    Opaque->guisection = _("Colors");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    if (Curl_flag->answer)
	Along_flag->answer = 1;

    db_init_string(&stmt);
    db_init_string(&valstr);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    type = Vect_option_to_types(Typopt);

    size = atof(Size->answer);
    space = size;		/* default: set spacing according to letter size (map units) */
    rotate = atof(Rotation->answer);

    if (FontSize->answer) {
	fontsize = atoi(FontSize->answer);

	/* figure out space param dynamically from current dispay */
	/* don't bother if Space was explicitly given (bypasses xmon req) */
	if (Along_flag->answer && !Space->answer) {
	    if (D_open_driver() != 0)	/* connect to the driver */
		G_fatal_error(_("No graphics device selected"));

	    /* Read in the map region associated with graphics window */
	    D_setup(0);
	    space = fontsize / D_get_u_to_d_xconv();	/* in earth units */

	    D_close_driver();
	}
    }
    else
	fontsize = 0;

    /* or if user explicitly gave a number for letter spacing, use that */
    if (Space->answer)
	space = atof(Space->answer);

    if (Along_flag->answer && !fontsize &&
	(size / space >= 2 || size / space <= 0.5))
	G_warning(_("size and space options vary significantly which may lead to crummy output"));


    /* parse reference answers */
    i = 0;
    strcpy(ref_pt, "");
    while (Reference->answers[i]) {
	if (i > 1)
	    G_fatal_error(_("Too many parameters for <reference>"));
	if (i > 0)
	    strcat(ref_pt, " ");
	strncat(ref_pt, Reference->answers[i], 7);
	i++;
    }

    /* open vector */
    if (Vect_open_old(&Map, Vectfile->answer, "") < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), Vectfile->answer);

    /* open database */
    field = atoi(Fieldopt->answer);
    fi = Vect_get_field(&Map, field);
    if (fi == NULL)
	G_fatal_error(_("Unable to get layer info for vector map"));

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database, fi->driver);

    /* open labels */
    if (!Labelfile->answer)
	Labelfile->answer = Vectfile->answer;

    labels = G_fopen_new("paint/labels", Labelfile->answer);

    /* write label */
    cnt = 0;

    while (1) {
	ltype = Vect_read_next_line(&Map, Points, Cats);
	if (ltype == -1)
	    G_fatal_error(_("Unable to read vector map"));
	if (ltype == -2)
	    break;		/* EOF */
	if (!(type & ltype))
	    continue;

	Vect_cat_get(Cats, field, &cat);
	if (cat < 0)
	    continue;		/* no cat for this field */

	/* Read label from database */

	if (whereopt->answer) {
	    sprintf(buf, "select %s from %s where %s = %d and %s",
		    Colopt->answer, fi->table, fi->key, cat,
		    whereopt->answer);
	}
	else {
	    sprintf(buf, "select %s from %s where %s = %d",
		    Colopt->answer, fi->table, fi->key, cat);
	}
	G_debug(3, "SQL: %s", buf);
	db_set_string(&stmt, buf);

	if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) !=
	    DB_OK)
	    G_fatal_error(_("Unable to select attributes"));

	nrows = db_get_num_rows(&cursor);
	if (nrows < 1) {
	    /* not optimal, but the warning isn't /that/ critical. */
	    if (!whereopt->answer) {
		G_warning(_("No record for category %d in table <%s>"),
			  cat, fi->table);
	    }
	    continue;
	}

	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK || !more)
	    continue;

	table = db_get_cursor_table(&cursor);
	column = db_get_table_column(table, 0);	/* first column */

	db_convert_column_value_to_string(column, &valstr);
	db_close_cursor(&cursor);

	txt = db_get_string(&valstr);
	G_debug(3, "Label: %s", txt);

	txtlength = strlen(txt);
	if (txtlength == 0)
	    continue;

	/* Line length */
	linlength = Vect_line_length(Points);

	if (ltype & GV_POINTS) {
	    print_label(labels, Points->x[0], Points->y[0], rotate, txt);
	}
	else if (!Along_flag->answer) {	/* Line, but not along */
	    /* get centre */
	    Vect_point_on_line(Points, linlength / 2, &x, &y, NULL, NULL,
			       NULL);
	    print_label(labels, x, y, rotate, txt);
	}
	else {			/* Along line */

	    /* find best orientation (most letters by bottom to down side */
	    rotate = 0;
	    for (i = 0; i < txtlength; i++) {
		/* distance of the letter from the beginning of line */
		lablength = txtlength * space;
		ldist = i * space + (linlength - lablength) / 2;

		if (ldist < 0)
		    ldist = 0;
		if (ldist > linlength)
		    ldist = linlength;

		Vect_point_on_line(Points, ldist, &x, &y, NULL, &rot, NULL);
		rot = rot * 180 / PI;
		if (rot > 90 || rot < -90)
		    rotate += -1;
		else
		    rotate += 1;
	    }
	    if (rotate >= 0) {
		direction = 0;
	    }
	    else {
		direction = 1;
	    }

	    if (Curl_flag->answer) {
		for (i = 0; i < txtlength; i++) {
		    /* distance of the letter from the beginning of line */
		    lablength = txtlength * space;

		    ldist = i * space + (linlength - lablength) / 2;

		    if (ldist < 0)
			ldist = 0;
		    if (ldist > linlength)
			ldist = linlength;

		    Vect_point_on_line(Points, ldist, &x, &y, NULL, &rotate,
				       NULL);
		    rotate = rotate * 180 / PI;

		    if (direction == 0) {
			sprintf(buf, "%c", txt[i]);
		    }
		    else {
			sprintf(buf, "%c", txt[txtlength - i - 1]);
			rotate += 180;
		    }
		    print_label(labels, x, y, rotate, buf);
		}
	    }
	    else {		/* same as above but take center value for placement & rotation */
		i = (int)(txtlength / 2.0 + 0.5);
		lablength = txtlength * space;
		ldist = i * space + (linlength - lablength) / 2;
		if (ldist < 0)
		    ldist = 0;
		if (ldist > linlength)
		    ldist = linlength;
		Vect_point_on_line(Points, ldist, &x, &y, NULL, &rotate,
				   NULL);
		rotate = rotate * 180 / PI;
		if (direction != 0)
		    rotate += 180;
		print_label(labels, x, y, rotate, txt);
	    }
	}
	cnt++;
    }

    Vect_destroy_line_struct(Points);

    Vect_close(&Map);
    db_close_database_shutdown_driver(driver);
    fclose(labels);

    G_message(_("Labeled %d lines."), cnt);

    exit(EXIT_SUCCESS);
}

void print_label(FILE * labels, double x, double y, double rotate,
		 char *label)
{
    fprintf(labels, "east: %f\n", x);
    fprintf(labels, "north: %f\n", y);
    fprintf(labels, "xoffset: %s\n", Xoffset->answer);
    fprintf(labels, "yoffset: %s\n", Yoffset->answer);
    fprintf(labels, "ref: %s\n", ref_pt);
    fprintf(labels, "font: %s\n", Font->answer);
    fprintf(labels, "color: %s\n", Color->answer);

    if (fontsize)
	fprintf(labels, "fontsize: %d\n", fontsize);
    else
	fprintf(labels, "size: %s\n", Size->answer);

    fprintf(labels, "width: %s\n", Width->answer);
    fprintf(labels, "hcolor: %s\n", Hcolor->answer);
    fprintf(labels, "hwidth: %s\n", Hwidth->answer);
    fprintf(labels, "background: %s\n", Bcolor->answer);
    fprintf(labels, "border: %s\n", Border->answer);
    fprintf(labels, "opaque: %s\n", Opaque->answer);
    if (rotate != 0)
	fprintf(labels, "rotate: %f\n", rotate);

    fprintf(labels, "text: %s\n\n", label);
}
