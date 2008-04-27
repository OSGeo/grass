/****************************************************************************
 *
 * MODULE:       r.patch
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Hamish Bowman <hamish_nospam yahoo.com>, Markus Neteler <neteler itc.it>
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
#include <grass/glocale.h>
#include "nfiles.h"
#include "local_proto.h"

int main (int argc, char *argv[])
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
    int ok;
    int row,nrows,ncols;
    int ZEROFLAG;
    char *new_name;
    char **names;
    char **ptr; 

    struct GModule *module;
    struct Flag *zeroflag;
    struct Option *opt1, *opt2 ;

    G_gisinit (argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Creates a composite raster map layer by using "
	  "known category values from one (or more) map layer(s) "
	  "to fill in areas of \"no data\" in another map layer.");

    /* Define the different options */

    opt1 = G_define_standard_option(G_OPT_R_INPUTS);
    opt1->description= _("Name of raster maps to be patched together");

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt2->description= _("Name for resultant raster map");

    /* Define the different flags */

    zeroflag = G_define_flag() ;
    zeroflag->key         = 'z' ;
    zeroflag->description = _("Use zero (0) for transparency instead of NULL") ;

    ZEROFLAG = 0; /* default: use NULL for transparency */

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    ZEROFLAG= (zeroflag->answer);
    
    ok = 1;
    names = opt1->answers;
    
    out_type = CELL_TYPE;

    for (ptr = names, nfiles = 0; *ptr != NULL; ptr++, nfiles++)
	    ;

    if (nfiles < 2)
        G_fatal_error(_("The minimum number of input raster maps is two"));

    infd = G_malloc(nfiles * sizeof(int));
    statf = G_malloc(nfiles * sizeof(struct Cell_stats));

    for (i = 0; i < nfiles; i++)
    {
        const char *name = names[i];
        const char *mapset = G_find_cell2 (name, "");
	int fd;

        if (mapset == NULL)
        {
            G_warning (_("Raster map <%s> not found"), name);
            G_sleep(3);
            ok = 0;
        }

        if (!ok) 
            continue;

        fd = G_open_cell_old (name, mapset);
        if (fd < 0)
        {
            ok = 0;
            continue;
        }

	infd[i] = fd;

        map_type = G_get_raster_map_type(fd);
	if(map_type==FCELL_TYPE && out_type == CELL_TYPE)
	       out_type = FCELL_TYPE;
        else if(map_type==DCELL_TYPE) 
	       out_type = DCELL_TYPE;

        G_init_cell_stats (&statf[i]);
    }

    if (!ok)
        G_fatal_error(_("One or more input raster maps not found"));

    rname = opt2->answer;
    outfd = G_open_raster_new (new_name = rname, out_type);
    if (outfd < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), new_name);
    
    presult = G_allocate_raster_buf(out_type);
    patch  = G_allocate_raster_buf(out_type);

    nrows = G_window_rows();
    ncols = G_window_cols();

    G_verbose_message(_("Percent complete..."));
    for (row = 0; row < nrows; row++)
    {
	G_percent (row, nrows, 2);
	if(G_get_raster_row (infd[0], presult, row, out_type) < 0)
	    G_fatal_error(_("Unable to read raster map <%s> row %d"),
			  names[0], row);

        if(out_type == CELL_TYPE)
            G_update_cell_stats ((CELL *) presult, ncols, &statf[0]);
	for (i = 1; i < nfiles; i++)
	{
	    if(G_get_raster_row (infd[i], patch, row, out_type) < 0)
		G_fatal_error(_("Unable to read raster map <%s> row %d"),
			      names[i], row);
	    if(!do_patch (presult, patch, &statf[i], ncols, out_type, ZEROFLAG))
		break;
	}
	G_put_raster_row (outfd, presult, out_type);
    }
    G_percent (row, nrows, 2);

    G_free (patch);
    G_free (presult);
    for (i = 0; i < nfiles; i++)
	G_close_cell (infd[i]);
/* 
 * build the new cats and colors. do this before closing the new
 * file, in case the new file is one of the patching files as well.
 */
    G_message (_("Creating support files for raster map <%s>"), new_name);
    support (names, statf, nfiles, &cats, &cats_ok, &colr, &colr_ok, out_type);

/* now close (and create) the result */
    G_close_cell (outfd);
    if (cats_ok)
	G_write_cats (new_name, &cats);
    if (colr_ok)
	G_write_colors (new_name, G_mapset(), &colr);

    G_short_history(new_name, "raster", &history);
    G_command_history(&history);
    G_write_history(new_name, &history);

    exit(EXIT_SUCCESS);
}
