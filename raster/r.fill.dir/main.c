/*
 *
 *****************************************************************************
 *
 * MODULE:       r.fill.dir
 * AUTHOR(S):    Original author unknown - Raghavan Srinivasan Nov, 1991
 *               (srin@ecn.purdue.edu) Agricultural Engineering, 
 *               Purdue University
 *               Markus Neteler: update to FP (C-code)
 *                             : update to FP (Fortran)
 *               Roger Miller: rewrite all code in C, complient with GRASS 5
 * PURPOSE:      fills a DEM to become a depression-less DEM
 *               This creates two layers from a user specified elevation map.
 *               The output maps are filled elevation or rectified elevation
 *               map and a flow direction map based on one of the type
 *               specified. The filled or rectified elevation map generated
 *               will be filled for depression, removed any circularity or
 *               conflict flow direction is resolved. This program helps to
 *               get a proper elevation map that could be used for
 *               delineating watershed using r.watershed module. However, the
 *               boundaries may have problem and could be resolved using
 *               the cell editor d.rast.edit
 *               Options have been added to produce a map of undrained areas
 *               and to run without filling undrained areas except single-cell
 *               pits.  Not all problems can be solved in a single pass.  The
 *               program can be run repeatedly, using the output elevations from
 *               one run as input to the next run until all problems are 
 *               resolved.
 * COPYRIGHT:    (C) 2001, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* for using the "open" statement */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* for using the close statement */
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define DEBUG
#include "tinf.h"
#include "local.h"

static int dir_type(int type, int dir);

int main(int argc, char **argv)
{
    int fe, fd, fm;
    int i, j, type;
    int new_id;
    int nrows, ncols, nbasins;
    int map_id, dir_id, bas_id;
    char map_name[GNAME_MAX], new_map_name[GNAME_MAX];
    const char *tempfile1, *tempfile2, *tempfile3;
    char dir_name[GNAME_MAX];
    char bas_name[GNAME_MAX];

    struct Cell_head window;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *opt4, *opt5;
    struct Flag *flag1;
    int in_type, bufsz;
    void *in_buf;
    CELL *out_buf;
    struct band3 bnd, bndC;
    struct Colors colors; 

    /*  Initialize the GRASS environment variables */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->description =
	_("Filters and generates a depressionless elevation map and a "
	  "flow direction map from a given elevation raster map.");
    
    opt1 = G_define_standard_option(G_OPT_R_ELEV);
    opt1->key = "input";
    
    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt2->description = _("Name for output depressionless elevation raster map");
    
    opt4 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt4->key = "outdir";
    opt4->description = _("Name for output flow direction map for depressionless elevation raster map");

    opt5 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt5->key = "areas";
    opt5->required = NO;
    opt5->description = _("Name for output raster map of problem areas");

    opt3 = G_define_option();
    opt3->key = "format";
    opt3->type = TYPE_STRING;
    opt3->required = NO;
    opt3->description =
	_("Aspect direction format");
    opt3->options = "agnps,answers,grass";
    opt3->answer = "grass";
    
    flag1 = G_define_flag();
    flag1->key = 'f';
    flag1->description = _("Find unresolved areas only");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (flag1->answer && opt5->answer == NULL) {
	G_fatal_error(_("The '%c' flag requires '%s'to be specified"),
		      flag1->key, opt5->key);
    }

    type = 0;
    strcpy(map_name, opt1->answer);
    strcpy(new_map_name, opt2->answer);
    strcpy(dir_name, opt4->answer);
    if (opt5->answer != NULL)
	strcpy(bas_name, opt5->answer);

    if (strcmp(opt3->answer, "agnps") == 0)
	type = 1;
    else if (strcmp(opt3->answer, "answers") == 0)
	type = 2;
    else if (strcmp(opt3->answer, "grass") == 0)
	type = 3;
    
    G_debug(1, "output type (1=AGNPS, 2=ANSWERS, 3=GRASS): %d", type);

    if (type == 3)
	G_verbose_message(_("Direction map is D8 resolution, i.e. 45 degrees"));
    
    /* open the maps and get their file id  */
    map_id = Rast_open_old(map_name, "");
    if (Rast_read_colors(map_name, "", &colors) < 0)
        G_warning(_("Unable to read color table for raster map <%s>"), map_name);
    
    /* allocate cell buf for the map layer */
    in_type = Rast_get_map_type(map_id);

    /* set the pointers for multi-typed functions */
    set_func_pointers(in_type);

    /* get the window information  */
    G_get_window(&window);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* buffers for internal use */
    bndC.ns = ncols;
    bndC.sz = sizeof(CELL) * ncols;
    bndC.b[0] = G_calloc(ncols, sizeof(CELL));
    bndC.b[1] = G_calloc(ncols, sizeof(CELL));
    bndC.b[2] = G_calloc(ncols, sizeof(CELL));

    /* buffers for external use */
    bnd.ns = ncols;
    bnd.sz = ncols * bpe();
    bnd.b[0] = G_calloc(ncols, bpe());
    bnd.b[1] = G_calloc(ncols, bpe());
    bnd.b[2] = G_calloc(ncols, bpe());

    in_buf = get_buf();

    tempfile1 = G_tempfile();
    tempfile2 = G_tempfile();
    tempfile3 = G_tempfile();

    fe = open(tempfile1, O_RDWR | O_CREAT, 0666);	/* elev */
    fd = open(tempfile2, O_RDWR | O_CREAT, 0666);	/* dirn */
    fm = open(tempfile3, O_RDWR | O_CREAT, 0666);	/* problems */

    G_message(_("Reading input elevation raste map..."));
    for (i = 0; i < nrows; i++) {
	G_percent(i, nrows, 2);
	get_row(map_id, in_buf, i);
	write(fe, in_buf, bnd.sz);
    }
    G_percent(1, 1, 1);
    Rast_close(map_id);

    /* fill single-cell holes and take a first stab at flow directions */
    G_message(_("Filling sinks..."));
    filldir(fe, fd, nrows, &bnd);

    /* determine flow directions for ambiguous cases */
    G_message(_("Determining flow directions for ambiguous cases..."));
    resolve(fd, nrows, &bndC);

    /* mark and count the sinks in each internally drained basin */
    nbasins = dopolys(fd, fm, nrows, ncols);
    if (!flag1->answer) {
	/* determine the watershed for each sink */
	wtrshed(fm, fd, nrows, ncols, 4);

	/* fill all of the watersheds up to the elevation necessary for drainage */
	ppupdate(fe, fm, nrows, nbasins, &bnd, &bndC);

	/* repeat the first three steps to get the final directions */
	G_message(_("Repeat to get the final directions..."));
	filldir(fe, fd, nrows, &bnd);
	resolve(fd, nrows, &bndC);
	nbasins = dopolys(fd, fm, nrows, ncols);
    }

    G_free(bndC.b[0]);
    G_free(bndC.b[1]);
    G_free(bndC.b[2]);

    G_free(bnd.b[0]);
    G_free(bnd.b[1]);
    G_free(bnd.b[2]);

    out_buf = Rast_allocate_c_buf();
    bufsz = ncols * sizeof(CELL);

    lseek(fe, 0, SEEK_SET);
    new_id = Rast_open_new(new_map_name, in_type);

    lseek(fd, 0, SEEK_SET);
    dir_id = Rast_open_new(dir_name, CELL_TYPE);

    if (opt5->answer != NULL) {
	lseek(fm, 0, SEEK_SET);
	bas_id = Rast_open_new(bas_name, CELL_TYPE);

	for (i = 0; i < nrows; i++) {
	    read(fm, out_buf, bufsz);
	    Rast_put_row(bas_id, out_buf, CELL_TYPE);
	}

	Rast_close(bas_id);
	close(fm);
    }

    G_important_message(_("Writing output raster maps..."));
    for (i = 0; i < nrows; i++) {
        G_percent(i, nrows, 5);
	read(fe, in_buf, bnd.sz);
	put_row(new_id, in_buf);

	read(fd, out_buf, bufsz);

	for (j = 0; j < ncols; j += 1)
	    out_buf[j] = dir_type(type, out_buf[j]);

	Rast_put_row(dir_id, out_buf, CELL_TYPE);
    }
    G_percent(1, 1, 1);

    /* copy color table from input */
    Rast_write_colors(new_map_name, G_mapset(), &colors);

    Rast_close(new_id);
    close(fe);
    
    Rast_close(dir_id);
    close(fd);

    G_free(in_buf);
    G_free(out_buf);

    exit(EXIT_SUCCESS);
}

static int dir_type(int type, int dir)
{
    if (type == 1) {		/* AGNPS aspect format */
	if (dir == 128)
	    return (1);
	else if (dir == 1)
	    return (2);
	else if (dir == 2)
	    return (3);
	else if (dir == 4)
	    return (4);
	else if (dir == 8)
	    return (5);
	else if (dir == 16)
	    return (6);
	else if (dir == 32)
	    return (7);
	else if (dir == 64)
	    return (8);
	else
	    return (dir);
    }

    else if (type == 2) {	/* ANSWERS aspect format */
	if (dir == 128)
	    return (90);
	else if (dir == 1)
	    return (45);
	else if (dir == 2)
	    return (360);
	else if (dir == 4)
	    return (315);
	else if (dir == 8)
	    return (270);
	else if (dir == 16)
	    return (225);
	else if (dir == 32)
	    return (180);
	else if (dir == 64)
	    return (135);
	else
	    return (dir);
    }

    else {			/* [new] GRASS aspect format */
	if (dir == 128)
	    return (90);
	else if (dir == 1)
	    return (45);
	else if (dir == 2)
	    return (360);
	else if (dir == 4)
	    return (315);
	else if (dir == 8)
	    return (270);
	else if (dir == 16)
	    return (225);
	else if (dir == 32)
	    return (180);
	else if (dir == 64)
	    return (135);
	else
	    return (dir);
    }

}
