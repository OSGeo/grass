/****************************************************************
 *
 * MODULE:     v.example
 *
 * AUTHOR(S):  GRASS Development Team
 *
 * PURPOSE:    example vector module does something like:
 *               v.llabel -i map=m1 value=1
 *             but the new map is written instead of update of 
 *             the old one
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 * TODO:       - add DB support
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    int i, type, cat;
    char *mapset;
    struct GModule *module;     /* GRASS module for parsing arguments */
    struct Option *old, *new;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    module->keywords = _("vector, keyword2, keyword3");
    module->description = _("My first vector module");

    /* Define the different options as defined in gis.h */
    old = G_define_standard_option(G_OPT_V_INPUT);

    new = G_define_standard_option(G_OPT_V_OUTPUT);

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_check_input_output_name(new->answer, old->answer, GV_FATAL_EXIT);

    if ((mapset = G_find_vector2(old->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), old->answer);

    Vect_set_open_level(2);

    if (1 > Vect_open_old(&In, old->answer, mapset))
	G_fatal_error(_("Unable to open vector map <%s>"), old->answer);

    if (0 > Vect_open_new(&Out, new->answer, WITHOUT_Z)) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"), new->answer);
    }

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    i = 1;
    while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	if (type == GV_LINE) {
	    if (Vect_cat_get(Cats, 1, &cat) == 0) {
		Vect_cat_set(Cats, 1, i);
		i++;
	    }
	}
	Vect_write_line(&Out, type, Points, Cats);
    }

    Vect_build(&Out, stdout);
    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
