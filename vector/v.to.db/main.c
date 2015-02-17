
/****************************************************************************
 *
 * MODULE:       v.to.db
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Wolf Bergenheim <wolf+grass bergenheim net>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      load values from vector to database
 * COPYRIGHT:    (C) 2000-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/glocale.h>
#include "global.h"

struct value *Values;
struct options options;
struct vstat vstat;

int main(int argc, char *argv[])
{
    int n, i, j, cat, lastcat, type, id, findex;
    struct Map_info Map;
    struct GModule *module;
    struct field_info *Fi;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("database"));
    module->description = _("Populates attribute values from vector features.");

    parse_command_line(argc, argv);

    if (!options.print && !options.total) {
        const char *mapset;

        mapset = G_find_vector2(options.name, "");
        if (!mapset || (strcmp(mapset, G_mapset()) != 0))
            G_fatal_error(_("Vector map <%s> not found in the current mapset. "
                            "Unable to modify vector maps from different mapsets."),
                          options.name);
    }

    G_begin_distance_calculations();
    G_begin_polygon_area_calculations();

    /* open map */
    Vect_set_open_level(2);
    if (Vect_open_old(&Map, options.name, "") < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), options.name);
    Vect_set_error_handler_io(&Map, NULL);
   
    Fi = Vect_get_field(&Map, options.field);

    if (!options.print && Fi == NULL) {
	G_fatal_error(_("Database connection not defined for layer %d. "
			"Use v.db.connect first."),
		      options.field);
    }

    /* allocate array for values */
    /* (+ 1 is for cat -1 (no category) reported at the end ) */
    findex = Vect_cidx_get_field_index(&Map, options.field);
    if (Vect_cidx_get_field_index(&Map, options.field) > -1) {
	n = Vect_cidx_get_num_unique_cats_by_index(&Map, findex);
    }
    else {
	n = 0;
    }
    G_debug(2, "%d unique cats", n);
    Values = (struct value *) G_calloc(n + 1, sizeof(struct value));

    /* prepopulate Values */
    n = Vect_cidx_get_num_cats_by_index(&Map, findex);
    i = 0;
    Values[i].cat = -1;		/* features without category */
    Values[i].used = 0;
    Values[i].count1 = 0;
    Values[i].count1 = 0;
    Values[i].i1 = -1;
    Values[i].i2 = -1;
    Values[i].d1 = 0.0;
    Values[i].d2 = 0.0;
    Values[i].qcat = NULL;
    Values[i].nqcats = 0;
    Values[i].aqcats = 0;

    i = 1;
    lastcat = -1;
    /* category index must be sorted,
     * i.e. topology must have been built with GV_BUILD_ALL */
    for (j = 0; j < n; j++) {
	Vect_cidx_get_cat_by_index(&Map, findex, j, &cat, &type, &id);
	if (lastcat > cat) {
	    Vect_close(&Map);
	    G_fatal_error(_("Category index for vector map <%s> is not sorted"),
	                  options.name);
	}

	if (lastcat != cat) {
	    Values[i].cat = cat;
	    Values[i].used = 0;
	    Values[i].count1 = 0;
	    Values[i].count1 = 0;
	    Values[i].i1 = -1;
	    Values[i].i2 = -1;
	    Values[i].d1 = 0.0;
	    Values[i].d2 = 0.0;
	    Values[i].qcat = NULL;
	    Values[i].nqcats = 0;
	    Values[i].aqcats = 0;

	    lastcat = cat;
	    i++;
	}
    }

    vstat.rcat = i;

    /* Read values from map */
    if (options.option == O_QUERY) {
	query(&Map);
    }
    else if ((options.option == O_AREA) || (options.option == O_COMPACT) ||
	     (options.option == O_PERIMETER) || (options.option == O_FD)) {
	read_areas(&Map);
    }
    else {
	read_lines(&Map);
    }

    /* prune unused values */
    n = vstat.rcat;
    j = 0;
    for (i = 0; i < n; i++) {
	if (Values[i].used) {
	    Values[j] = Values[i];
	    j++;
	}
    }
    vstat.rcat = j;

    conv_units();

    if (options.print || options.total) {
	report();
    }
    else {
	update(&Map);
	Vect_set_db_updated(&Map);
    }

    Vect_close(&Map);

    if (!(options.print || options.total)) {
	print_stat();
    }

    /* free list */
    G_free(Values);

    exit(EXIT_SUCCESS);
}
