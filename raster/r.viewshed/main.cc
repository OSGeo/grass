
/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *
 * Date:         july 2008 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The height of a cell is assumed to be constant, and the
 * terrain is viewed as a tesselation of flat cells.  This model is
 * suitable for high resolution rasters; it may not be accurate for
 * low resolution rasters, where it may be better to interpolate the
 * height at a point based on the neighbors, rather than assuming
 * cells are "flat".  The viewshed algorithm is efficient both in
 * terms of CPU operations and I/O operations. It has worst-case
 * complexity O(n lg n) in the RAM model and O(sort(n)) in the
 * I/O-model.  For the algorithm and all the other details see the
 * paper: "Computing Visibility on * Terrains in External Memory" by
 * Herman Haverkort, Laura Toma and Yi Zhuang.
 *
 * COPYRIGHT: (C) 2008 by the GRASS Development Team
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

#ifdef __GRASS__
extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}
#include "grass.h"
#include <grass/iostream/ami.h>

#else
#include <ami.h>
#endif

#include "viewshed.h"
#include "visibility.h"
#include "grid.h"
#include "rbbst.h"
#include "statusstructure.h"
#include "distribute.h"
#include "print_message.h"




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
void print_timings_internal(Rtimer sweepTime, Rtimer outputTime, Rtimer totalTime); 
void print_timings_external_memory(Rtimer totalTime, Rtimer viewshedTime,
								   Rtimer outputTime, Rtimer sortOutputTime); 
void print_status(Viewpoint vp,ViewOptions viewOptions, long long memSizeBytes);void print_usage(); 
#ifdef __GRASS__
void parse_args(int argc, char *argv[], int *vpRow, int *vpCol,  
				ViewOptions* viewOptions,  long long *memSizeBytes, 
				Cell_head * window); 
#else
void parse_args(int argc, char *argv[], int *vpRow, int *vpCol, 
				ViewOptions* viewOptions,  long long *memSizeBytes); 
#endif
  






/* ------------------------------------------------------------ */
int main(int argc, char *argv[]) {

#ifdef __GRASS__
  /* GRASS initialization stuff */
    struct GModule *module;

    /*initialize GIS environment */
    G_gisinit(argv[0]);

    /*initialize module */
    module = G_define_module();
    module->keywords = _("raster, viewshed, line of sight");
    module->description = _("IO-efficient viewshed algorithm");

    struct Cell_head region;
    if (G_get_set_window(&region) == -1) {
	  G_fatal_error("error getting current region");
	  exit(EXIT_FAILURE);
    }
#endif



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
	viewOptions.obsElev =  DEFAULT_OBS_ELEVATION;  
	viewOptions.maxDist =  INFINITY_DISTANCE;
	viewOptions.outputMode =  OUTPUT_ANGLE; 
	viewOptions.doCurv = 0; 

#ifdef __GRASS__
    parse_args(argc,argv, &vpRow, &vpCol,&viewOptions, &memSizeBytes,&region);
#else
    parse_args(argc,argv, &vpRow, &vpCol, &viewOptions, &memSizeBytes);
#endif

   
    /* set viewpoint with the coordinates specified by user. The
	   height of the viewpoint is not known at this point---it will be
	   set during the execution of the algorithm */
    Viewpoint vp;
    set_viewpoint_coord(&vp, vpRow, vpCol);

	print_status(vp, viewOptions,memSizeBytes);
	
	
    /* ************************************************************ */
    /* set up the header of the raster with all raster info and make
	   sure the requested viewpoint is on the map */
	GridHeader *hd;
#ifdef __GRASS__
    hd = read_header_from_GRASS(viewOptions.inputfname, &region);
    assert(hd);

	/* LT: there is no need to exit if viewpoint is outside grid,
	   the algorithm will work correctly in theory. But this
	   requires some changes. To do.*/
    if (!(vp.row < hd->nrows && vp.col < hd->ncols)) {
	  G_fatal_error(_("Viewpoint outside grid"));
	  G_fatal_error(_("viewpont: (row=%d, col=%d)"), vp.row, vp.col);
	  G_fatal_error(_("grid: (rows=%d, cols=%d)"), hd->nrows, hd->ncols);
	  exit(EXIT_FAILURE);
    }
#else
	/*open file input file and read grid header from grid ascii file */
    hd = read_header_from_arcascii_file(viewOptions.inputfname);
    assert(hd);
    printf("input grid: (rows=%d, cols=%d)\n", hd->nrows, hd->ncols);
    fflush(stdout);
    /*sanity check */
    if (!(vp.row < hd->nrows && vp.col < hd->ncols)) {
	  printf("Viewpoint outside grid\n");
	  printf("viewpont: (row=%d, col=%d)\n", vp.row, vp.col);
	  printf("grid: (rows=%d, cols=%d)\n", hd->nrows, hd->ncols);
	  exit(1);
	  /* LT: there is no need to exit if viewpoint is outside grid,
		 the algorithm will work correctly in theory. But this
		 requires some changes. To do.*/
	}
#endif /*__GRASS__*/
	

	/* set curvature params */
#ifdef __GRASS__
	viewOptions.cellsize = region.ew_res;
	double e2; 
	G_get_ellipsoid_parameters(&viewOptions.ellps_a, &e2);
	if (viewOptions.ellps_a == 0) { 
	  /*according to r.los, this can be
		problematic, so we'll have a backup, hardcoded radius :-( */
	  G_warning(_
				("Problems obtaining current ellipsoid parameters, usting sphere (6370997.0)"));
	  viewOptions.ellps_a = 6370997.00;
	}
#else
	/* in standalone mode we do not know how to adjust for curvature */
	assert(viewOptions.doCurv == 0); 
	viewOptions.ellps_a  = 0; 
#endif




    /* ************************************************************ */
    /* decide whether the computation of the viewshed will take place
       in-memory or in external memory */
    int IN_MEMORY;
	long long inmemSizeBytes = get_viewshed_memory_usage(hd); 
    printf("In-memory memory usage is %lld B (%d MB), \
max mem allowed=%lld B(%dMB)\n",   
		   inmemSizeBytes,  (int) (inmemSizeBytes >> 20),  
		   memSizeBytes, (int)(memSizeBytes>>20));
    if (inmemSizeBytes < memSizeBytes) {
	  IN_MEMORY = 1;
	  print_message("*************\nIN_MEMORY MODE\n*************\n");
    }
    else {
	  print_message("*************\nEXTERNAL_MEMORY MODE\n**********\n");
	  IN_MEMORY = 0;
    }
    fflush(stdout);
    /* the mode can be forced to in memory or external if the user
       wants to test or debug a specific mode  */
#ifdef FORCE_EXTERNAL
    IN_MEMORY = 0;
    print_message("FORCED EXTERNAL\n");
#endif

#ifdef FORCE_INTERNAL
    IN_MEMORY = 1;
    print_message("FORCED INTERNAL\n");
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
	  visgrid = viewshed_in_memory(viewOptions.inputfname,hd,&vp, viewOptions);
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
		fprintf(stderr, "%s=%s\n", STREAM_TMPDIR, getenv(STREAM_TMPDIR));
		printf("Intermediate stream location: %s\n", getenv(STREAM_TMPDIR)); 
	  }
	  else {
		/*set it */
	    	  sprintf(buf, "%s=%s", STREAM_TMPDIR, "/var/tmp/");
		  fprintf(stderr, "setting %s ", buf);
		  putenv(buf);
		if (getenv(STREAM_TMPDIR) == NULL) {
		  fprintf(stderr, ", not set\n");
		  exit(1);
		}
		else {
		  fprintf(stderr, ", ok.\n");
		}
		printf("Intermediate stream location: %s\n",  "/var/tmp/" ); 
	  }
	  fprintf(stderr, "Intermediate files will not be deleted "
		  "in case of abnormal termination.\n");
	  fprintf(stderr, "To save space delete these files manually!\n");
	  
	  
	  /* initialize IOSTREAM memory manager */
	  MM_manager.set_memory_limit(memSizeBytes);
	  MM_manager.warn_memory_limit();
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
		print_message
		  ("---Active structure small, starting base case---\n");
		
	    Rtimer totalTime, viewshedTime, outputTime, sortOutputTime;
		
	    rt_start(totalTime);
		
	    /*run viewshed's algorithm */
	    IOVisibilityGrid *visgrid;
		
	    rt_start(viewshedTime);
	    visgrid = viewshed_external(viewOptions.inputfname,hd,&vp,viewOptions);
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
	    print_message("---Active structure does not fit in memory,");
#else
	    print_message("FORCED DISTRIBUTION\n");
#endif
		
		Rtimer totalTime, sweepTime, outputTime, sortOutputTime;
		
	    rt_start(totalTime);
		
	    /*get the viewshed solution by distribution */
	    IOVisibilityGrid *visgrid;
		
	    rt_start(sweepTime);
	    visgrid = distribute_and_sweep(viewOptions.inputfname,hd,&vp,viewOptions);
							 
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
	
	
	



    /* ************************************************************ */
    /* FINISH UP, ALL CASES */
    /* ************************************************************ */
    /*close input file and free grid header */
#ifdef __GRASS__
    G_free(hd);
	/*following GRASS's coding standards for history and exiting */
    struct History history;
	G_short_history(viewOptions.outputfname, "raster", &history);
    G_command_history(&history);
    G_write_history(viewOptions.outputfname, &history);
	exit(EXIT_SUCCESS);
#else
	free(hd);
#endif
}







/* ------------------------------------------------------------ */
/*There are two versions of the function parse_args, one for when it is in GRASS, one for independent compilition. */
#ifdef __GRASS__
void
parse_args(int argc, char *argv[], int *vpRow, int *vpCol,
		   ViewOptions* viewOptions, long long *memSizeBytes,
		   Cell_head *window) {
  
  assert(vpRow && vpCol && memSizeBytes && window); 

  /* the input */
  struct Option *inputOpt;
  inputOpt = G_define_standard_option(G_OPT_R_ELEV);
  inputOpt->key = "input";

  /* the output */
  struct Option *outputOpt;
  outputOpt = G_define_standard_option(G_OPT_R_OUTPUT);
  outputOpt->description = "Name of output viewshed raser map\n\t\t\tdefault format: {NODATA, -1 (invisible), vertical angle wrt viewpoint (visible)}"; 

  
  /* row-column flag */
  struct Flag *row_col;
  row_col = G_define_flag();
  row_col->key = 'r';
  row_col->description =
	_("Use row-column location rather than latitude-longitude location");
  
  /* curvature flag */
  struct Flag *curvature;
  curvature = G_define_flag();
  curvature->key = 'c';
  curvature->description =
	_("Consider the curvature of the earth (current ellipsoid)");
  
  
  /* boolean output flag */
  struct Flag *booleanOutput;
  booleanOutput = G_define_flag();
  booleanOutput->key = 'b';
  booleanOutput->description =
	_("Output format is {0 (invisible) 1 (visible)}");
  
  /* output mode = elevation flag */ 
  struct Flag *elevationFlag;
  elevationFlag = G_define_flag();
  elevationFlag->key = 'e';
  elevationFlag->description =
	("Output format is {NODATA, -1 (invisible), elev-viewpoint_elev (visible)}");

  /* viewpoint coordinates */
  struct Option *viewLocOpt;
  viewLocOpt = G_define_option();
  viewLocOpt->key = "viewpoint_location";
  viewLocOpt->type = TYPE_STRING;
  viewLocOpt->required = YES;
  viewLocOpt->key_desc = "lat,long";
  viewLocOpt->description =
	("Coordinates of viewing position in latitude-longitude (if -r flag is present, then coordinates are row-column)");
  
  /* observer elevation */
  struct Option *obsElevOpt;
  obsElevOpt = G_define_option();
  obsElevOpt->key = "observer_elevation";
  obsElevOpt->type = TYPE_DOUBLE;
  obsElevOpt->required = NO;
  obsElevOpt->key_desc = "value";
  obsElevOpt->description = _("Viewing elevation above the ground");
  obsElevOpt->answer = "0.0";
  
  /* max distance */
  struct Option *maxDistOpt;
  maxDistOpt = G_define_option();
  maxDistOpt->key = "max_dist";
  maxDistOpt->type = TYPE_DOUBLE;
  maxDistOpt->required = NO;
  maxDistOpt->key_desc = "value";
  maxDistOpt->description =
	("Maximum visibility radius. By default infinity (-1).");
  char infdist[10]; 
  sprintf(infdist, "%d", INFINITY_DISTANCE);
  maxDistOpt->answer = infdist; 


	/* memory size */
    struct Option *memAmountOpt;
    memAmountOpt = G_define_option();
    memAmountOpt->key = "memory_usage";
    memAmountOpt->type = TYPE_INTEGER;
    memAmountOpt->required = NO;
    memAmountOpt->key_desc = "value";
    memAmountOpt->description = _("The amount of main memory in MB to be used");
    memAmountOpt->answer = "500";
	

    /*fill the options and flags with G_parser */
    if (G_parser(argc, argv))
	  exit(EXIT_FAILURE);


    /* store the parameters into a structure to be used along the way */
    strcpy(viewOptions->inputfname, inputOpt->answer);
    strcpy(viewOptions->outputfname, outputOpt->answer);

    viewOptions->obsElev = atof(obsElevOpt->answer);

    viewOptions->maxDist = atof(maxDistOpt->answer);
    if (viewOptions->maxDist < 0 && 
	viewOptions->maxDist!= INFINITY_DISTANCE) {
      G_fatal_error(_("negative max distance value is not valid"));
      exit(EXIT_FAILURE);
    }

    

	viewOptions->doCurv = curvature->answer;
	if (booleanOutput->answer) 
	  viewOptions->outputMode = OUTPUT_BOOL; 
	else if (elevationFlag->answer)
	  viewOptions->outputMode = OUTPUT_ELEV; 
	else  viewOptions->outputMode = OUTPUT_ANGLE; 

    int memSizeMB = atoi(memAmountOpt->answer);
	if (memSizeMB <0) {
	  printf("Memory cannot be negative.\n");
	  exit(1);
	}
    *memSizeBytes = (long long)memSizeMB;
    *memSizeBytes = (*memSizeBytes) << 20;

    /*The algorithm runs with the viewpoint row and col, so depending
	  on if the row_col flag is present we either need to store the
	  row and col, or convert the lat-lon coordinates to row and
	  column format */
    if (row_col->answer) {
	  *vpRow = atoi(viewLocOpt->answers[1]);
	  *vpCol = atoi(viewLocOpt->answers[0]);
	  printf("viewpoint in row-col mode: (%d,%d)\n", *vpRow, *vpCol);
    }
    else {
	  *vpRow =
	    (int)G_northing_to_row(atof(viewLocOpt->answers[1]), window);
	  *vpCol = (int)G_easting_to_col(atof(viewLocOpt->answers[0]), window);
	  printf("viewpoint converted from lat-lon mode: (%d,%d)\n",*vpRow,*vpCol);
	  
	}

	 
    return;
}




/* ------------------------------------------------------------ */
#else /* if not in GRASS mode */
void
parse_args(int argc, char *argv[], int *vpRow, int *vpCol, 
		   ViewOptions* viewOptions,  long long *memSizeBytes)  {
  
  assert(vpRow && vpCol && viewOptions && memSizeBytes); 

  int gotrow = 0, gotcol=0, gotinput = 0, gotoutput = 0; 
  int c;
  
  /*deal with flags for the output using getopt */
  while ((c = getopt(argc, argv, "i:o:r:c:v:e:d:m:")) != -1) {
	switch (c) {
	case 'i':
	  /* inputfile name */
	  //*inputfname = optarg;
	  strcpy(viewOptions->inputfname, optarg);
	  gotinput = 1; 
	  break;
	case 'o':
	  //*outputfname = optarg;
	  strcpy(viewOptions->outputfname, optarg);
	  gotoutput= 1; 
	  break;
	case 'r':
	    *vpRow = atoi(optarg);
		gotrow=1; 
	    break;
	case 'c':
	    *vpCol = atoi(optarg);
		gotcol = 1; 
	    break;
	case 'v':
	  /* output mode */
	  if (strcmp(optarg, "angle")==0)  
		 viewOptions->outputMode= OUTPUT_ANGLE; 
	  else if (strcmp(optarg, "bool")==0)  
		viewOptions->outputMode= OUTPUT_BOOL; 
	  else if (strcmp(optarg, "elev")==0)  
		viewOptions->outputMode= OUTPUT_ELEV; 
	  else {
		printf("unknown option %s: use  -v: [angle|bool|elev]\n", optarg); 
		exit(1); 
	  }
	  break;
	case 'e':
	  viewOptions->obsElev = atof(optarg);
	  break;
	case 'd':
	  viewOptions->maxDist = atof(optarg);
	  if (viewOptions->maxDist < 0) {
		printf("max distance needs to be positive\n");
		exit(EXIT_FAILURE); 
	  }
	  break;
	case 'm':
	  int memSizeMB; 
	  memSizeMB = atoi(optarg);
	  if (memSizeMB <0) {
		printf("Memory cannot be negative.\n");
		exit(1);
	  }
	  *memSizeBytes = (long long)memSizeMB;
	  *memSizeBytes = (*memSizeBytes) << 20;
	  break;
	case '?':
	  if (optopt == 'i' || optopt == 'o' || optopt == 'r' ||
		  optopt == 'c' || optopt == 'e' || optopt == 'd' ||
		  optopt == 'm')
		fprintf(stderr, "Option -%c requires an argument.\n", optopt);
	  else if (isprint(optopt)) 
		fprintf(stderr, "Unknown option '-%c'.\n", optopt);
	  else
		fprintf(stderr, "Unknown option character '\\x%x.\n", optopt);
	  print_usage();
	  exit(1);
	  //default:
	  //exit(1);
	}
  } /* while getopt */
  
    /*check to make sure the required options are set.*/
  if(!(gotinput && gotoutput && gotrow &&gotcol)) {
	printf("Not all required options set.\n");
	print_usage();
	exit(1);
  }
  printf("viewpoint: (%d, %d)\n", *vpRow, *vpCol);
  return;
}
#endif








/* ------------------------------------------------------------ */
/*print the timings for the internal memory method of computing the
  viewshed */
void
print_timings_internal(Rtimer sweepTime, Rtimer outputTime, Rtimer totalTime) {
  
  char timeused[100]; 
  printf("TOTAL TIMING: \n");
  rt_sprint_safe(timeused, sweepTime);
  printf("\t%30s", "sweep:");
  printf(timeused);   printf("\n");
 
  rt_sprint_safe(timeused, outputTime);
  printf("\t%30s", "output:");
  printf(timeused);   printf("\n");

  rt_sprint_safe(timeused, totalTime);
  printf("\t%30s", "total:");
  printf(timeused); 
  printf("\n");
}


/* ------------------------------------------------------------ */
/*print the timings for the external memory method of solving the viewshed */
void
print_timings_external_memory(Rtimer totalTime, Rtimer viewshedTime,
			      Rtimer outputTime, Rtimer sortOutputTime)
{

    /*print timings */
    char timeused[100];

    printf("\n\nTOTAL TIMING: \n");
    rt_sprint_safe(timeused, viewshedTime);
    printf("\t%30s", "total sweep:");
    printf(timeused);
    printf("\n");
    rt_sprint_safe(timeused, sortOutputTime);
    printf("\t%30s", "sort output:");
    printf(timeused);
    printf("\n");
    rt_sprint_safe(timeused, outputTime);
    printf("\t%30s", "Write result grid:");
    printf(timeused);
    printf("\n");
    rt_sprint_safe(timeused, totalTime);
    printf("\t%30s", "Total Time:");
    printf(timeused);
    printf("\n\n");
    return;
}

/* ------------------------------------------------------------ */
void print_status(Viewpoint vp,ViewOptions viewOptions, long long memSizeBytes)
{


#ifdef __GRASS__
  G_message(_("Options set as:\n"));
  G_message(_("---input: %s \n---output: %s \n---viewpoint: (%d, %d)"),
  		viewOptions.inputfname, viewOptions.outputfname, 
  		vp.row, vp.col);
  if (viewOptions.outputMode == OUTPUT_ANGLE) {
  G_message(_("---outputting viewshed in angle mode:")); 
  G_message(_("---The output is {NODATA, %d(invisible),angle(visible)}.\n"),  INVISIBLE);
  }
  if (viewOptions.outputMode == OUTPUT_BOOL) {
  G_message(_("---outputting viewshed in boolean mode: "));
  G_message(_("---The output is {%d (invisible), %d (visible)}.\n"), 
  	   BOOL_INVISIBLE, BOOL_VISIBLE);
  }
  if (viewOptions.outputMode == OUTPUT_ELEV) {
  G_message(_("---outputting viewshed in elevation mode: "));    
  G_message(_("---The output is {NODATA, %d (invisible), elev (visible)}.\n"), 
  		  INVISIBLE);
  }
  G_message(_("---observer elevation above terrain: %f\n"), viewOptions.obsElev);
  
  if (viewOptions.maxDist == INFINITY_DISTANCE)
  G_message(_("---max distance: infinity\n")); 
  else G_message(_("---max distance: %f\n"), viewOptions.maxDist); 
  
  G_message(_("---consider earth curvature: %d\n"), viewOptions.doCurv); 
  
  G_message(_("---max memory = %d MB\n"), (int)(memSizeBytes >> 20));
  G_message(_("---------------------------------\n"));

#else
  printf("---------------------------------\nOptions set as:\n"); 
  printf("input: %s \noutput: %s\n",
		 viewOptions.inputfname, viewOptions.outputfname);
  printf("viewpoint: (%d, %d)\n", vp.row, vp.col);
  if (viewOptions.outputMode == OUTPUT_ANGLE) {
	printf("outputting viewshed in angle mode: ");
	printf("The output is {NODATA, %d (invisible), angle (visible)}.\n", 
		   INVISIBLE);
  }
  if (viewOptions.outputMode == OUTPUT_BOOL) {
	printf("outputting viewshed in boolean mode: ");
	printf("The output is {%d (invisible), %d (visible)}.\n", 
		   BOOL_INVISIBLE, BOOL_VISIBLE);
  }
  if (viewOptions.outputMode == OUTPUT_ELEV) {
	printf("outputting viewshed in elevation mode: ");    
	  printf("The output is {NODATA, %d (invisible), elev (visible)}.\n", 
			 INVISIBLE);
  }
  
  printf("observer elevation above terrain: %f\n", viewOptions.obsElev);
  
  if (viewOptions.maxDist == INFINITY_DISTANCE)
	printf("max distance: infinity\n"); 
  else printf("max distance: %f\n", viewOptions.maxDist); 
  
  printf("consider earth curvature: %d\n", viewOptions.doCurv); 
  
  printf("max memory = %d MB\n", (int)(memSizeBytes >> 20));
  printf("---------------------------------\n");
  
#endif

}

/* ------------------------------------------------------------ */
/*print the usage information.  Only used in the stand-alone version*/
void print_usage() {
	printf("\nusage: ioviewshed -i <input name> -o <output name> -r <row number> -c <column number> [-v <angle | bool | elev>] [-e <observer elevation>] [-d <max distance>] [-m <memory usage MB>]\n\n");

	printf("OPTIONS\n");
	printf("-i \t input map name.\n");
	printf("-o \t output map name.\n");
	printf("-r \t row number.\n");
	printf("-c \t column number.\n");
	printf("-v \t iutput mode. Default is angle.\n");
	printf("   \t\t angle: output is {NODATA, -1 (invisible), angle (visible)}\n\t\t\t angle is a value in [0,180] and represents the vertical angle wrt viewpoint.\n"); 
	printf("   \t\t bool:  output is {0 (invisible), 1 (visible)}.\n"); 
	printf("   \t\t elev:  output is {NODATA, -1 (invisible), elev (visible)}. This is not implemented in the standalone version.\n"); 
	printf("-e \t observer elevation. Default is 0.\n");
	printf("-d \t maximum distance. Default is infinity.\n");
	printf("-m \t memory usage in MB. Default is 500.\n");
}
