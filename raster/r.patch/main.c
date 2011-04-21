
/****************************************************************************
 *
 * MODULE:       r.patch
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Hamish Bowman <hamish_b yahoo.com>, Markus Neteler <neteler itc.it>
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>,
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    int *infd;
    struct Categories cats;
    struct Cell_stats *statf;
    struct Colors colr;
    int cats_ok;
    int colr_ok;
    int outfd;
    RASTER_MAP_TYPE out_type, map_type;
    struct History history;
    void *presult, *patch;
    int nfiles;
    char *rname;
    int i;
    int row, nrows, ncols;
    int ZEROFLAG;
    char *new_name;
    char **names;
    char **ptr;

    struct GModule *module;
    struct Flag *zeroflag;
    struct Option *opt1, *opt2;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("geometry"));
    module->description =
	_("Creates a composite raster map layer by using "
	  "known category values from one (or more) map layer(s) "
	  "to fill in areas of \"no data\" in another map layer.");

    /* Define the different options */

    opt1 = G_define_standard_option(G_OPT_R_INPUTS);
    opt1->description = _("Name of raster maps to be patched together");

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt2->description = _("Name for resultant raster map");

    /* Define the different flags */

    zeroflag = G_define_flag();
    zeroflag->key = 'z';
    zeroflag->description =
	_("Use zero (0) for transparency instead of NULL");

    ZEROFLAG = 0;		/* default: use NULL for transparency */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    ZEROFLAG = (zeroflag->answer);

    names = opt1->answers;

    out_type = CELL_TYPE;

    for (ptr = names, nfiles = 0; *ptr != NULL; ptr++, nfiles++) ;

    if (nfiles < 2)
	G_fatal_error(_("The minimum number of input raster maps is two"));

    infd = G_malloc(nfiles * sizeof(int));
    statf = G_malloc(nfiles * sizeof(struct Cell_stats));

    for (i = 0; i < nfiles; i++) {
	const char *name = names[i];
	int fd;

	fd = Rast_open_old(name, "");

	infd[i] = fd;

	map_type = Rast_get_map_type(fd);
	if (map_type == FCELL_TYPE && out_type == CELL_TYPE)
	    out_type = FCELL_TYPE;
	else if (map_type == DCELL_TYPE)
	    out_type = DCELL_TYPE;

	Rast_init_cell_stats(&statf[i]);
    }

    rname = opt2->answer;
    outfd = Rast_open_new(new_name = rname, out_type);

    presult = Rast_allocate_buf(out_type);
    patch = Rast_allocate_buf(out_type);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    G_verbose_message(_("Percent complete..."));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	Rast_get_row(infd[0], presult, row, out_type);

	if (out_type == CELL_TYPE)
	    Rast_update_cell_stats((CELL *) presult, ncols, &statf[0]);
	for (i = 1; i < nfiles; i++) {
	    Rast_get_row(infd[i], patch, row, out_type);
	    if (!do_patch
		(presult, patch, &statf[i], ncols, out_type, ZEROFLAG))
		break;
	}
	Rast_put_row(outfd, presult, out_type);
    }
    G_percent(row, nrows, 2);

    G_free(patch);
    G_free(presult);
    for (i = 0; i < nfiles; i++)
	Rast_close(infd[i]);
    /* 
     * build the new cats and colors. do this before closing the new
     * file, in case the new file is one of the patching files as well.
     */
    G_message(_("Creating support files for raster map <%s>"), new_name);
    support(names, statf, nfiles, &cats, &cats_ok, &colr, &colr_ok, out_type);

    /* now close (and create) the result */
    Rast_close(outfd);
    if (cats_ok)
	Rast_write_cats(new_name, &cats);
    if (colr_ok)
	Rast_write_colors(new_name, G_mapset(), &colr);

    Rast_short_history(new_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(new_name, &history);

    exit(EXIT_SUCCESS);
}
