/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from 
 *               multiple raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#define GLOBAL
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "glob.h"
#include "local_proto.h"
#include <grass/glocale.h>

static int cmp(const void *, const void *);


int 
main (int argc, char *argv[])
{
    int fd[NFILES];
    int outfd;
    int i;
    char *name;
    char *output;
    char *mapset;
    int non_zero;
    struct Range range;
    CELL ncats, max_cats;
    int primary;
    struct Categories pcats;
    struct Colors pcolr;
    char buf[1024];
    CELL result;
    CELL cross();
    struct GModule *module;
    struct
    {
	struct Option *input, *output;
    } parm;
    struct
    {
	struct Flag *z;
    } flag;

    /* please, remove before GRASS 7 released */
    struct Flag *q_flag;

    G_gisinit (argv[0]);

/* Define the different options */

	module = G_define_module();
	module->keywords = _("raster");
    module->description =
		_("Creates a cross product of the category values from "
		"multiple raster map layers.");
			
    parm.input = G_define_option() ;
    parm.input->key        = "input";
    parm.input->type       = TYPE_STRING;
    parm.input->required   = YES;
    parm.input->multiple   = YES;	
    parm.input->gisprompt  = "old,cell,raster" ;
    sprintf(parm.input->description= G_malloc(60),
	_("Names of 2-%d input raster maps"), NFILES);

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

/* Define the different flags */

    flag.z = G_define_flag() ;
    flag.z->key         = 'z' ;
    flag.z->description = _("Non-zero data only") ;

    /* please, remove before GRASS 7 released */
    q_flag = G_define_flag() ;
    q_flag->key         = 'q' ;  
    q_flag->description = _("Run quietly") ;

    if (G_parser(argc, argv))
	exit (EXIT_FAILURE);

    if(q_flag->answer) {
        G_putenv("GRASS_VERBOSE","0");
        G_warning(_("The '-q' flag is superseded and will be removed "
            "in future. Please use '--quiet' instead."));
    }

    nrows = G_window_rows();
    ncols = G_window_cols();

    nfiles = 0;
    non_zero = 0;
  

    non_zero = flag.z->answer;

    for (nfiles = 0; (name = parm.input->answers[nfiles]); nfiles++)
    {
        if (nfiles >= NFILES)
            G_fatal_error (_("More than %d files not allowed"), NFILES);
        mapset = G_find_cell2 (name, "");
        if (!mapset)
            G_fatal_error (_("Raster map <%s> not found"), name);
        names[nfiles] = name;
        fd[nfiles] = G_open_cell_old (name, mapset);
        if (fd[nfiles] < 0)
            G_fatal_error (_("Unable to open raster map <%s>"), name);
        G_read_range (name, mapset, &range);
        ncats = range.max - range.min;

        if (nfiles == 0 || ncats > max_cats)
        {
            primary = nfiles;
            max_cats = ncats;
        }
    }
   
    if (nfiles <= 1)
	G_fatal_error (_("Must specify 2 or more input maps"));
    output = parm.output->answer; 
    outfd = G_open_cell_new (output);

    if (outfd < 0)
	G_fatal_error (_("Unable to create raster map <%s>"), parm.output->answer);

    sprintf (buf, "Cross of %s", names[0]);
    for (i = 1; i < nfiles-1; i++)
    {
	strcat (buf, ", ");
	strcat (buf, names[i]);
    }
    strcat (buf, " and ");
    strcat (buf, names[i]);
    G_init_cats ((CELL) 0, buf, &pcats);

/* first step is cross product, but un-ordered */
    result = cross (fd, non_zero, primary, outfd);

/* print message STEP mesage */
    G_message (_("%s: STEP 2 ..."),G_program_name());

/* now close all files */
    for (i = 0; i < nfiles; i++)
	G_close_cell (fd[i]);
    G_close_cell (outfd);

    if (result <= 0)
	exit(0);


/* build the renumbering/reclass and the new cats file */
    qsort (reclass, result+1, sizeof(RECLASS), cmp);
    table = (CELL *) G_calloc (result+1, sizeof(CELL));
    for (i =0; i < nfiles; i++)
    {
	mapset = G_find_cell (names[i], "");
	G_read_cats (names[i], mapset, &labels[i]);
    }

    for (ncats = 0; ncats <= result; ncats++)
    {
	table[reclass[ncats].result] = ncats;
	set_cat (ncats, reclass[ncats].cat, &pcats);
    }

    for (i = 0; i < nfiles; i++)
	G_free_cats (&labels[i]);

/* reopen the output cell for reading and for writing */
    fd[0] = G_open_cell_old (output, G_mapset());
    outfd = G_open_cell_new (output);

    renumber (fd[0], outfd);

    G_message (_("Creating support files for <%s>..."), output);
    G_close_cell (fd[0]);
    G_close_cell (outfd);
    G_write_cats (output, &pcats);
    G_free_cats (&pcats);
    if (result > 0)
    {
	G_make_random_colors (&pcolr, (CELL) 1, result);
	G_write_colors (output, G_mapset(), &pcolr);
    }

    G_message (_("%ld categories"), (long) result);
    exit(EXIT_SUCCESS);
}

static int cmp(const void *aa, const void *bb)
{
    const RECLASS *a = aa, *b = bb;
    int i;

    for (i = 0; i < (nfiles + 2); i++)
    {
	if (a->cat[i] < b->cat[i]) return -1;
	if (a->cat[i] > b->cat[i]) return 1;
    }
    return 0;
}
