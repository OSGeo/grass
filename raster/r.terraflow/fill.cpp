/****************************************************************************
 * 
 *  MODULE:	r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
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

#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <string> 
#include "fill.h"
#include "common.h"
#include "water.h"
#include "sortutils.h"
#include "streamutils.h"
#include "filldepr.h"
#include "grid.h"

/* globals in common.H

extern statsRecorder *stats;       stats file 
extern userOptions *opt;           command-line options 
extern struct  Cell_head *region;  header of the region 
extern dimension_type nrows, ncols;
*/


#define FILL_DEBUG if(0)
#define FILL_SAVEALL if(0)


/* defined in this module */
void recordWatersheds(AMI_STREAM<labelElevType> *labeledWater);
long assignDirections(AMI_STREAM<plateauStats> *statstr, 
					  AMI_STREAM<plateauType> *platstr,
					  AMI_STREAM<waterType> *waterstr);
void assignFinalDirections(AMI_STREAM<plateauStats> *statstr, 
			   AMI_STREAM<plateauType> *platstr,
			   AMI_STREAM<waterType> *waterstr);
template<class T1, class T2, class T3, class T4, class FUN>
void mergeStreamGridGrid(AMI_STREAM<T1> *grid1,
			 AMI_STREAM<T2> *grid2,
			 dimension_type rows, dimension_type cols, 
			 AMI_STREAM<T3> *str,
			 FUN fo,
			 AMI_STREAM<T4> *outStream);
void merge2waterBase(AMI_STREAM<waterType> *unsortedWaterStr, 
		     AMI_STREAM<direction_type> *dirStr,
		     AMI_STREAM<elevation_type> *elStr, 
		     AMI_STREAM<waterWindowBaseType> *merge);
AMI_STREAM<waterGridType>*
merge2waterGrid(AMI_STREAM<waterType> *unsortedWaterStr, 
		AMI_STREAM<direction_type> *dirStr, 
		AMI_STREAM<elevation_type> *elStr);
AMI_STREAM<boundaryType> *
findBoundariesMain(AMI_STREAM<labelElevType> *labeledWater);


 

/* ********************************************************************** */
/* some helper classes */
/* ********************************************************************** */

class printElevation {
public:
  char *operator()(const elevation_type &p) {
	static char buf[20];
	sprintf(buf, "%.1f", (float)p);
	return buf;
  }
};

class printDirection {
public:
  char *operator()(const direction_type &p) {
	static char buf[20];
	sprintf(buf, "%3d", p);
	return buf;
  }
  char *operator()(const waterWindowBaseType &p) {
	static char buf[20];
	sprintf(buf, "%3d", p.dir);
	return buf;
  }
#if(0)
  char *operator()(const waterWindowBaseType &p) {
	static char buf[3];
	buf[0] = directionSymbol(p.dir);
	buf[1] = '\0';
	return buf;
  }
#endif
};

class printLabel {
public: 
  char *operator()(const labelElevType&p) {
	static char buf[8];
	sprintf(buf, CCLABEL_FMT, p.getLabel());
	return buf;
  }
  char *operator()(const waterGridType &p) {
	static char buf[8];
	sprintf(buf, CCLABEL_FMT, p.getLabel());
	return buf;
  }
  char *operator()(const waterType &p) {
	static char buf[8];
	sprintf(buf, CCLABEL_FMT, p.getLabel());
	return buf;
  }
};

class printDepth {
public:
  char *operator()(const waterGridType &p) {
	static char buf[3];
	sprintf(buf, "%1u", p.depth);
	return buf;
  }
};




char *
verbosedir(std::string s) {
  static char buf[BUFSIZ];
  sprintf(buf, "dump/%s", s.c_str());
  return buf;
}



/* ---------------------------------------------------------------------- */
/* fill the terrain (if necessary) and compute flow direction stream;
   elstr must exist and contain grid data before call, filledstr and
   dirstr are created; elstr is deleted and replaced with the
   classified elstr, which has boundary nodata distinguished from
   inner nodata */
AMI_STREAM<waterWindowBaseType>*
computeFlowDirections(AMI_STREAM<elevation_type>*& elstr,
		      AMI_STREAM<elevation_type>*& filledstr,
		      AMI_STREAM<direction_type>*& dirstr,
		      AMI_STREAM<labelElevType> *& labeledWater) {

  Rtimer rt, rtTotal;
  AMI_STREAM<elevation_type> *elstr_reclass=NULL;
  AMI_STREAM<ElevationWindow > *winstr=NULL;
  AMI_STREAM<plateauStats> *statstr=NULL;
  AMI_STREAM<plateauType> *platstr=NULL;
  AMI_STREAM<waterType> *waterstr=NULL;
  AMI_STREAM<waterGridType> *mergedWaterStr=NULL;
  AMI_STREAM<boundaryType> *boundaryStr=NULL;
  AMI_STREAM<waterWindowType> *waterWindows=NULL;
  
  rt_start(rtTotal);
  assert(elstr && filledstr == NULL && dirstr == NULL && labeledWater == NULL);
  if (stats) {
      stats->comment("------------------------------");
      stats->comment("COMPUTING FLOW DIRECTIONS");
  
      /* adjust nodata -- boundary nodata distinguished from inner
	 nodata */ 
      stats->comment("classifying nodata (inner & boundary)");
  }
  
  elstr_reclass = classifyNodata(elstr);
  delete elstr;
  elstr = elstr_reclass;
  

  /* ---------------------------------------------------------------------- */
  /* find the plateaus. */
  /* ---------------------------------------------------------------------- */
  if (stats) {
      stats->comment("----------",  opt->verbose);
      stats->comment("assigning preliminary directions");
  }
  
  rt_start(rt);
  dirstr = new AMI_STREAM<direction_type>;
  winstr = new AMI_STREAM<ElevationWindow>();
  statstr = new AMI_STREAM<plateauStats>;
  
  platstr = findPlateaus(elstr, nrows, ncols, nodataType::ELEVATION_NODATA,
						 winstr, dirstr, statstr);
 
  delete winstr; /* not used; not made */
  rt_stop(rt);
  
  if (stats) {
      stats->recordTime("findingPlateaus", rt);
      stats->recordLength("plateaus", platstr);
      stats->recordLength("plateau stats", statstr);
  }
  FILL_SAVEALL {
    /* printStream(*stats, statstr); */
    FILL_DEBUG cout << "sort plateauStr (by ij): ";
    AMI_STREAM<plateauType> *tmp = sort(platstr, ijCmpPlateauType());
    printStream2Grid(tmp, nrows, ncols, 
		     verbosedir("label1.asc"), plateauType::printLabel);
    delete tmp;
  }
  

  /* ---------------------------------------------------------------------- */
  /* assign labels and directions & BFS ordering. depressions have
     labels, but no direction information.
  */
  /* ---------------------------------------------------------------------- */
  rt_start(rt);
  waterstr = new AMI_STREAM<waterType>();
  assignDirections(statstr, platstr, waterstr);
  delete platstr;
  delete statstr;
  rt_stop(rt);
  if (stats) {
      stats->recordTime("assigning directions", rt);
      *stats << "maxWatershedCount=" << labelFactory::getLabelCount() << endl;
  }
  
  rt_start(rt);
  mergedWaterStr = merge2waterGrid(waterstr, dirstr, elstr);
  delete dirstr;
  delete waterstr;
  rt_stop(rt);
  if (stats) {
      stats->recordTime("merging", rt);
      stats->recordLength("mergedWaterStr", mergedWaterStr);
  }

 /* ---------------------------------------------------------------------- */
  /* watershed analysis */
  /* IN: mergedWaterStr, labelFactory::... */
  /* ---------------------------------------------------------------------- */
  if (stats) {
      stats->comment("--------------", opt->verbose);
      stats->comment("generating watersheds and watershed graph");
  }

  rt_start(rt);
  waterWindows = new AMI_STREAM<waterWindowType>();
  createWaterWindows(mergedWaterStr, nrows, ncols, waterWindows);
  delete mergedWaterStr;
  rt_stop(rt);
  if (stats) {
      stats->recordTime("creating windows", rt);
      stats->recordLength("waterWindows", waterWindows);
  }

  /* ---------------------------------------------------------------------- */
  rt_start(rt);
  labeledWater = new AMI_STREAM<labelElevType>();
  boundaryStr = new AMI_STREAM<boundaryType>();
  generateWatersheds(&waterWindows, nrows, ncols, labeledWater, boundaryStr);

  /* do we need to make boundaries here?? */
  delete waterWindows;
  /* cout << "bogus boundary length = " << boundaryStr->stream_len() << endl;*/
  assert(boundaryStr->stream_len() == 0);
  delete boundaryStr;
  
  assert(labeledWater->stream_len() == nrows * ncols);
  rt_stop(rt);
  if (stats)
    stats->recordTime("generating watersheds", rt);
  
  /* ---------------------------------------------------------------------- */
  /* find boundaries */
  /* ---------------------------------------------------------------------- */
  FILL_DEBUG cerr << "sort labeledWater (by ij):";
  sort(&labeledWater, ijCmpLabelElevType());

#ifdef SAVE_ASCII  
  cerr << "saving WATERSHED grid as watershed_grid\n";
  printStream2Grid(labeledWater, nrows, ncols, 
		   "watershed.asc", labelElevType::printLabel);
#endif
  boundaryStr = findBoundariesMain(labeledWater);
  
  
  /* ---------------------------------------------------------------------- */
  /* filling */
  /* valid streams are: boundaryStr, labeledWater */
  /* ---------------------------------------------------------------------- */
  rt_start(rt);
  elevation_type *raise;
  /*find the raise elevations */
  
  FILL_DEBUG cerr << "sort boundaryStr (by elev): ";
  sort(&boundaryStr, elevCmpBoundaryType());

  raise = fill_depression(boundaryStr, labelFactory::getLabelCount());
  delete boundaryStr;
  rt_stop(rt);
  if (stats)
    stats->recordTime("filling depressions", rt);
  
  /*fill the terrain*/
  rt_start(rt);
  filledstr = new AMI_STREAM<elevation_type>();
  commit_fill(labeledWater, raise, labelFactory::getLabelCount(), filledstr);
  assert(filledstr->stream_len() == nrows * ncols);
  delete [] raise;
  rt_stop(rt);
  if (stats) {
      stats->recordTime("updating filled grid", rt);
      stats->recordLength("filledstr", filledstr);

      /* ---------------------------------------------------------------------- */
      /* find plateaus again and reassign directions */
      /* ---------------------------------------------------------------------- */

      stats->comment("------------------------------");
      stats->comment("REASSIGNING DIRECTIONS");
  }

  rt_start(rt);
  winstr = NULL;
  dirstr = new AMI_STREAM<direction_type>();
  statstr = new AMI_STREAM<plateauStats>();
  platstr = findPlateaus(filledstr, nrows, ncols, nodataType::ELEVATION_NODATA,
			 winstr, dirstr, statstr);
  rt_stop(rt);
  if (stats) {
      stats->recordTime("findingPlateaus2", rt);
      stats->recordLength("final plateaus", platstr);
      stats->recordLength("final plateau stats", statstr);
  }
  FILL_SAVEALL {
    FILL_DEBUG cout << "sort plateauStr (by ij): "; 
    AMI_STREAM<plateauType> *tmp = sort(platstr, ijCmpPlateauType());
    printStream2Grid(tmp, nrows, ncols, 
		     verbosedir("plateaus.asc"), plateauType::printLabel);
    delete tmp;
  }

  /* assign final directions */
  rt_start(rt);
  waterstr = new AMI_STREAM<waterType>();
  long dc = assignDirections(statstr, platstr, waterstr);
  if(dc && stats) {
    *stats << "WARNING: " << dc << " depressions (islands) detected\n";
  }
  delete platstr;
  delete statstr;
  rt_stop(rt);
  if (stats)
    stats->recordTime("final directions", rt);
  
  /* merge */
  rt_start(rt);
  AMI_STREAM<waterWindowBaseType> *flowStream;
  /*STREAM_TO_OPTION(flowStream, "flowStream");*/
   char path[BUFSIZ];
  char* base_dir = getenv(STREAM_TMPDIR);
  assert(base_dir);
  sprintf(path, "%s/flowStream", base_dir);
  flowStream = new AMI_STREAM<waterWindowBaseType>(path);
  /*flowStream->persist(PERSIST_PERSISTENT); */
  if (stats)
    stats->comment("creating flowStream: ", flowStream->sprint());

  merge2waterBase(waterstr, dirstr, filledstr, flowStream);
  delete waterstr;
  rt_stop(rt);
  if (stats)
    stats->recordTime("merge water,dir,elev to flow", rt);
  rt_stop(rtTotal);
  
#ifdef SAVE_ASCII
  /*write grids as ascii file */
  printGridStream(filledstr, nrows, ncols,
		  "filled_elev.asc", printElevation());
  printGridStream(flowStream, nrows, ncols,  
		  "direction.asc", printDirection());
#endif
  
  if (stats) {
      stats->recordTime("Total compute flow direction running time", rtTotal);
      stats->comment("compute flow directions done.");
  }
   
  return flowStream;
}


/* ---------------------------------------------------------------------- */
void
recordWatersheds(AMI_STREAM<labelElevType> *labeledWater) {
  AMI_err ae;
  long labelCount = 0;
  AMI_STREAM<labelElevType> *tmp;

  if (stats)
    *stats << "--- watershed stats" << endl;
  FILL_DEBUG cout << "sort labeledWater (by wat label): ";
  tmp = sort(labeledWater, labelCmpLabelElevType());

  labelElevType *p;
  cclabel_type prev(LABEL_UNDEF);
  tmp->seek(0);
  while((ae = tmp->read_item(&p)) == AMI_ERROR_NO_ERROR) {
	if(p->getLabel() != prev) {
	  labelCount++;
	  prev = p->getLabel();
	}
  }
  assert(ae == AMI_ERROR_END_OF_STREAM);

  if (stats) {
      *stats << "watershed count = " << labelCount << endl;
      *stats << "---" << endl;
      stats->flush();
  }

  delete tmp;
}





/* ********************************************************************** */
/* assign directions to plateaus that have sinks;
 * reassign labels to depressions (don't drain out).
 * all plateaus are written out to the waterstr. */
long
assignDirections(AMI_STREAM<plateauStats> *statstr, 
		 AMI_STREAM<plateauType> *platstr,
		 AMI_STREAM<waterType> *waterstr) {
  size_t fmem;
  AMI_err ae;
  plateauStats *ps;

  if (stats) {
      stats->comment("----------",  opt->verbose);
      stats->comment("assigning directions on plateaus");
  }
  
  labelFactory::reset();		/* we are relabeling now */
  
  statstr->seek(0);
  platstr->seek(0);
  fmem = getAvailableMemory();
  long depressionCount=0;
  long spillCount=0;
  while((ae = statstr->read_item(&ps)) == AMI_ERROR_NO_ERROR) {
    if(ps->size*sizeof(gridElement) > fmem) {
      cerr << "WARNING: grid larger than memory (ignored)" << endl;
    }
    assert(ps->label != LABEL_NODATA);
    if(ps->hasSpill) {
      spillCount++;
      grid *platGrid = new grid(ps->iMin, ps->jMin, ps->iMax, ps->jMax, 
				ps->size, ps->label);
      platGrid->load(*platstr);
      platGrid->assignDirections(opt->d8 ? 1 : 0);
      platGrid->save(*waterstr); /* this doesn't save labels */
      delete platGrid;
    } else {
      /* depression - just give contiguous labels only */
      depressionCount++;
      cclabel_type label = labelFactory::getNewLabel();
      for(int i=0; i<ps->size; i++) {
	plateauType *pt;
	platstr->read_item(&pt);
	pt->cclabel = label;	/* assign new label */
	waterType wt(*pt);		/* write it out */
	ae = waterstr->write_item(wt);
	assert(ae == AMI_ERROR_NO_ERROR);
      }
    }
  }
  if (stats) {
      *stats << "depression count = " << depressionCount << endl;
      *stats << "spill count = " << spillCount << endl;
  }

  return depressionCount;
}




/* ********************************************************************** */
/* assign directions to plateaus that have sinks;
 * check that there are no depressions.
 */
void
assignFinalDirections(AMI_STREAM<plateauStats> *statstr, 
		      AMI_STREAM<plateauType> *platstr,
		      AMI_STREAM<waterType> *waterstr) {
  AMI_err ae;
  plateauStats *ps;

  if (stats)
    stats->comment("assigning final directions");

  statstr->seek(0);
  platstr->seek(0);
  while((ae = statstr->read_item(&ps)) == AMI_ERROR_NO_ERROR) {
    
    if(ps->hasSpill) {
      grid *platGrid = new grid(ps->iMin, ps->jMin, ps->iMax, ps->jMax, 
				ps->size, ps->label);
      platGrid->load(*platstr);
      platGrid->assignDirections(opt->d8 ? 1 : 0);
      platGrid->save(*waterstr); /* this doesn't save labels */
      delete platGrid;
    } else {
      /* could be legitimate */
      cerr << "WARNING: depression detected: " << *ps << endl;
      continue;
    } 
  }
};



/* ********************************************************************** */
class directionElevationMerger {
public:
  waterGridType operator()(elevation_type el, direction_type dir, 
			   waterType p) { 
    /* check that no (boundary) nodata values got in here */
    assert(el != nodataType::ELEVATION_BOUNDARY);
    assert(!is_nodata(el));		/* p should be a valid grid cell */
    return waterGridType(el, p.getDirection(), p.getLabel(), p.getDepth());
  }
  waterGridType operator()(elevation_type el, direction_type dir) {
    waterGridType wg(el, dir);
    if(el == nodataType::ELEVATION_BOUNDARY) { /* hack XXX (approved) */
      wg.setLabel(LABEL_BOUNDARY); 
    }
    /* nodata => boundary or undef */
    assert(!is_nodata(el) ||
	   (wg.getLabel()==LABEL_BOUNDARY || wg.getLabel()==LABEL_UNDEF)); 
	return wg;
  }
};



/* ---------------------------------------------------------------------- */
AMI_STREAM<waterGridType> *
merge2waterGrid(AMI_STREAM<waterType> *unsortedWaterStr, 
		AMI_STREAM<direction_type> *dirStr, 
		AMI_STREAM<elevation_type> *elStr) {
  AMI_STREAM<waterType> *waterStr;
  
  
  FILL_DEBUG cout << "sort waterStr (by ij): ";
  waterStr = sort(unsortedWaterStr, ijCmpWaterType());

  FILL_SAVEALL printStream2Grid(waterStr, nrows, ncols, 
				verbosedir("platlabels.asc"), printLabel());
 
  AMI_STREAM<waterGridType> *mergedWaterStr = new AMI_STREAM<waterGridType>();
  mergeStreamGridGrid(elStr, dirStr,
		      nrows, ncols,
		      waterStr,
		      directionElevationMerger(),
		      mergedWaterStr);
  delete waterStr;
  FILL_SAVEALL printGridStream(mergedWaterStr, nrows, ncols, 
							   verbosedir("mergedlabels.asc"), printLabel());

  assert(mergedWaterStr->stream_len());
  return mergedWaterStr;
}



/* ---------------------------------------------------------------------- */
void
merge2waterBase(AMI_STREAM<waterType> *unsortedWaterStr, 
		AMI_STREAM<direction_type> *dirStr,
		AMI_STREAM<elevation_type> *elStr, 
		AMI_STREAM<waterWindowBaseType> *merge) {
  AMI_STREAM<waterType> *waterStr;
  FILL_DEBUG cout << "sort waterStr (by ij): ";
  waterStr = sort(unsortedWaterStr, ijCmpWaterType());
  mergeStreamGridGrid(elStr, dirStr,
		      nrows, ncols,
		      waterStr,
		      directionElevationMerger(),
		      merge);
  delete waterStr;
}




/* ---------------------------------------------------------------------- */
/*
 * merge 2 grids and a stream together to form a new grid. 
 * str should be sorted in ij order 
 */
template<class T1, class T2, class T3, class T4, class FUN>
void
mergeStreamGridGrid(AMI_STREAM<T1> *grid1,
		    AMI_STREAM<T2> *grid2,
		    dimension_type rows, dimension_type cols, 
		    AMI_STREAM<T3> *str,
		    FUN fo,
		    AMI_STREAM<T4> *outStream) {
  T1 *t1p;
  T2 *t2p;
  T3 *t3p;
  AMI_err aeUpd, ae;

  
  grid1->seek(0);
  grid2->seek(0);
  str->seek(0);
  aeUpd = str->read_item(&t3p);
  assert(aeUpd == AMI_ERROR_NO_ERROR || aeUpd == AMI_ERROR_END_OF_STREAM);

  for(dimension_type row = 0; row < rows; row++) {
    for(dimension_type col = 0; col < cols; col++) {
      ae = grid1->read_item(&t1p);
      assert(ae == AMI_ERROR_NO_ERROR);
      ae = grid2->read_item(&t2p);
      assert(ae == AMI_ERROR_NO_ERROR);
      
      T4 t4;
      if(aeUpd == AMI_ERROR_NO_ERROR && t3p->i == row && t3p->j == col) {
		/* cell present in stream */
	t4 = fo(*t1p, *t2p, *t3p);
	aeUpd = str->read_item(&t3p);
	assert(aeUpd == AMI_ERROR_NO_ERROR || 
	       aeUpd == AMI_ERROR_END_OF_STREAM);
      } else {					
		/* not in stream */
		t4 = fo(*t1p, *t2p);
      }
      ae = outStream->write_item(t4);
      assert(ae == AMI_ERROR_NO_ERROR);
    }
    /*assert(outStream->stream_len() == (row+1) * cols); */
  }
  assert(outStream->stream_len() == rows * cols);
  return;
}



/* ---------------------------------------------------------------------- */
/* make boundaryStr from labeledWater */
AMI_STREAM<boundaryType> *
findBoundariesMain(AMI_STREAM<labelElevType> *labeledWater) {
  AMI_STREAM<boundaryType> *boundaryStr;
  Rtimer rt;
  
  rt_start(rt);
  boundaryStr = new AMI_STREAM<boundaryType>();
  FILL_SAVEALL printGridStream(labeledWater, nrows, ncols,  
			       verbosedir("labels.asc"), printLabel());
  
  findBoundaries(labeledWater, nrows, ncols, boundaryStr);
  if (stats)
    stats->recordLength("all boundaries", boundaryStr);
  
  FILL_SAVEALL {
    FILL_DEBUG cout << "sort boundaryStr (by ij): ";
    sort(&boundaryStr, ijCmpBoundaryType());
    removeDuplicatesEx(&boundaryStr, ijCmpBoundaryType());
    printStream2Grid(boundaryStr, nrows, ncols,
		     verbosedir("boundary.asc"), boundaryType::print);
  }
  FILL_DEBUG cout << "sort boundaryStr (by wat label): ";
  sort(&boundaryStr, waterCmpBoundaryType());
  removeDuplicatesEx(&boundaryStr, boundaryCmpBoundaryType());
  
  rt_stop(rt);
  if (stats) {
      stats->recordTime("generating boundaries", rt);
      stats->recordLength("boundary stream", boundaryStr);
  }
  
  /*if(GETOPT("veryfillverbose")) printStream(cout, boundaryStr);
	SAVE_ON_OPTION(boundaryStr, "saveBoundaryStream");
  */
  return boundaryStr;
}


