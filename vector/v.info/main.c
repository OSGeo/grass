/***************************************************************
 *
 * MODULE:       v.info
 * 
 * AUTHOR(S):    CERL, updated to 5.7 by Markus Neteler
 *               Update to 7.0 by Martin Landa <landa.martin gmail.com> (2009)
 *               Support for level 1 by Markus Metz (2009)
 *               
 * PURPOSE:      Print vector map info
 *               
 * COPYRIGHT:    (C) 2002-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;

    char *input_opt, *field_opt;
    int hist_flag, col_flag, reg_flag, topo_flag, title_flag;
    
    struct Map_info Map;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("metadata"));
    G_add_keyword(_("history"));
    module->description =
	_("Outputs basic information about a vector map.");

    G_debug(1,"LFS is %s", sizeof(off_t) == 8 ? "available" : "not available");
    
    parse_args(argc, argv,
	       &input_opt, &field_opt,
	       &hist_flag, &col_flag, &reg_flag, &topo_flag, &title_flag);

     /* try to open head-only on level 2 */
    if (Vect_open_old_head2(&Map, input_opt, "", field_opt) < 2) {
	/* force level 1, open fully
	 * NOTE: number of points, lines, boundaries, centroids, faces, kernels is still available */
	Vect_close(&Map);
	Vect_set_open_level(1); /* no topology */
	if (Vect_open_old2(&Map, input_opt, "", field_opt) < 1)
	    G_fatal_error(_("Unable to open vector map <%s>"), Vect_get_full_name(&Map));

	/* level one info not needed for history, title, columns */
	if (!hist_flag && !title_flag && !col_flag)
	    level_one_info(&Map);
    }

    if (hist_flag) {
	char buf[1001];
	
	Vect_hist_rewind(&Map);
	while (Vect_hist_read(buf, 1000, &Map) != NULL) {
	    fprintf(stdout, "%s\n", buf);
	}
    }
    else if (title_flag) {
	fprintf(stdout, "%s\n", Vect_get_map_name(&Map));
    }
    else if (reg_flag || topo_flag) {
	if (reg_flag) {
	    print_region(&Map);
	}
	if (topo_flag) {
	    print_topo(&Map);
	}
    }
    else {
	if (col_flag) {
	    print_columns(&Map, input_opt, field_opt);
	}
	else {
	    print_info(&Map);
	}
    }

    Vect_close(&Map);

    return (EXIT_SUCCESS);
}
