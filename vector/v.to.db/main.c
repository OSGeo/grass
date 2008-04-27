/****************************************************************************
 *
 * MODULE:       v.to.db
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Wolf Bergenheim <wolf+grass bergenheim net>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      load values from vector to database
 * COPYRIGHT:    (C) 2000-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define MAIN
#include <grass/glocale.h>
#include "global.h"

int 
main (int argc, char *argv[])
{
    int n;
    struct Map_info Map;
    struct GModule *module;
    struct field_info *Fi;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector, database, attribute table");
    module->description = _("Populate database values from vector features.");

    parse_command_line (argc, argv);

    G_begin_distance_calculations();
    G_begin_polygon_area_calculations();

    /* open map */
    Vect_set_open_level (2);
    Vect_open_old(&Map,options.name,options.mapset);

    Fi = Vect_get_field ( &Map, options.field);

    if ( !options.print && Fi == NULL ) {
         G_fatal_error(_("Database connection not defined for layer %d. Use v.db.connect first."),
		       options.field);
    }

    /* allocate array for values */
    /* (+ 1 is for cat -1 (no category) reported at the end ) */
    if (Vect_cidx_get_field_index(&Map, options.field) > -1) {
	n = Vect_cidx_get_num_unique_cats_by_index (&Map,
						    Vect_cidx_get_field_index(&Map, options.field));
    }
    else {
	n = 0;
    }
    G_debug ( 2, "%d unique cats", n );
    Values = (VALUE *) G_calloc ( n + 1, sizeof ( VALUE ) );
    vstat.rcat = 0;

    /* Read values from map */
    if ( options.option == O_QUERY ){
	query(&Map);
    } else if ( ( options.option == O_AREA ) || ( options.option == O_COMPACT ) ||
		( options.option == O_PERIMETER ) || ( options.option == O_FD ) ){
	read_areas(&Map);
    } else { 
        read_lines(&Map); 
    }		

    conv_units();
    
    if ( options.print ) {
	report();
    } else {
	update( &Map );
	Vect_set_db_updated ( &Map );
    }
    
    Vect_close (&Map);

    /* free list */
    G_free ( Values );

    if ( !(options.print && options.total) ) {
        print_stat();
    }

    exit(EXIT_SUCCESS);
}
