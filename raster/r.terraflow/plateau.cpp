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

#include <grass/iostream/ami.h> /* for queue */

#include "plateau.h"
#include "common.h"
#include "streamutils.h"
#include "sortutils.h"
#include "types.h"
#include "ccforest.h"
#include "3scan.h"

#define PLAT_DEBUG if(0)




/* ********************************************************************** */
/* ********************************************************************** */

/* this is a function object to create the direction and plateau
 * streams given a window */

class detectPlateaus {
private:
  AMI_STREAM<direction_type> *dirStream;
  AMI_STREAM<plateauType> *platStream;
  AMI_STREAM<ElevationWindow > *winStream;
  queue<direction_type> *dirQueue;
  queue<plateauType> *platQueue;
  ccforest<cclabel_type> colTree;
  plateauType *getPlateauForward(dimension_type i, dimension_type j,
				 dimension_type nr, dimension_type n);
  direction_type *getDirectionForward(dimension_type i, dimension_type j,
				      dimension_type nr, dimension_type n);
  const dimension_type nrows;
  const dimension_type ncols;
  const elevation_type nodata_value;
public:
  detectPlateaus(const dimension_type gnrows,const dimension_type gncols,
				 const elevation_type gnodata_value,
				 AMI_STREAM<direction_type>* dirstr,
				 AMI_STREAM<ElevationWindow > *winstr);
  ~detectPlateaus();
  void processWindow(dimension_type row, dimension_type col,
					 elevation_type *a,
					 elevation_type *b,
					 elevation_type *c);
  void generatePlateaus(AMI_STREAM<elevation_type> &elstr);
  void removeDuplicates();
  void relabelPlateaus();
  void generateStats(AMI_STREAM<plateauStats> *statStr);
  AMI_STREAM<plateauType> *getPlateaus() { return platStream; }
};


/* ********************************************************************** */


detectPlateaus::detectPlateaus(const dimension_type gnrows,
							   const dimension_type gncols,
							   const elevation_type gnodata_value,
							   AMI_STREAM<direction_type>* gdirstr,
							   AMI_STREAM<ElevationWindow > *gwinstr):
  dirStream(gdirstr), winStream(gwinstr), 
  nrows(gnrows), ncols(gncols), nodata_value(gnodata_value) {
  platStream = new AMI_STREAM<plateauType>();
}

detectPlateaus::~detectPlateaus() {
  /*delete platStream;	*/		/* user must delete */
}



/* ********************************************************************** */
/* return a pointer to three plateauType structures, starting at
   location i,j. caller should check valid field in returned
   structs. */
plateauType *
detectPlateaus::getPlateauForward(dimension_type i, dimension_type j,
				  dimension_type nr, dimension_type nc) {
  bool ok;
  static plateauType ptarr[3];	/* return value */
  plateauType pt;
  
  ok = platQueue->peek(0, &pt);
  while(ok && (pt.i < i || (pt.i==i && pt.j<j))) {
	platQueue->dequeue(&pt);		/* past needing this, so remove */
	ok = platQueue->peek(0, &pt);
  }
  if(ok && pt.i == i && pt.j == j) {
	platQueue->dequeue(&pt);		/* found it, so remove */
	ptarr[0] = pt;
  } else {
	ptarr[0].invalidate();
  }
  /* locate next two, if possible */
  for(int kk=0,k=1; k<3; k++) {
	ok = platQueue->peek(kk, &pt);
	if(ok && pt.i == i && pt.j == j+k) {
	  ptarr[k] = pt;
	  kk++; /* found something, so need to peek further forward */
	} else {
	  ptarr[k].invalidate();
	}
  }

#if(0)
  cout << "request at " << i << "," << j << " returns: " <<
	ptarr[0] << ptarr[1] << ptarr[2] << endl;
  platQueue->peek(0, &pt);
  cout << "queue length = " << platQueue->length() 
	   << "; head=" << pt << endl;
#endif
  return ptarr;  
}



/* ********************************************************************** */
/* should be called for each element in the grid */
direction_type *
detectPlateaus::getDirectionForward(dimension_type i, dimension_type j,
				    dimension_type nr, dimension_type nc) {
  static direction_type dirarr[3]; /* return value */
  
  dirarr[0] = 0;
  dirarr[1] = 0;
  dirarr[2] = 0;
  
  assert(i<nr-1);
  assert(nc>3);

  if(i>=0) {
	if(!(i==0 && j==-1)) dirQueue->dequeue(dirarr);
	if(j == -1) dirarr[0] = 0;
	if(j+1 < nc) dirQueue->peek(0, dirarr+1);
	if(j+2 < nc) dirQueue->peek(1, dirarr+2);
  }

#if(0)
  cout << "\t\t\trequest at " << i << "," << j << " returns: " <<
	dirarr[0] << " " << dirarr[1] << " " << dirarr[2] << "\t";
  direction_type dir;
  dirQueue->peek(0, &dir);
  cout << "queue length = " << dirQueue->length() 
	   << "; head=" << dir << endl;
#endif

  return dirarr;
}



/* ********************************************************************** */
void
detectPlateaus::processWindow(dimension_type row, dimension_type col,
			      elevation_type *a,
			      elevation_type *b,
			      elevation_type *c) {
  AMI_err ae;
  static plateauType prevPlat;	/*  cell on left (auto-initialized) */
  direction_type dir;

  assert(row>=0);
  assert(col>=0);

  /* create window and write out */
  ElevationWindow win(a, b, c);

  /* compute direction; pits should have been filled */
  dir = encodeDirection(win, nrows, ncols, row, col);
  dirQueue->enqueue(dir); /* write dir to dir stream */
  ae = dirStream->write_item(dir);	/* save to file for later use */
  assert(ae == AMI_ERROR_NO_ERROR);

  /* must always read to keep in sync */
  direction_type *dirarr =
	getDirectionForward(row-1, col-1, nrows, ncols);
  
  /* if(dir == DIRECTION_UNDEF) return; */
  if(is_nodata(win.get())) {	/* if nodata, get outa here */
	prevPlat.cclabel = LABEL_UNDEF;
	return;
  }

  if(col == 0) prevPlat.cclabel = LABEL_UNDEF; /* no left cell */
  
  /* now check for continuing plateaus */
  plateauType *ptarr = 
	getPlateauForward(row-1, col-1, nrows, ncols);
  cclabel_type crtlabel=LABEL_UNDEF;
  for(int i=0; i<4; i++) {
	if(win.get(i) != win.get()) continue; /* only interesting if same elev */

	/* determine label for cell */
	cclabel_type label = LABEL_UNDEF;
	if(i<3) {
	  if(ptarr[i].valid) label = ptarr[i].cclabel;
	} else {
	  if(prevPlat.valid) label = prevPlat.cclabel;
	}

	/* check for collisions */
	if(label != LABEL_UNDEF) {
	  if (crtlabel == LABEL_UNDEF) {
		crtlabel = label;
	  } else if(crtlabel != label) {  		  /* collision!! */
		/* pick smaller label */
		if(crtlabel<label) { 
		  colTree.insert(crtlabel, label);
		} else {
		  colTree.insert(label, crtlabel);
		  crtlabel = label;
		}
	  }
	}
  }
  
  /* assign label if required */
  if(crtlabel == LABEL_UNDEF) {
    /* if we have a direction, we're done.  we are not part of a known
       plateau. if we are part(neighbor) of a plateau to be identified
       later, our neighbor will write us later. */
    if(dir > 0) {
      prevPlat = plateauType(row, col, dir);
      PLAT_DEBUG cout << "skipping " << prevPlat << endl;
      return;
    }
    crtlabel = labelFactory::getNewLabel();
  }

  /* check boundaries that are also part of plateau (but didnt know it) */
  for(int i=0; i<4; i++) {
	direction_type ndir(0);
	if(win.get(i) != win.get()) continue; /* only interesting if same elev */
	
	/* determine direction for cell */
	if(i<3) {
	  ndir = dirarr[i];
	} else {
	  if(prevPlat.valid) ndir = prevPlat.dir;
	}

	/* check for boundaries */
	if(ndir > 0) {				/* has direction */
	  plateauType nbor;
	  if(i<3) {
	    nbor = plateauType(row-1, col+i-1, ndir, crtlabel);
	  } else {
	    nbor = plateauType(row, col-1, ndir, crtlabel);
	  }	 
	  if((nbor.i >= 0) & (nbor.j >= 0)) { /* make sure nbor on grid;
											 this can happen because
											 we pad the grid with
											 nodata */
	    PLAT_DEBUG cout << "neighbor insert " << nbor << endl;
	    ae = platStream->write_item(nbor);
	    assert(ae == AMI_ERROR_NO_ERROR);
	  }
	}
  } /* for i */

  /* write this plateau point to the plateau stream */
  plateauType pt;
  prevPlat = pt = plateauType(row, col, dir, crtlabel);
  platQueue->enqueue(pt);
  
  PLAT_DEBUG cout << "inserting " << pt << endl;
  
  platStream->write_item(pt);	/* save to file for later use */
}



/* ********************************************************************** */
void
detectPlateaus::generatePlateaus(AMI_STREAM<elevation_type> &elstr) { 
  dirQueue = new queue<direction_type>();
  platQueue = new queue<plateauType>();
  /* scan3(elstr, hdr, hdr.get_nodata(), *this);  */
  //  if (opt->verbose) STRACE("starting memscan");
  memoryScan(elstr, nrows, ncols, nodata_value, *this); 
  //if (opt->verbose) STRACE("memscan done");
  delete dirQueue;
  delete platQueue;
}




/* ********************************************************************** */
class duplicateFixer {
  ccforest<cclabel_type> *colTree;
public:
  duplicateFixer(ccforest<cclabel_type> *p) : colTree(p) {};
  int compare(const plateauType &a, const plateauType &b) {
	int c = ijCmpPlateauType::compare(a,b);
	if(c==0 && 	(a.cclabel != b.cclabel)) {	/* collision	   */
	  if(a.cclabel<b.cclabel) { 
		colTree->insert(a.cclabel, b.cclabel);
	  } else {
		colTree->insert(b.cclabel, a.cclabel);
	  }
	}
	return c;
  }
};



/* ********************************************************************** */
/* take out plateau elements that were generated multiple times */
void
detectPlateaus::removeDuplicates() {
  PLAT_DEBUG cout << "sort plateauStream (by ij): ";
  sort(&platStream, ijCmpPlateauType());
  ::removeDuplicatesEx(&platStream, duplicateFixer(&colTree));
}




/* ********************************************************************** */
/* collapse labels; remove nodata regions */
void
detectPlateaus::relabelPlateaus() {
  AMI_err ae;
  plateauType *pt;

  AMI_STREAM<plateauType> *sortedInStr;
  PLAT_DEBUG cout << "sort plateauStream (by label): "; 
  sortedInStr = sort(platStream, labelCmpPlateauType());
  delete platStream;

  platStream = new AMI_STREAM<plateauType>;
  sortedInStr->seek(0);
  
  /* 
	 cout << "EDGESTREAM:" << endl; colTree.printEdgeStream();
	 cout << "ROOTSTREAM:" << endl; colTree.printRootStream();
	 cout << "RELABELING:" << endl;
  */
  while((ae = sortedInStr->read_item(&pt)) == AMI_ERROR_NO_ERROR) {
	cclabel_type root = colTree.findNextRoot(pt->cclabel);
	assert(root <= pt->cclabel);
	assert(root >= LABEL_START);
	pt->cclabel = root;
	ae = platStream->write_item(*pt);
	assert(ae == AMI_ERROR_NO_ERROR);
	/* cout << *pt << endl; */
  }
  delete sortedInStr;
}



/* ********************************************************************** */

void
detectPlateaus::generateStats(AMI_STREAM<plateauStats> *statStr) {
  AMI_err ae;
  plateauType *pt;

  /* sort by label */
  AMI_STREAM<plateauType> *sortedStream;
  PLAT_DEBUG cout << "sort plateauStream (by label): ";
  sortedStream = sort(platStream, labelCmpPlateauType());
  delete platStream;

  plateauStats labelStats = plateauStats();
  sortedStream->seek(0);
  while((ae = sortedStream->read_item(&pt)) == AMI_ERROR_NO_ERROR) {
	if(pt->cclabel != labelStats.label) {
	  if(labelStats.label != LABEL_UNDEF) {
		ae = statStr->write_item(labelStats);
		assert(ae == AMI_ERROR_NO_ERROR);
	  }
	  labelStats = plateauStats(pt->cclabel);
	}
	labelStats.add(*pt);
  }
	
  ae = statStr->write_item(labelStats);
  assert(ae == AMI_ERROR_NO_ERROR);

  platStream = sortedStream;
}



/* ********************************************************************** */
/* ********************************************************************** */

AMI_STREAM<plateauType> *
findPlateaus(AMI_STREAM<elevation_type> *elstr,
	     const dimension_type nrows, const dimension_type ncols,
	     const elevation_type nodata_value,
	     AMI_STREAM<ElevationWindow > *winstr,
	     AMI_STREAM<direction_type> *dirStr,
	     AMI_STREAM<plateauStats> *statStr) {
  Rtimer rt;
  
  labelFactory::reset();
  
  /* find plateaus */
  rt_start(rt);
  if (stats) {
      stats->comment("----------",  opt->verbose);
      stats->comment("finding flat areas (plateaus and depressions)");
  }
  detectPlateaus md(nrows, ncols,nodata_value, dirStr, winstr);
  md.generatePlateaus(*elstr);
  rt_stop(rt);
  if (stats) {
      stats->recordTime("findPlateaus::generate plateaus", rt);
      stats->recordLength("plateaus", md.getPlateaus());
  }

  rt_start(rt);
  if (stats)
    stats->comment("removing duplicate plateaus",  opt->verbose);
  md.removeDuplicates(); /* get rid of duplicates of same plateau point */
  rt_stop(rt);
  if (stats) {
      stats->recordTime("findPlateaus::removing duplicates",  rt);
      stats->recordLength("plateaus", md.getPlateaus());
  }
  
#if(0)
  { /* XXX */
    AMI_STREAM<plateauType> *tmp = sort(md.getPlateaus(), ijCmpPlateauType());
    printStream2Grid(tmp, nrows, ncols,
		     "label0.asc", plateauType::printLabel);
    delete tmp;
  }
#endif

  rt_start(rt);
  if (stats)
    stats->comment("relabeling plateaus",  opt->verbose);
  md.relabelPlateaus();  /* re-assign labels (combine connected plateaus) */
  rt_stop(rt);
  if (stats) {
      stats->recordTime("findPlateaus::relabeling",  rt);
      stats->recordLength("plateaus", md.getPlateaus());
  }
  
  rt_start(rt);
  if (stats)
    stats->comment("generating plateau statistics",  opt->verbose);
  md.generateStats(statStr);
  rt_stop(rt);
  if (stats) {
      stats->recordTime("findPlateaus::generating stats",  rt);
      stats->recordLength("plateaus", md.getPlateaus());
  }
  dirStr->seek(0);
  return md.getPlateaus();
}

/* ********************************************************************** */

