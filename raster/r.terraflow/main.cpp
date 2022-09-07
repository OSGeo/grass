/****************************************************************************
 * 
 *  MODULE:	r.terraflow
 *
 *  COPYRIGHT (C) 2007, 2010 Laura Toma
 *   
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>

#ifdef HAVE_STATVFS_H
#include <sys/statvfs.h>
#endif


extern "C" {
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
}

#include "option.h"
#include "common.h" /* declares the globals */
#include "fill.h"
#include "flow.h"
#include "nodata.h"
#include "grass2str.h"
#include "water.h"
#include "sortutils.h"


/* globals: in common.H
extern statsRecorder *stats; 
extern userOptions* opt;    
extern struct Cell_head region; 
*/


/* #define JUMP2FLOW */
/* define it only if you want to skip the flow direction computation
   and jump directly to computing flow accumulation; the flowstream
   must exist in /STREAM_DIR/flowStream */


/* ---------------------------------------------------------------------- */
void 
parse_args(int argc, char *argv[]) {

  /* input elevation grid  */
  struct Option *input_elev;
  input_elev = G_define_standard_option(G_OPT_R_ELEV);

  /* output filled elevation grid */
  struct Option *output_elev;
  output_elev = G_define_standard_option(G_OPT_R_OUTPUT);
  output_elev->key        = "filled";
  output_elev->description= _("Name for output filled (flooded) elevation raster map");
  output_elev->required = NO;
  output_elev->guisection = _("Outputs");
  
 /* output direction  grid */
  struct Option *output_dir;
  output_dir = G_define_standard_option(G_OPT_R_OUTPUT);
  output_dir->key        = "direction";
  output_dir->description= _("Name for output flow direction raster map");
  output_dir->required = NO;
  output_dir->guisection = _("Outputs");
    
  /* output sinkwatershed  grid */
  struct Option *output_watershed;
  output_watershed = G_define_standard_option(G_OPT_R_OUTPUT);
  output_watershed->key        = "swatershed";
  output_watershed->description= _("Name for output sink-watershed raster map");
  output_watershed->required = NO;
  output_watershed->guisection = _("Outputs");
   
  /* output flow accumulation grid */
  struct Option *output_accu;
  output_accu = G_define_standard_option(G_OPT_R_OUTPUT);
  output_accu->key        = "accumulation";
  output_accu->description= _("Name for output flow accumulation raster map");
  output_accu->required = NO;
  output_accu->guisection = _("Outputs");
   
#ifdef OUTPUT_TCI
  struct Option *output_tci;
  output_tci = G_define_standard_option(G_OPT_R_OUTPUT);
  output_tci->key        = "tci";
  output_tci->description=
    _("Name for output topographic convergence index (tci) raster map");
  output_tci->required = NO;
  output_tci->guisection = _("Outputs");
#endif

  /* MFD/SFD flag */
  struct Flag *sfd_flag;
  sfd_flag = G_define_flag() ;
  sfd_flag->key        = 's';
  sfd_flag->label= _("SFD (D8) flow (default is MFD)");
  sfd_flag->description =
	_("SFD: single flow direction, MFD: multiple flow direction");

  /* D8CUT value*/
  struct Option *d8cut;
  d8cut = G_define_option();
  d8cut->key  = "d8cut";
  d8cut->type = TYPE_DOUBLE;
  d8cut->required = NO;
  d8cut->label = _("Routing using SFD (D8) direction");
  d8cut->description = 
    _("If flow accumulation is larger than this value it is routed using "
      "SFD (D8) direction (meaningful only for MFD flow). "
      "If no answer is given it defaults to infinity.");

  /* raster cache memory */
  struct Option *mem;
  mem = G_define_standard_option(G_OPT_MEMORYMB);

  /* temporary STREAM path */
  struct Option *streamdir;
  streamdir = G_define_option() ;
  streamdir->key        = "directory";
  streamdir->type       = TYPE_STRING;
  streamdir->required   = NO;
  /* streamdir->answer     = ""; */
  streamdir->description=
     _("Directory to hold temporary files (they can be large)");

 /* stats file */
  struct Option *stats_opt;
  stats_opt = G_define_option() ;
  stats_opt->key        = "stats";
  stats_opt->type       = TYPE_STRING;
  stats_opt->required   = NO;
  stats_opt->description= _("Name for output file containing runtime statistics");
  stats_opt->guisection = _("Outputs");
  
  G_option_requires(input_elev, output_elev, output_dir, output_watershed,
                    output_accu, output_tci, NULL);
  
  if (G_parser(argc, argv)) {
    exit (EXIT_FAILURE);
  }
  
  /* ************************* */
  assert(opt);
  opt->elev_grid = input_elev->answer;
  opt->filled_grid = output_elev->answer;
  opt->dir_grid = output_dir->answer; 
  opt->watershed_grid = output_watershed->answer;
  opt->flowaccu_grid = output_accu->answer;
#ifdef OUTPUT_TCI
  opt->tci_grid = output_tci->answer;
#endif

  opt->d8 = sfd_flag->answer;
  if (!d8cut->answer) {
    opt->d8cut = MAX_ACCU;
  } else {
    opt->d8cut = atof(d8cut->answer);
  }

  opt->mem = atoi(mem->answer);
  if (!streamdir->answer) {
    const char *tmpdir = G_tempfile();
    
    if (G_mkdir(tmpdir) == -1)
	G_fatal_error(_("Unable to create temp dir"));
    opt->streamdir = G_store(tmpdir);
  }
  else
    opt->streamdir = streamdir->answer;

  opt->stats = stats_opt->answer;

  opt->verbose = G_verbose() == G_verbose_max();

  /* somebody should delete the options */
}


/* ---------------------------------------------------------------------- */
/* check compatibility of map header and region header */
void check_header(char* cellname) {

  const char *mapset;
  mapset = G_find_raster(cellname, "");
  if (mapset == NULL) {
    G_fatal_error(_("Raster map <%s> not found"), cellname);
  }
  /* read cell header */
  struct Cell_head cell_hd;
  Rast_get_cellhd (cellname, mapset, &cell_hd);
  
  /* check compatibility with module region */
  if (!((region->ew_res == cell_hd.ew_res)
		&& (region->ns_res == cell_hd.ns_res))) {
    G_fatal_error(_("Raster map <%s> resolution differs from current region"),
                  cellname);
  } else {
      G_verbose_message(_("Header of raster map <%s> compatible with region header"),
                        cellname);
  }


  /* check type of input elevation raster and check if precision is lost */
    RASTER_MAP_TYPE data_type;
    data_type = Rast_map_type(opt->elev_grid, mapset);
#ifdef ELEV_SHORT
	G_verbose_message(_("Elevation stored as SHORT (%dB)"),
		sizeof(elevation_type));
	if (data_type == FCELL_TYPE) {
	  G_warning(_("Raster map <%s> is of type FCELL_TYPE "
                      "-- precision may be lost"), opt->elev_grid); 
	}
	if (data_type == DCELL_TYPE) {
	  G_warning(_("Raster map <%s> is of type DCELL_TYPE "
                      "-- precision may be lost"),  opt->elev_grid);
	}
#endif 
#ifdef ELEV_FLOAT
	G_verbose_message(_("Elevation stored as FLOAT (%ldB)"), 
                          sizeof(elevation_type));
	if (data_type == CELL_TYPE) {
	  G_warning(_("Raster map <%s> is of type CELL_TYPE "
                      "-- you should use r.terraflow.short"), opt->elev_grid); 
	}
	if (data_type == DCELL_TYPE) {
	  G_warning(_("Raster map <%s> is of type DCELL_TYPE "
                      "-- precision may be lost"),  opt->elev_grid);
	}
#endif
	



}

/* ---------------------------------------------------------------------- */
void check_args() {

  /* check if filled elevation grid name is  valid */
  if (opt->filled_grid && G_legal_filename (opt->filled_grid) < 0) {
    G_fatal_error(_("<%s> is an illegal file name"), opt->filled_grid);
  }
  /* check if output grid names are valid */
  if (opt->dir_grid && G_legal_filename (opt->dir_grid) < 0) {
    G_fatal_error(_("<%s> is an illegal file name"), opt->dir_grid);
  }
  if (opt->flowaccu_grid && G_legal_filename (opt->flowaccu_grid) < 0) {
    G_fatal_error(_("<%s> is an illegal file name"), opt->flowaccu_grid);
  }
  if (opt->watershed_grid && G_legal_filename (opt->watershed_grid) < 0) {
    G_fatal_error(_("<%s> is an illegal file name"), opt->watershed_grid);
  }
#ifdef OUTPUT_TCI
  if (opt->tci_grid && G_legal_filename (opt->tci_grid) < 0) {
  G_fatal_error(_("<%s> is an illegal file name"), opt->tci_grid);
  }
#endif
  
  /* check compatibility with region */
  check_header(opt->elev_grid);

  /* what else ? */  


}



/* ---------------------------------------------------------------------- */
void record_args(int argc, char **argv) {

  time_t t = time(NULL);
  char buf[BUFSIZ];
  if(t == (time_t)-1) {
    perror("time");
    exit(1);
  }

#ifdef __MINGW32__
  strcpy(buf, ctime(&t));
#else
  ctime_r(&t, buf);
  buf[24] = '\0';
#endif
  stats->timestamp(buf);
  
  *stats << "Command Line: " << endl;
  for(int i=0; i<argc; i++) {
    *stats << argv[i] << " ";
  }
  *stats << endl;
  
  *stats << "input elevation grid: " << opt->elev_grid << "\n";
  if (opt->filled_grid)
      *stats << "output (flooded) elevations grid: " << opt->filled_grid << "\n";
  if (opt->dir_grid)
      *stats << "output directions grid: " << opt->dir_grid << "\n";
  if (opt->watershed_grid)
      *stats << "output sinkwatershed grid: " << opt->watershed_grid << "\n";
  if (opt->flowaccu_grid)
      *stats << "output accumulation grid: " << opt->flowaccu_grid << "\n";
#ifdef OUTPUT_TCI
  if (opt->tci_grid)
      *stats <<  "output tci grid: " << opt->tci_grid << "\n";
#endif
  if (opt->d8) {
    stats ->comment("SFD (D8) flow direction");
  } else {
    stats->comment("MFD flow direction");
  }

  sprintf(buf, "D8CUT=%f", opt->d8cut);
  stats->comment(buf);

  size_t mm_size = (size_t) opt->mem  << 20; /* (in bytes) */
  char tmp[100];
  formatNumber(tmp, mm_size);
  sprintf(buf, "Memory size: %s bytes", tmp);
  stats->comment(buf);
}



/* ---------------------------------------------------------------------- */
void 
setFlowAccuColorTable(char* cellname) {
  struct Colors colors;
  const char *mapset;
  struct Range r;

  mapset = G_find_raster(cellname, "");
  if (mapset == NULL) {
    G_fatal_error (_("Raster map <%s> not found"), cellname);
  }
  if (Rast_read_range(cellname, mapset, &r) == -1) {
    G_fatal_error(_("cannot read range"));
  }
  /*G_debug(1, "%s range is: min=%d, max=%d\n", cellname, r.min, r.max);*/
  int v[6];
  v[0] = r.min;
  v[1] = 5;
  v[2] = 30;
  v[3] = 100;
  v[4] = 1000;
  v[5] = r.max;
  

  Rast_init_colors(&colors);
 
  Rast_add_c_color_rule(&v[0], 255,255,255,  &v[1],     255,255,0, &colors);
  Rast_add_c_color_rule(&v[1], 255,255,0,    &v[2],       0,255,255, &colors);
  Rast_add_c_color_rule(&v[2],   0,255,255,  &v[3],       0,127,255, &colors);
  Rast_add_c_color_rule(&v[3],   0,127,255,  &v[4],       0,0,255,   &colors);
  Rast_add_c_color_rule(&v[4],   0,0,255,    &v[5],   0,0,0,     &colors);

  Rast_write_colors(cellname, mapset, &colors);

  Rast_free_colors(&colors);
}


/* ---------------------------------------------------------------------- */
void
setSinkWatershedColorTable(char* cellname) {
  struct  Colors colors;
  const char *mapset;
  struct Range r;

  mapset = G_find_raster(cellname, "");
  if (mapset == NULL) {
    G_fatal_error (_("Raster map <%s> not found"), cellname);
  }
  if (Rast_read_range(cellname, mapset, &r) == -1) {
    G_fatal_error(_("cannot read range"));
  }

  Rast_init_colors(&colors);
  Rast_make_random_colors(&colors, 1, r.max);

  Rast_write_colors(cellname, mapset, &colors);

  Rast_free_colors(&colors);
}



/* print the largest interim file that will be generated during
   r.terraflow */
void
printMaxSortSize(long nodata_count) {
  char buf[BUFSIZ];
  off_t  fillmaxsize = (off_t)nrows*ncols*sizeof(waterWindowType);
  off_t  flowmaxsize = ((off_t)nrows*ncols - nodata_count)*sizeof(sweepItem);
  off_t maxneed = (fillmaxsize > flowmaxsize) ? fillmaxsize: flowmaxsize;
  maxneed =  2*maxneed; /* need 2*N to sort */

  G_debug(1, "total elements=%" PRI_OFF_T ", nodata elements=%ld",
          (off_t)nrows * ncols, nodata_count);
  G_debug(1, "largest temporary files: ");
  G_debug(1, "\t\t FILL: %s [%" PRI_OFF_T " elements, %ldB each]",
          formatNumber(buf, fillmaxsize),
          (off_t)nrows * ncols, sizeof(waterWindowType));
  G_debug(1, "\t\t FLOW: %s [%" PRI_OFF_T " elements, %ldB each]",
          formatNumber(buf, flowmaxsize),
          (off_t)nrows * ncols - nodata_count, sizeof(sweepItem));
  G_debug(1, "Will need at least %s space available in %s",
          formatNumber(buf, maxneed),  	  /* need 2*N to sort */
          getenv(STREAM_TMPDIR));

#ifdef HAVE_STATVFS_H
  G_debug(1, "Checking current space in %s: ", getenv(STREAM_TMPDIR));
  struct statvfs statbuf;
  statvfs(getenv(STREAM_TMPDIR), &statbuf);

  float avail = statbuf.f_bsize*statbuf.f_bavail;
  G_debug(1, "available %ld blocks x %ldB = %.0fB",
          (long)statbuf.f_bavail, statbuf.f_bsize, avail);
  if (avail > maxneed) {
      G_debug(1, ". OK.");
  } else {
      G_fatal_error("Not enough space available");
  }
#endif
}



/* ---------------------------------------------------------------------- */
int
main(int argc, char *argv[]) {
  struct GModule *module;
  Rtimer rtTotal;    
  char buf[BUFSIZ];

  /* initialize GIS library */
  G_gisinit(argv[0]);

 
  module = G_define_module();
  module->description = _("Performs flow computation for massive grids.");
  G_add_keyword(_("raster"));
  G_add_keyword(_("hydrology"));
  G_add_keyword(_("flow"));
  G_add_keyword(_("accumulation"));
  G_add_keyword(_("sink"));
  
  /* read user options; fill in global <opt> */  
  opt = (userOptions*)malloc(sizeof(userOptions));
  assert(opt);
  
  region = (struct Cell_head*)malloc(sizeof(struct Cell_head));
  assert(region);

  parse_args(argc, argv);

  /* get the current region and dimensions */  
  G_get_set_window(region);

  check_args();

  int nr = Rast_window_rows();
  int nc = Rast_window_cols();
  if ((nr > dimension_type_max) || (nc > dimension_type_max)) {
    G_fatal_error(_("[nrows=%d, ncols=%d] dimension_type overflow -- "
	"change dimension_type and recompile"), nr, nc);
  } else {
    nrows = (dimension_type)nr;
    ncols = (dimension_type)nc;
  }

  G_verbose_message( _("Region size is %d x %d"), nrows, ncols);
 
  /* check STREAM path (the place where intermediate STREAMs are placed) */
  sprintf(buf, "%s=%s",STREAM_TMPDIR, opt->streamdir);
  /* don't pass an automatic variable; putenv() isn't guaranteed to make a copy */
  putenv(G_store(buf));
  if (getenv(STREAM_TMPDIR) == NULL) {
      G_fatal_error(_("%s not set"), STREAM_TMPDIR);
  } else {
      G_verbose_message(_("STREAM temporary files in <%s>. "
                          "THESE INTERMEDIATE STREAMS WILL NOT BE DELETED "
                          "IN CASE OF ABNORMAL TERMINATION OF THE PROGRAM. "
                          "TO SAVE SPACE PLEASE DELETE THESE FILES MANUALLY!"),
                        getenv(STREAM_TMPDIR));
  }
  
  if (opt->stats) {
      /* open the stats file */
      stats = new statsRecorder(opt->stats);
      record_args(argc, argv);
      {
	char buf[BUFSIZ];
	off_t grid_size = (off_t) nrows * ncols;
	*stats << "region size = " <<  formatNumber(buf, grid_size) << " elts "
	       << "(" << nrows << " rows x " << ncols << " cols)\n";

	stats->flush();
      }
  }

  /* set up STREAM memory manager */
  size_t mm_size = (size_t) opt->mem << 20; /* opt->mem is in MB */
  MM_manager.set_memory_limit(mm_size);
  if (opt->verbose) {
      MM_manager.warn_memory_limit();
  } else {
      MM_manager.ignore_memory_limit();
  }
  if (opt->verbose)
      MM_manager.print_limit_mode();


  /* initialize nodata */
  nodataType::init();
  if (stats)
    *stats << "internal nodata value: " << nodataType::ELEVATION_NODATA << endl;
   
  /* start timing -- after parse_args, which are interactive */
  rt_start(rtTotal);

#ifndef JUMP2FLOW 
  /* read elevation into a stream */
  AMI_STREAM<elevation_type> *elstr=NULL;
  long nodata_count;
  elstr = cell2stream<elevation_type>(opt->elev_grid, elevation_type_max,
									  &nodata_count);
  /* print the largest interim file that will be generated */
  printMaxSortSize(nodata_count);
  

  /* -------------------------------------------------- */
  /* compute flow direction and filled elevation (and watersheds) */
  AMI_STREAM<direction_type> *dirstr=NULL;
  AMI_STREAM<elevation_type> *filledstr=NULL;
  AMI_STREAM<waterWindowBaseType> *flowStream=NULL;
  AMI_STREAM<labelElevType> *labeledWater = NULL;

  flowStream=computeFlowDirections(elstr, filledstr, dirstr, labeledWater);

  delete elstr;

  /* write streams to GRASS raster maps */
  if (opt->dir_grid)
      stream2_CELL(dirstr, nrows, ncols, opt->dir_grid);
  delete dirstr;
  if (opt->filled_grid) {
#ifdef ELEV_SHORT
      stream2_CELL(filledstr, nrows, ncols, opt->filled_grid);
#else
      stream2_CELL(filledstr, nrows, ncols, opt->filled_grid,true);
#endif
  }
  delete filledstr; 

  if (opt->watershed_grid) {
      stream2_CELL(labeledWater, nrows, ncols, labelElevTypePrintLabel(), 
                   opt->watershed_grid);
      setSinkWatershedColorTable(opt->watershed_grid);
  }
  delete labeledWater;
  
#else 
  AMI_STREAM<waterWindowBaseType> *flowStream;
  char path[GPATH_MAX];

  sprintf(path, "%s/flowStream", streamdir->answer);
  flowStream = new AMI_STREAM<waterWindowBaseType>(path);
  G_verbose_message(_("flowStream opened: len=%lld\n", flowStream->stream_len());
  G_verbose_message(_("jumping to flow accumulation computation\n");
#endif
  
  /* -------------------------------------------------- */
  /* compute flow accumulation (and tci) */
  AMI_STREAM<sweepOutput> *outstr=NULL;
  
  computeFlowAccumulation(flowStream, outstr);
  /* delete flowStream -- deleted inside */

  /* write output stream to GRASS raster maps */
#ifdef OUTPUT_TCI
  if (opt->flowaccu_grid && opt->tci_grid)
      stream2_FCELL(outstr, nrows, ncols, printAccumulation(), printTci(),
                    opt->flowaccu_grid, opt->tci_grid);
  else if (opt->tci_grid)
      stream2_FCELL(outstr, nrows, ncols, printTci(),
                    opt->tci_grid);
  else if (opt->flowaccu_grid)
      stream2_FCELL(outstr, nrows, ncols, printAccumulation(),
                    opt->flowaccu_grid);
#else
  if (opt->flowaccu_grid)
      stream2_FCELL(outstr, nrows, ncols, printAccumulation(),
                    opt->flowaccu_grid);
#endif
  if (opt->flowaccu_grid)
      setFlowAccuColorTable(opt->flowaccu_grid);
  delete outstr;
  
  rt_stop(rtTotal);
  if (stats) {
      stats->recordTime("Total running time: ", rtTotal);
      stats->timestamp("end");
  }

  G_done_msg(" ");
  
  /* free the globals */
  free(region);
  free(opt);
  if (stats)
    delete stats;

  return 0;
}
