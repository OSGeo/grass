
/****************************************************************************
 *
 * MODULE:       r.report
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Roberto Flor <flor itc.it>
 *               Jachym Cepicky <jachym les-ejk.cz>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 *               Sort option by Martin Landa <landa.martin gmail.com>
 * PURPOSE:      Reports statistics for raster map(s).
 * COPYRIGHT:    (C) 1999-2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/glocale.h>
#include "global.h"

struct Cell_head window;

LAYER *layers;
int nlayers;

GSTATS *Gstats;
int nstats;

UNITS unit[MAX_UNITS];
int nunits;

int page_width;
int page_length;
int masking = 1;
int use_formfeed;
int nlines;
int with_headers = 1;
int e_format;
int no_nulls;
int no_nulls_all;
int do_sort = SORT_DEFAULT;

char *stats_file;
char *no_data_str;
int stats_flag;
int nsteps, cat_ranges, as_int;
int *is_fp;
DCELL *DMAX, *DMIN;

int maskfd;
CELL *mask;
CELL NULL_CELL;
int (*get_row)();

char fs[2];
struct Categories *labels;

int main(int argc, char *argv[])
{
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    module->description = _("Reports statistics for raster maps.");

    parse_command_line(argc, argv);

    G_get_window(&window);

    get_stats();

    report();

    exit(EXIT_SUCCESS);
}
