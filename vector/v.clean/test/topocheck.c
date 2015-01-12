
/***************************************************************
 *
 * MODULE:       v.topo.check
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Check vector topology.
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int i, atype, with_z;
    int nlines, n;
    char *mapset;
    struct GModule *module;
    struct Option *in_opt, *out_opt;
    struct Flag *poly_f;
    int left, right;
    int *lines, ndlines;
    int nareas1, nareas2;
    double tarea1, tarea2, darea, pdarea;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    module->description = "Break lines at intersections.";

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    poly_f = G_define_flag();
    poly_f->key = 'p';
    poly_f->description =
	"Write areas and isles as closed polygons instead of topo check";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    G_begin_cell_area_calculations();

    /* open input vector */
    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level(2);
    if (Vect_open_old(&In, in_opt->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);

    with_z = Vect_is_3d(&In);

    Vect_set_fatal_error(GV_FATAL_PRINT);
    if (0 > Vect_open_new(&Out, out_opt->answer, with_z)) {
	Vect_close(&In);
	exit(1);
    }

    Vect_copy_head_data(&In, &Out);

    if (!poly_f->answer) {	/* do check */
	Vect_copy_map_lines(&In, &Out);
	Vect_build(&Out);

	Vect_close(&In);
	Vect_close(&Out);

	if (Vect_open_update(&Out, out_opt->answer, G_mapset()) < 0)
	    G_fatal_error(_("Unable to open vector map <%s>"), out_opt->answer);

	/* Count number of areas and total area in input */
	nareas1 = Vect_get_num_areas(&Out);
	tarea1 = 0;
	for (i = 1; i <= nareas1; i++) {
	    tarea1 += Vect_get_area_area(&Out, i);
	}

	fprintf(stderr, "Number of areas in : %5d\n", nareas1);
	fprintf(stderr, "Total area in : %f\n", tarea1);

	/* Remove all lines which have areas on both sides */
	nlines = Vect_get_num_lines(&Out);
	/* alloc enough space */
	lines = (int *)G_malloc(nlines * sizeof(int));

	ndlines = 0;
	for (i = 1; i <= nlines; i++) {
	    atype = Vect_read_line(&Out, Points, Cats, i);
	    if (!(atype & GV_BOUNDARY))
		continue;
	    Vect_get_line_areas(&Out, i, &left, &right);
	    if (left > 0 && right > 0) {
		lines[ndlines] = i;
		ndlines++;
	    }
	}

	for (i = 0; i < ndlines; i++) {
	    Vect_delete_line(&Out, lines[i]);
	}

	/* Count number of areas and total area in output */
	nareas2 = 0;
	tarea2 = 0;
	for (i = 1; i <= Vect_get_num_areas(&Out); i++) {
	    if (!Vect_area_alive(&Out, i))
		continue;
	    nareas2++;
	    tarea2 += Vect_get_area_area(&Out, i);
	}
	fprintf(stderr, "Number of areas out: %5d\n", nareas2);
	fprintf(stderr, "Total area out: %f\n", tarea2);

	darea = tarea2 - tarea1;
	pdarea = 100 * darea / tarea2;
	fprintf(stderr, "Area difference: %e = %e %%\n", darea, pdarea);

	Vect_close(&Out);

	if (nareas2 != 1) {
	    fprintf(stderr, "ERROR: number of areas out != 1\n");
	    exit(1);
	}
	if (pdarea < 0.000001) {
	    fprintf(stderr, "Difference < 0.000001 %%, seems to be OK.\n");
	}
	else {
	    fprintf(stderr, "Difference > 0.000001 %%, seems to be WRONG.\n");
	    exit(1);
	}
    }
    else {			/* write polygons */
	n = 0;
	for (i = 1; i <= Vect_get_num_areas(&In); i++) {
	    if (!Vect_area_alive(&In, i))
		continue;
	    Vect_get_area_points(&In, i, Points);
	    Vect_write_line(&Out, GV_BOUNDARY, Points, Cats);

	    n++;
	}
	fprintf(stderr, "%d area polygons written.\n", n);

	Vect_build(&Out);
	Vect_close(&Out);
    }

    exit(0);
}
