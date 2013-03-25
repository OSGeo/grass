
/****************************************************************************
 *
 * MODULE:       r3.mkdspf
 * AUTHOR(S):    Helena Mitasova and Bill Brown  (original contributor)
 *               Roberto Flor <flor itc.it>, Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Radim Blazek <radim.blazek gmail.com>, Markus Neteler <neteler itc.it>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2000-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* This program implements the marching cubes surface tiler described by
 * Lorenson & Cline in the Siggraph 87 Conference Proceedings.
 *
 * This program reads in data from a grid3 formatted file containing 3D data
 * and creates a display file.  
 * 
 * The display file consists of cell_info structures.
 * The cell_info structure:
 *      threshold (specified on commandline)
 *      number of polygons 
 *      polygon vertice coordinates
 *      surface or vertex normals (depending on lighting model specified)
 * 
 * The user must specify the data file name and the thresholds and lighting
 * model desired.  
 *
 * Based on a program outline written by Mike Krogh, NCSA, Feb.,1990
 *
 * written by Jan Moorman for D. Lawrance, Mednet NCSA.
 * rewritten for CERL, August 1990
 *
 * rewritten by Bill Brown
 *     for UI Geographic Modeling Systems Laboratory February 1996
 *     to use new GRASS 3dgrid data files & API
 */

#include <stdlib.h>
#include <math.h>
#include "vizual.h"
#include <grass/gis.h>
#include <grass/raster3d.h>
#include "local_proto.h"
#include <grass/glocale.h>

file_info Headfax;	/* contains info about command line */
Cube_data CUBE;		/* and the data for a single cube */
int NTHRESH;

int main(int argc, char *argv[])
{
    char element[GNAME_MAX+10];
    const char *dspout;
    void *g3map;
    RASTER3D_Region g3reg;
    const char *mapset;
    double dmin, dmax;
    struct GModule *module;

    struct Option *levels;
    struct Option *out;
    struct Option *min;
    struct Option *max;
    struct Option *step;
    struct Option *tnum;
    struct Option *name;

    struct Flag *shade;
    struct Flag *quiet;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("voxel"));
    module->description =
	_("Creates a display file from an existing grid3 file according to specified threshold levels.");

    name = G_define_option();
    name->key = "input";
    name->type = TYPE_STRING;
    name->required = YES;
    name->gisprompt = "old,grid3,3dcell";
    /* should still find the DIRECTORY */
    name->description = _("Name of an existing 3d raster map");

    out = G_define_standard_option(G_OPT_F_OUTPUT);
    out->key = "dspf";
    out->required = YES;
    out->description = _("Name for output display file");

    levels = G_define_option();
    levels->key = "levels";
    levels->type = TYPE_DOUBLE;
    levels->required = NO;
    levels->multiple = YES;
    levels->description = _("List of thresholds for isosurfaces");

    min = G_define_option();
    min->key = "min";
    min->type = TYPE_DOUBLE;
    min->required = NO;
    min->description = _("Minimum isosurface level");

    max = G_define_option();
    max->key = "max";
    max->type = TYPE_DOUBLE;
    max->required = NO;
    max->description = _("Maximum isosurface level");

    step = G_define_option();
    step->key = "step";
    step->type = TYPE_DOUBLE;
    step->required = NO;
    step->description = _("Positive increment between isosurface levels");

    tnum = G_define_option();
    tnum->key = "tnum";
    tnum->type = TYPE_INTEGER;
    tnum->required = NO;
    tnum->answer = "7";
    tnum->description = _("Number of isosurface threshold levels");

    quiet = G_define_flag();
    quiet->key = 'q';
    quiet->description = _("Suppress progress report & min/max information");

    shade = G_define_flag();
    shade->key = 'f';
    shade->description = _("Use flat shading rather than gradient");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Rast3d_init_defaults();

    Rast3d_get_window(&g3reg);
    G_message(_("Region from getWindow: %d %d %d"),
	      g3reg.rows, g3reg.cols, g3reg.depths);

    if (NULL ==
	(dspout =
	 check_get_any_dspname(out->answer, name->answer, G_mapset())))
	exit(EXIT_FAILURE);

    Rast3d_set_error_fun(Rast3d_print_error);

    /* open g3 file for reading and writing */
    if (NULL == (mapset = G_find_file2("grid3", name->answer, "")))
	G_fatal_error(_("Not able to find grid3 file for [%s]"),
		      name->answer);

    g3map = Rast3d_open_cell_old(name->answer, mapset, &g3reg,
			    RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);
    /*
       g3map = Rast3d_open_cell_old (name->answer, mapset, RASTER3D_DEFAULT_WINDOW,
       RASTER3D_TILE_SAME_AS_FILE,
       RASTER3D_USE_CACHE_DEFAULT);
     */

    if (NULL == g3map)
	G_fatal_error(_("Unable to open 3D raster map <%s>"), name->answer);

    if (0 == Rast3d_range_load(g3map))
	G_fatal_error(_("Unable to read range of 3D raster map <%s>"), name->answer);

    /* TODO: look at this - should use current 3dregion rather than
       region represented by original 3dgrid file */
    /*
       Rast3d_get_region_struct_map (g3map, &g3reg);
     */

    /* DONT USE Headfax any more ?
       g3read_header(&Headfax);
     */
    Rast3d_range_min_max(g3map, &dmin, &dmax);
    viz_make_header(&Headfax, dmin, dmax, &g3reg);

    /* puts command line options into cmndln_info structure */
    /* need to send it answers */
    viz_calc_tvals(&Headfax.linefax, levels->answers, min->answer,
		   max->answer, step->answer, tnum->answer,
		   quiet->answer ? 1 : 0);

    if (shade->answer) {
	Headfax.linefax.litmodel = 1;	/* flat */
    }
    else
	Headfax.linefax.litmodel = 2;	/* gradient */

    /* open display file for writing */
    sprintf(element, "grid3/%s/dsp", name->answer);
    if ((Headfax.dspfoutfp = G_fopen_new(element, dspout)) == NULL)
	G_fatal_error(_("Unable to open display file <%s>"), dspout);

    /* write display file header info */
    /* have to adjust dimensions  -dpg */
    {
	Headfax.xdim -= 1;
	Headfax.ydim -= 1;
	Headfax.zdim -= 1;

	G_message("DSPF DIMS: %d %d %d", Headfax.ydim + 1, Headfax.xdim + 1,
		  Headfax.zdim + 1);
	if (dfwrite_header(&Headfax) < 0) {
	    fclose(Headfax.dspfoutfp);
	    exit(EXIT_FAILURE);
	}
	Headfax.xdim += 1;
	Headfax.ydim += 1;
	Headfax.zdim += 1;
    }

    if (!quiet->answer)
	G_message(_("Writing %s from %s..."), dspout, name->answer);

    viz_iso_surface(g3map, &g3reg, &Headfax.linefax, quiet->answer ? 1 : 0);

    if (!quiet->answer)
	fprintf(stderr, "\n");

    /* tries to write a header! */
    Rast3d_close(g3map);

    fclose(Headfax.dspfoutfp);

    exit(EXIT_SUCCESS);
}
