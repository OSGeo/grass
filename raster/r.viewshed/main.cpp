/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu
 *
 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *               Markus Metz: surface interpolation
 *
 * Date:         July 2008; April 2011 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The terrain is NOT viewed as a tesselation of flat cells, 
 * i.e. if the line-of-sight does not pass through the cell center, 
 * elevation is determined using bilinear interpolation.
 * The viewshed algorithm is efficient both in
 * terms of CPU operations and I/O operations. It has worst-case
 * complexity O(n lg n) in the RAM model and O(sort(n)) in the
 * I/O-model.  For the algorithm and all the other details see the
 * paper: "Computing Visibility on * Terrains in External Memory" by
 * Herman Haverkort, Laura Toma and Yi Zhuang.
 *
 * COPYRIGHT: (C) 2008-2011 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}
#include "grass.h"
#include <grass/iostream/ami.h>

#include "viewshed.h"
#include "visibility.h"
#include "grid.h"
#include "rbbst.h"
#include "statusstructure.h"
#include "distribute.h"




/* if the user does not specify how much memory is available for the
   program, this is the default value used (in bytes) */
#define DEFAULT_MEMORY 500<<20


/* observer elevation above the terrain */
#define DEFAULT_OBS_ELEVATION 0


/* All these flags are used for debugging */

/* if this flag is set, it always runs in memory */
//#define FORCE_INTERNAL

/* if this is set, it runs in external memory, even if the problem is
   small enough to fit in memory.  In external memory it first tries
   to run the base-case, then recursion. */
//#define FORCE_EXTERNAL

/* if this flag is set it runs in external memory, and starts
   recursion without checking the base-case. */
//#define FORCE_DISTRIBUTION



/* ------------------------------------------------------------ */
/* forward declarations */
/* ------------------------------------------------------------ */
void print_timings_internal(Rtimer sweepTime, Rtimer outputTime,
			    Rtimer totalTime);
void print_timings_external_memory(Rtimer totalTime, Rtimer viewshedTime,
				   Rtimer outputTime, Rtimer sortOutputTime);

void parse_args(int argc, char *argv[], int *vpRow, int *vpCol,
		ViewOptions * viewOptions, long long *memSizeBytes,
		Cell_head * window);




/* ------------------------------------------------------------ */
int main(int argc, char *argv[])
{

    /* GRASS initialization stuff */
    struct GModule *module;

    /*initialize GIS environment */
    G_gisinit(argv[0]);

    /*initialize module */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("viewshed"));
    G_add_keyword(_("line of sight"));
    module->label = _("Computes the viewshed of a point on an elevation raster map.");
    module->description = _("Default format: NULL (invisible), vertical angle wrt viewpoint (visible).");

    struct Cell_head region;

    Rast_get_window(&region);


    /* ************************************************************ */
    /* parameters set up */
    long long memSizeBytes = DEFAULT_MEMORY;

    /* the maximum size of main memory that the program ca use. The
       user can specify it, otherwise the default value of 500MB is
       used.  The program uses this value to decied in which mode to
       run --- in internal memory, or external memory.  */

    int vpRow, vpCol;

    /* the coordinates of the viewpoint in the raster; right now the
       algorithm assumes that the viewpoint is inside the grid, though
       this is not necessary; some changes will be needed to make it
       work with a viewpoint outside the terrain */

    ViewOptions viewOptions;

    //viewOptions.inputfname = (char*)malloc(500); 
    //viewOptions.outputfname = (char*)malloc(500);
    //assert(inputfname && outputfname);
    viewOptions.obsElev = DEFAULT_OBS_ELEVATION;
    viewOptions.maxDist = INFINITY_DISTANCE;
    viewOptions.outputMode = OUTPUT_ANGLE;
    viewOptions.doCurv = FALSE;
    viewOptions.doRefr = FALSE;
    viewOptions.refr_coef = 1.0/7.0;

    parse_args(argc, argv, &vpRow, &vpCol, &viewOptions, &memSizeBytes,
	       &region);

    /* set viewpoint with the coordinates specified by user. The
       height of the viewpoint is not known at this point---it will be
       set during the execution of the algorithm */
    Viewpoint vp;

    set_viewpoint_coord(&vp, vpRow, vpCol);


    /* ************************************************************ */
    /* set up the header of the raster with all raster info and make
       sure the requested viewpoint is on the map */
    GridHeader *hd;

    hd = read_header(viewOptions.inputfname, &region);
    assert(hd);
    G_get_set_window(&(hd->window));

    /* LT: there is no need to exit if viewpoint is outside grid,
       the algorithm will work correctly in theory. But this
       requires some changes. To do. */
    if (!(vp.row < hd->nrows && vp.col < hd->ncols)) {
	G_warning(_("Viewpoint outside grid"));
	G_warning(_("viewpont: (row=%d, col=%d)"), vp.row, vp.col);
	G_fatal_error(_("grid: (rows=%d, cols=%d)"), hd->nrows, hd->ncols);
    }


    /* set curvature params */
    viewOptions.cellsize = region.ew_res;
    double e2;

    G_get_ellipsoid_parameters(&viewOptions.ellps_a, &e2);
    if (viewOptions.ellps_a == 0) {
	/*according to r.los, this can be
	   problematic, so we'll have a backup, hardcoded radius :-( */
	G_warning(_("Problems obtaining current ellipsoid parameters, using sphere (6370997.0)"));
	viewOptions.ellps_a = 6370997.00;
    }

    G_begin_distance_calculations();




    /* ************************************************************ */
    /* decide whether the computation of the viewshed will take place
       in-memory or in external memory */
    int IN_MEMORY;
    long long inmemSizeBytes = get_viewshed_memory_usage(hd);

    G_verbose_message(_("In-memory memory usage is %lld B (%d MB), \
			max mem allowed=%lld B(%dMB)"), inmemSizeBytes,
			(int)(inmemSizeBytes >> 20), memSizeBytes,
			(int)(memSizeBytes >> 20));
    if (inmemSizeBytes < memSizeBytes) {
	IN_MEMORY = 1;
	G_verbose_message("*****  IN_MEMORY MODE  *****");
    }
    else {
	G_verbose_message("*****  EXTERNAL_MEMORY MODE  *****");
	IN_MEMORY = 0;
    }

    /* the mode can be forced to in memory or external if the user
       wants to test or debug a specific mode  */
#ifdef FORCE_EXTERNAL
    IN_MEMORY = 0;
    G_debug(1, "FORCED EXTERNAL");
#endif

#ifdef FORCE_INTERNAL
    IN_MEMORY = 1;
    G_debug(1, "FORCED INTERNAL");
#endif


    /* ************************************************************ */
    /* compute viewshed in memory */
    /* ************************************************************ */
    if (IN_MEMORY) {
	/*//////////////////////////////////////////////////// */
	/*/viewshed in internal  memory */
	/*//////////////////////////////////////////////////// */
	Rtimer totalTime, outputTime, sweepTime;
	MemoryVisibilityGrid *visgrid;

	rt_start(totalTime);

	/*compute the viewshed and store it in visgrid */
	rt_start(sweepTime);
	visgrid =
	    viewshed_in_memory(viewOptions.inputfname, hd, &vp, viewOptions);
	rt_stop(sweepTime);

	/* write the output */
	rt_start(outputTime);
	save_inmem_visibilitygrid(visgrid, viewOptions, vp);
	rt_stop(outputTime);

	rt_stop(totalTime);

	print_timings_internal(sweepTime, outputTime, totalTime);
    }




    /* ************************************************************ */
    /* compute viewshed in external memory */
    /* ************************************************************ */
    else {

	/* ************************************************************ */
	/* set up external memory mode */
	/* setup STREAM_DIR if not already set */
	char buf[1000];

	if (getenv(STREAM_TMPDIR) != NULL) {
	    /*if already set */
	    G_debug(1, "%s=%s", STREAM_TMPDIR, getenv(STREAM_TMPDIR));
	    G_debug(1, "Intermediate stream location: %s",
		   getenv(STREAM_TMPDIR));
	}
	else {
	    /*set it */
	    sprintf(buf, "%s=%s", STREAM_TMPDIR, viewOptions.streamdir);
	    G_debug(1, "setting %s ", buf);
	    putenv(buf);
	    if (getenv(STREAM_TMPDIR) == NULL) {
		G_fatal_error(_("%s not set"), "STREAM_TMPDIR");
		exit(1);
	    }
	    else {
		G_debug(1, "are ok.");
	    }
	}
	G_important_message(_("Intermediate files will not be deleted \
		              in case of abnormal termination."));
	G_important_message(_("Intermediate location: %s"), viewOptions.streamdir);
	G_important_message(_("To save space delete these files manually!"));


	/* initialize IOSTREAM memory manager */
	MM_manager.set_memory_limit(memSizeBytes);
	MM_manager.ignore_memory_limit();
	MM_manager.print_limit_mode();



	/* ************************************************************ */
	/* BASE CASE OR DISTRIBUTION */
	/* determine whether base-case of external algorithm is enough,
	   or recursion is necessary */
	int BASE_CASE = 0;

	if (get_active_str_size_bytes(hd) < memSizeBytes)
	    BASE_CASE = 1;

	/*if the user set the FORCE_DISTRIBUTION flag, then the
	   algorithm runs in the fuly recursive mode (even if this is
	   not necessary). This is used solely for debugging purpses  */
#ifdef FORCE_DISTRIBUTION
	BASE_CASE = 0;
#endif




	/* ************************************************************ */
	/* external memory, base case  */
	/* ************************************************************ */
	if (BASE_CASE) {
	    G_debug
		(1, "---Active structure small, starting base case---");

	    Rtimer totalTime, viewshedTime, outputTime, sortOutputTime;

	    rt_start(totalTime);

	    /*run viewshed's algorithm */
	    IOVisibilityGrid *visgrid;

	    rt_start(viewshedTime);
	    visgrid =
		viewshed_external(viewOptions.inputfname, hd, &vp,
				  viewOptions);
	    rt_stop(viewshedTime);

	    /*sort output */
	    rt_start(sortOutputTime);
	    sort_io_visibilitygrid(visgrid);
	    rt_stop(sortOutputTime);

	    /*save output stream to file. */
	    rt_start(outputTime);
	    save_io_visibilitygrid(visgrid, viewOptions, vp);
	    rt_stop(outputTime);

	    rt_stop(totalTime);

	    print_timings_external_memory(totalTime, viewshedTime,
					  outputTime, sortOutputTime);
	}



	/************************************************************/
	/* external memory, recursive distribution sweeping recursion */
	/************************************************************ */
	else {			/* if not  BASE_CASE */
#ifndef FORCE_DISTRIBUTION
	    G_debug(1, "---Active structure does not fit in memory,");
#else
	    G_debug(1, "FORCED DISTRIBUTION");
#endif

	    Rtimer totalTime, sweepTime, outputTime, sortOutputTime;

	    rt_start(totalTime);

	    /*get the viewshed solution by distribution */
	    IOVisibilityGrid *visgrid;

	    rt_start(sweepTime);
	    visgrid =
		distribute_and_sweep(viewOptions.inputfname, hd, &vp,
				     viewOptions);

	    rt_stop(sweepTime);

	    /*sort the visibility grid so that it is in order when it is
	       outputted */
	    rt_start(sortOutputTime);
	    sort_io_visibilitygrid(visgrid);
	    rt_stop(sortOutputTime);

	    rt_start(outputTime);
	    save_io_visibilitygrid(visgrid, viewOptions, vp);
	    rt_stop(outputTime);


	    rt_stop(totalTime);

	    print_timings_external_memory(totalTime, sweepTime,
					  outputTime, sortOutputTime);

	}
    }
    /*end external memory, distribution sweep */


    /**************************************/
    /*        FINISH UP, ALL CASES        */
    /**************************************/

    /*close input file and free grid header */
    G_free(hd);
    /*following GRASS's coding standards for history and exiting */
    struct History history;

    Rast_short_history(viewOptions.outputfname, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(viewOptions.outputfname, &history);
    exit(EXIT_SUCCESS);
}




/* ------------------------------------------------------------ */
/* parse arguments */
void
parse_args(int argc, char *argv[], int *vpRow, int *vpCol,
	   ViewOptions * viewOptions, long long *memSizeBytes,
	   Cell_head * window)
{

    assert(vpRow && vpCol && memSizeBytes && window);

    /* the input */
    struct Option *inputOpt;

    inputOpt = G_define_standard_option(G_OPT_R_ELEV);
    inputOpt->key = "input";

    /* the output */
    struct Option *outputOpt;

    outputOpt = G_define_standard_option(G_OPT_R_OUTPUT);
    
    /* curvature flag */
    struct Flag *curvature;

    curvature = G_define_flag();
    curvature->key = 'c';
    curvature->description =
	_("Consider the curvature of the earth (current ellipsoid)");

    /* atmospheric refraction flag */
    struct Flag *refractionFlag;

    refractionFlag = G_define_flag();
    refractionFlag->key = 'r';
    refractionFlag->description =
	_("Consider the effect of atmospheric refraction");
    refractionFlag->guisection = _("Refraction");

    /* boolean output flag */
    struct Flag *booleanOutput;

    booleanOutput = G_define_flag();
    booleanOutput->key = 'b';
    booleanOutput->description =
	_("Output format is invisible = 0, visible = 1");
    booleanOutput->guisection = _("Output format");

    /* output mode = elevation flag */
    struct Flag *elevationFlag;

    elevationFlag = G_define_flag();
    elevationFlag->key = 'e';
    elevationFlag->description =
	_("Output format is invisible = NULL, else current elev - viewpoint_elev");
    elevationFlag->guisection = _("Output format");

    /* viewpoint coordinates */
    struct Option *viewLocOpt;

    viewLocOpt = G_define_standard_option(G_OPT_M_COORDS);
    viewLocOpt->required = YES;
    viewLocOpt->description = _("Coordinates of viewing position");

    /* observer elevation */
    struct Option *obsElevOpt;

    obsElevOpt = G_define_option();
    obsElevOpt->key = "obs_elev";
    obsElevOpt->type = TYPE_DOUBLE;
    obsElevOpt->required = NO;
    obsElevOpt->key_desc = "value";
    obsElevOpt->description = _("Viewing elevation above the ground");
    obsElevOpt->answer = "1.75";
    obsElevOpt->guisection = _("Settings");

    /* target elevation offset */
    struct Option *tgtElevOpt;

    tgtElevOpt = G_define_option();
    tgtElevOpt->key = "tgt_elev";
    tgtElevOpt->type = TYPE_DOUBLE;
    tgtElevOpt->required = NO;
    tgtElevOpt->key_desc = "value";
    tgtElevOpt->description = _("Offset for target elevation above the ground");
    tgtElevOpt->answer = "0.0";
    tgtElevOpt->guisection = _("Settings");

    /* max distance */
    struct Option *maxDistOpt;

    maxDistOpt = G_define_option();
    maxDistOpt->key = "max_dist";
    maxDistOpt->type = TYPE_DOUBLE;
    maxDistOpt->required = NO;
    maxDistOpt->key_desc = "value";
    maxDistOpt->description =
	_("Maximum visibility radius. By default infinity (-1)");
    char infdist[10];

    sprintf(infdist, "%d", INFINITY_DISTANCE);
    maxDistOpt->answer = infdist;
    maxDistOpt->guisection = _("Settings");

    /* atmospheric refraction coeff. 1/7 for visual, 0.325 for radio waves, ... */
    /* in future we might calculate this based on the physics, for now we
       just fudge by the 1/7th approximation.

        ?? See ??

        @article{yoeli1985making,
          title={The making of intervisibility maps with computer and plotter},
          author={Yoeli, Pinhas},
          journal={Cartographica: The International Journal for Geographic Information and Geovisualization},
          volume={22},
          number={3},
          pages={88--103},
          year={1985},
          publisher={UT Press}
        }
    */
    struct Option *refrCoeffOpt;

    refrCoeffOpt = G_define_option();
    refrCoeffOpt->key = "refraction_coeff";
    refrCoeffOpt->description = _("Refraction coefficient");
    refrCoeffOpt->type = TYPE_DOUBLE;
    refrCoeffOpt->required = NO;
    refrCoeffOpt->answer = "0.14286";
    refrCoeffOpt->options = "0.0-1.0";
    refrCoeffOpt->guisection = _("Refraction");
    
    /* memory size */
    struct Option *memAmountOpt;

    memAmountOpt = G_define_option();
    memAmountOpt->key = "memory";
    memAmountOpt->type = TYPE_INTEGER;
    memAmountOpt->required = NO;
    memAmountOpt->key_desc = "value";
    memAmountOpt->description =
	_("Amount of memory to be used in MB");
    memAmountOpt->answer = "500";

    /* temporary STREAM path */
    struct Option *streamdirOpt;

    streamdirOpt = G_define_option() ;
    streamdirOpt->key        = "stream_dir";
    streamdirOpt->type       = TYPE_STRING;
    streamdirOpt->required   = NO;
    //streamdirOpt->answer     = "";
    streamdirOpt->description=
       _("Directory to hold temporary files (they can be large)");

    /*fill the options and flags with G_parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* store the parameters into a structure to be used along the way */
    strcpy(viewOptions->inputfname, inputOpt->answer);
    strcpy(viewOptions->outputfname, outputOpt->answer);
    
    if (!streamdirOpt->answer) {
	const char *tmpdir = G_tempfile();
	
	if (G_mkdir(tmpdir) == -1)
	    G_fatal_error(_("Unable to create temp dir"));
	strcpy(viewOptions->streamdir, tmpdir);
    }
    else
	strcpy(viewOptions->streamdir,streamdirOpt->answer);
	
    viewOptions->obsElev = atof(obsElevOpt->answer);
    if(tgtElevOpt->answer)
	viewOptions->tgtElev = atof(tgtElevOpt->answer);

    viewOptions->maxDist = atof(maxDistOpt->answer);
    if (viewOptions->maxDist < 0 && viewOptions->maxDist != INFINITY_DISTANCE) {
	G_fatal_error(_("A negative max distance value is not allowed"));
    }

    viewOptions->doCurv = curvature->answer;
    viewOptions->doRefr = refractionFlag->answer;
    if (refractionFlag->answer && !curvature->answer)
	G_fatal_error(_("Atmospheric refraction is only calculated with "
			"respect to the curvature of the Earth. "
			"Enable the -c flag as well."));
    viewOptions->refr_coef = atof(refrCoeffOpt->answer);

    if (booleanOutput->answer)
	viewOptions->outputMode = OUTPUT_BOOL;
    else if (elevationFlag->answer)
	viewOptions->outputMode = OUTPUT_ELEV;
    else
	viewOptions->outputMode = OUTPUT_ANGLE;

    int memSizeMB = atoi(memAmountOpt->answer);

    if (memSizeMB < 0) {
	G_warning(_("Amount of memory cannot be negative."));
	G_warning(_(" Converting %d to %d MB"), memSizeMB, -memSizeMB);
	memSizeMB = -memSizeMB;
    }
    *memSizeBytes = (long long)memSizeMB;
    *memSizeBytes = (*memSizeBytes) << 20;

    G_get_set_window(window);

    /*The algorithm runs with the viewpoint row and col, so we need to
        convert the lat-lon coordinates to row and column format */
    *vpRow = (int)Rast_northing_to_row(atof(viewLocOpt->answers[1]), window);
    *vpCol = (int)Rast_easting_to_col(atof(viewLocOpt->answers[0]), window);
    G_debug(3, "viewpoint converted from current projection: (%.3f, %.3f)  to col, row (%d, %d)",
        atof(viewLocOpt->answers[0]), atof(viewLocOpt->answers[1]), *vpCol, *vpRow);

    return;
}




/* ------------------------------------------------------------ */
/*print the timings for the internal memory method of computing the
   viewshed */
void
print_timings_internal(Rtimer sweepTime, Rtimer outputTime, Rtimer totalTime)
{

    char timeused[100];

    G_verbose_message("TOTAL TIMING:");

    rt_sprint_safe(timeused, sweepTime);
    G_verbose_message("Sweep: %s", timeused);
    G_verbose_message("\n");

    rt_sprint_safe(timeused, outputTime);
    G_verbose_message("Output: %s", timeused);
    G_verbose_message("\n");

    rt_sprint_safe(timeused, totalTime);
    G_verbose_message("Total: %s", timeused);
    G_verbose_message("\n");
}


/* ------------------------------------------------------------ */
/*print the timings for the external memory method of solving the viewshed */
void
print_timings_external_memory(Rtimer totalTime, Rtimer viewshedTime,
			      Rtimer outputTime, Rtimer sortOutputTime)
{

    /*print timings */
    char timeused[100];

    G_verbose_message("\n\nTOTAL TIMING:");

    rt_sprint_safe(timeused, viewshedTime);
    G_verbose_message("Total sweep: %s", timeused);

    rt_sprint_safe(timeused, sortOutputTime);
    G_verbose_message("Sort output: %s", timeused);

    rt_sprint_safe(timeused, outputTime);
    G_verbose_message("Write result grid: %s", timeused);

    rt_sprint_safe(timeused, totalTime);
    G_verbose_message("Total Time: %s", timeused);
    G_verbose_message("\n\n");
    return;
}
