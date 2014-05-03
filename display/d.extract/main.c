
/****************************************************************
 *
 * MODULE:       d.extract
 * 
 * AUTHOR(S):    Radim Blazek, Markus Neteler
 *               
 * PURPOSE:      A graphical vector extractor
 *               
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 ****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

int extract(struct Map_info *, struct Map_info *, int,
	    const struct color_rgb *, const struct color_rgb *);

int main(int argc, char **argv)
{
    struct Option *input, *output, *type_opt;
    struct Option *color_opt, *hcolor_opt;
    struct GModule *module;
    char *mapset;
    struct Map_info In, Out;
    int type;
    struct color_rgb color, hcolor;
    int r, g, b;
    struct field_info *Fi, *Fin;
    int i, n, tbtype, ret;


    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("vector"));
    module->description =
	_("Selects and extracts vectors with mouse into new vector map.");

    input = G_define_standard_option(G_OPT_V_INPUT);
    output = G_define_standard_option(G_OPT_V_OUTPUT);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,line,boundary,centroid,area,face";
    type_opt->answer = "point,line,boundary,centroid,area,face";

    color_opt = G_define_option();
    color_opt->key = "color";
    color_opt->type = TYPE_STRING;
    color_opt->answer = "black";
    color_opt->description = _("Original line color");

    hcolor_opt = G_define_option();
    hcolor_opt->key = "hcolor";
    hcolor_opt->type = TYPE_STRING;
    hcolor_opt->answer = "red";
    hcolor_opt->description = _("Highlight color");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    type = Vect_option_to_types(type_opt);

    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    color = G_standard_color_rgb(BLACK);
    if (G_str_to_color(color_opt->answer, &r, &g, &b)) {
	color.r = r;
	color.g = g;
	color.b = b;
    }

    hcolor = G_standard_color_rgb(RED);
    if (G_str_to_color(hcolor_opt->answer, &r, &g, &b)) {
	hcolor.r = r;
	hcolor.g = g;
	hcolor.b = b;
    }

    mapset = G_find_vector2(input->answer, NULL);

    if (mapset == NULL)
	G_fatal_error(_("Vector map <%s> not found"), input->answer);

    Vect_set_open_level(2);

    if (Vect_open_old(&In, input->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), input->answer);

    if (Vect_open_new(&Out, output->answer, Vect_is_3d(&In)) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), output->answer);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    D_setup(0);

    G_setup_plot(D_get_d_north(), D_get_d_south(), D_get_d_west(),
		 D_get_d_east(), D_move_abs, D_cont_abs);

    extract(&In, &Out, type, &color, &hcolor);

    R_close_driver();

    /* Copy tables */
    G_message(_("Copying tables..."));
    n = Vect_get_num_dblinks(&In);
    tbtype = GV_1TABLE;
    if (n > 1)
	tbtype = GV_MTABLE;
    for (i = 0; i < n; i++) {
	Fi = Vect_get_dblink(&In, i);
	if (Fi == NULL) {
	    G_warning(_("Cannot get db link info -> cannot copy table."));
	    continue;
	}
	Fin = Vect_default_field_info(&Out, Fi->number, Fi->name, tbtype);
	G_debug(3, "Copy drv:db:table '%s:%s:%s' to '%s:%s:%s'",
		Fi->driver, Fi->database, Fi->table, Fin->driver,
		Fin->database, Fin->table);
	Vect_map_add_dblink(&Out, Fi->number, Fi->name, Fin->table, Fi->key,
			    Fin->database, Fin->driver);

	ret = db_copy_table(Fi->driver, Fi->database, Fi->table,
			    Fin->driver, Vect_subst_var(Fin->database, &Out),
			    Fin->table);
	if (ret == DB_FAILED) {
	    G_warning("Unable to copy table");
	    continue;
	}
    }				/* for of copy table */

    Vect_build(&Out, stdout);
    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
