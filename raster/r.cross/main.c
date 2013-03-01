
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "glob.h"
#include "local_proto.h"
#include <grass/raster.h>
#include <grass/glocale.h>

int nfiles;
int nrows;
int ncols;
const char *names[NFILES];
struct Categories labels[NFILES];
RECLASS *reclass;
CELL *table;

static int cmp(const void *, const void *);


int main(int argc, char *argv[])
{
    int fd[NFILES];
    int outfd;
    int i;
    const char *name;
    const char *output;
    const char *mapset;
    int non_zero;
    struct Range range;
    CELL ncats, max_cats;
    int primary;
    struct Categories pcats;
    struct Colors pcolr;
    char buf[1024];
    CELL result;
    struct GModule *module;
    struct
    {
	struct Option *input, *output;
    } parm;
    struct
    {
	struct Flag *z;
    } flag;

    G_gisinit(argv[0]);

    /* Define the different options */

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    module->description =
	_("Creates a cross product of the category values from "
	  "multiple raster map layers.");

    parm.input = G_define_option();
    parm.input->key = "input";
    parm.input->type = TYPE_STRING;
    parm.input->required = YES;
    parm.input->multiple = YES;
    parm.input->gisprompt = "old,cell,raster";
    sprintf(buf, _("Names of 2-%d input raster maps"), NFILES);
    parm.input->description = G_store(buf);

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    /* Define the different flags */

    flag.z = G_define_flag();
    flag.z->key = 'z';
    flag.z->description = _("Non-zero data only");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    nfiles = 0;
    non_zero = flag.z->answer;

    for (nfiles = 0; (name = parm.input->answers[nfiles]); nfiles++) {
	if (nfiles >= NFILES)
	    G_fatal_error(_("More than %d files not allowed"), NFILES);
	mapset = G_find_raster2(name, "");
	if (!mapset)
	    G_fatal_error(_("Raster map <%s> not found"), name);
	names[nfiles] = name;
	fd[nfiles] = Rast_open_old(name, mapset);
	Rast_read_range(name, mapset, &range);
	ncats = range.max - range.min;

	if (nfiles == 0 || ncats > max_cats) {
	    primary = nfiles;
	    max_cats = ncats;
	}
    }

    if (nfiles <= 1)
	G_fatal_error(_("Must specify 2 or more input maps"));
    output = parm.output->answer;
    outfd = Rast_open_c_new(output);

    sprintf(buf, "Cross of %s", names[0]);
    for (i = 1; i < nfiles - 1; i++) {
	strcat(buf, ", ");
	strcat(buf, names[i]);
    }
    strcat(buf, " and ");
    strcat(buf, names[i]);
    Rast_init_cats(buf, &pcats);

    /* first step is cross product, but un-ordered */
    result = cross(fd, non_zero, primary, outfd);

    /* print message STEP mesage */
    G_message(_("%s: STEP 2 ..."), G_program_name());

    /* now close all files */
    for (i = 0; i < nfiles; i++)
	Rast_close(fd[i]);
    Rast_close(outfd);

    if (result <= 0)
	exit(0);


    /* build the renumbering/reclass and the new cats file */
    qsort(reclass, result + 1, sizeof(RECLASS), cmp);
    table = (CELL *) G_calloc(result + 1, sizeof(CELL));
    for (i = 0; i < nfiles; i++) {
	mapset = G_find_raster2(names[i], "");
	Rast_read_cats(names[i], mapset, &labels[i]);
    }

    for (ncats = 0; ncats <= result; ncats++) {
	table[reclass[ncats].result] = ncats;
	set_cat(ncats, reclass[ncats].cat, &pcats);
    }

    for (i = 0; i < nfiles; i++)
	Rast_free_cats(&labels[i]);

    /* reopen the output cell for reading and for writing */
    fd[0] = Rast_open_old(output, G_mapset());
    outfd = Rast_open_c_new(output);

    renumber(fd[0], outfd);

    G_message(_("Creating support files for <%s>..."), output);
    Rast_close(fd[0]);
    Rast_close(outfd);
    Rast_write_cats(output, &pcats);
    Rast_free_cats(&pcats);
    if (result > 0) {
	Rast_make_random_colors(&pcolr, (CELL) 1, result);
	Rast_write_colors(output, G_mapset(), &pcolr);
    }

    G_message(_("%ld categories"), (long)result);
    exit(EXIT_SUCCESS);
}

static int cmp(const void *aa, const void *bb)
{
    const RECLASS *a = aa, *b = bb;
    int i;

    for (i = 0; i < (nfiles + 2); i++) {
	if (a->cat[i] < b->cat[i])
	    return -1;
	if (a->cat[i] > b->cat[i])
	    return 1;
    }
    return 0;
}
