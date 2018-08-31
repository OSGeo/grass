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

#include <grass/iostream/ami.h> 

#include "nodata.h"
#include "common.h"
#include "sortutils.h"
#include "streamutils.h"
#include "ccforest.h"
#include "3scan.h"
/* #include "plateau.h" */
#include "genericWindow.h"

/* globals 
extern statsRecorder *stats;     
extern userOptions *opt;         
extern struct  Cell_head *region; 
extern dimension_type nrows, ncols;
*/


#define NODATA_DEBUG if(0)



/* ********************************************************************** */

elevation_type nodataType::ELEVATION_BOUNDARY;
elevation_type nodataType::ELEVATION_NODATA;

/* ********************************************************************** */

/*  int */
/*  is_nodata(elevation_type el) { */
/*    return (el == nodataType::ELEVATION_BOUNDARY || */
/*  		  el == nodataType::ELEVATION_NODATA); */
/*  } */


int
is_nodata(short x) {
  return (x == nodataType::ELEVATION_BOUNDARY ||
		  x == nodataType::ELEVATION_NODATA);
}
int
is_nodata(int x) {
  return (x == nodataType::ELEVATION_BOUNDARY ||
		  x == nodataType::ELEVATION_NODATA);
}


int
is_nodata(float x) {
  return (x == nodataType::ELEVATION_BOUNDARY ||
		  x == nodataType::ELEVATION_NODATA);
}


int
is_void(elevation_type el) {
  return (el == nodataType::ELEVATION_NODATA);
}




/* ********************************************************************** */
class detectEdgeNodata {
private:
  AMI_STREAM<nodataType> *nodataStream;
  AMI_STREAM<elevation_type> *elevStream;
  queue<nodataType> *nodataQueue;
  ccforest<cclabel_type> colTree;
  nodataType *getNodataForward(dimension_type i, dimension_type j,
			       dimension_type nr, dimension_type n);
  const dimension_type nr, nc;
  const elevation_type nodata;
public:
  detectEdgeNodata(const dimension_type nrows, const dimension_type nc, 
		   const elevation_type nodata);
  ~detectEdgeNodata();
  void processWindow(dimension_type row, dimension_type col,
		     elevation_type &point,
		     elevation_type *a,
		     elevation_type *b,
		     elevation_type *c);
  void generateNodata(AMI_STREAM<elevation_type> &elstr);
  void relabelNodata();
  AMI_STREAM<elevation_type> *merge();
  AMI_STREAM<nodataType> *getNodata() { return nodataStream; }
};


/* ********************************************************************** */
detectEdgeNodata::detectEdgeNodata(const dimension_type nrows, 
				   const dimension_type ncols, 
				   const elevation_type gnodata)
  : nr(nrows), nc(ncols), nodata(gnodata)  {
  nodataStream = new AMI_STREAM<nodataType>();
  elevStream = new AMI_STREAM<elevation_type>();
}


/* ********************************************************************** */
detectEdgeNodata::~detectEdgeNodata() {
  delete nodataStream;
  delete elevStream;
}


/* ********************************************************************** */
/* return a pointer to three plateauType structures, starting at
   location i,j. caller should check valid field in returned
   structs. */
nodataType *
detectEdgeNodata::getNodataForward(dimension_type i, dimension_type j,
				   dimension_type nr, dimension_type nc) {
  bool ok;
  static nodataType ptarr[3];	/* return value */
  nodataType pt;

  ok = nodataQueue->peek(0, &pt);
  while(ok && (pt.i < i || (pt.i==i && pt.j<j))) {
	nodataQueue->dequeue(&pt);		/* past needing this, so remove */
	ok = nodataQueue->peek(0, &pt);
  }
  if(ok && pt.i == i && pt.j == j) {
	nodataQueue->dequeue(&pt);		/* found it, so remove */
	ptarr[0] = pt;
  } else {
	ptarr[0].invalidate();
  }
  /* locate next two, if possible */
  for(int kk=0,k=1; k<3; k++) {
	ok = nodataQueue->peek(kk, &pt);
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
  nodataQueue->peek(0, &pt);
  cout << "queue length = " << nodataQueue->length() 
	   << "; head=" << pt << endl;
#endif
  return ptarr;  
}


/* ********************************************************************** */


void
detectEdgeNodata::processWindow(dimension_type row, dimension_type col,
				elevation_type &point,
				elevation_type *a,
				elevation_type *b,
				elevation_type *c) {
  AMI_err ae;
  static nodataType prevCell;	/* cell on left (gets initialized) */

  assert(row>=0);
  assert(col>=0);

  /* create window and write out */
  ElevationWindow win(a, b, c);
  fillPit(win);          /* fill pit in window */
  ae = elevStream->write_item(win.get());
  assert(ae == AMI_ERROR_NO_ERROR);


  /* only interested in nodata in this pass */
  if(win.get() !=  nodata) {
	prevCell.label = LABEL_UNDEF;
	return;
  }

  if(col == 0) prevCell.label = LABEL_UNDEF; /* no left cell */

  /* now check for continuing plateaus */
  nodataType *ptarr = 
	getNodataForward(row-1, col-1, nr, nc);

  /* make sure we use boundary label if appropriate */
  cclabel_type crtlabel;
  crtlabel = (IS_BOUNDARY(row,col,nr, nc) ? LABEL_BOUNDARY : LABEL_UNDEF);

  for(int i=0; i<4; i++) {
	if(win.get(i) != win.get()) continue; /* only interesting if same elev */

	/* determine label for cell */
	cclabel_type label = LABEL_UNDEF;
	if(i<3) {
	  if(ptarr[i].valid) label = ptarr[i].label;
	} else {
	  if(prevCell.valid) label = prevCell.label;
	}

	/* check for collisions */
	if(label != LABEL_UNDEF) {
	  if (crtlabel == LABEL_UNDEF) {
		crtlabel = label;
	  } else if(crtlabel != label) {  		  /* collision!! */
		/* pick smaller label, but prefer nodata */
		if(crtlabel==LABEL_BOUNDARY || crtlabel<label) { 
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
	crtlabel = labelFactory::getNewLabel();
  }

  /* write this plateau point to the plateau stream */
  nodataType pt;
  prevCell = pt = nodataType(row, col, crtlabel);
  nodataQueue->enqueue(pt);

  /* NODATA_DEBUG *stats << "inserting " << pt << endl; */
  
  nodataStream->write_item(pt);	/*  save to file for later use */
}


/* ********************************************************************** */
void
detectEdgeNodata::generateNodata(AMI_STREAM<elevation_type> &elstr) { 
  nodataQueue = new queue<nodataType>();
  scan3(elstr, nr, nc, nodata, *this); 
  delete nodataQueue;
}


/* ********************************************************************** */
/* collapse labels; remove nodata regions */
void
detectEdgeNodata::relabelNodata() {
  AMI_err ae;
  nodataType *pt;

  /* sort by label */
  if (stats)
    NODATA_DEBUG *stats << "sort nodataStream (by nodata label): ";
  AMI_STREAM<nodataType> *sortedInStream;
  sortedInStream = sort(nodataStream, labelCmpNodataType());
  delete nodataStream;

  nodataStream = new AMI_STREAM<nodataType>();

  while((ae = sortedInStream->read_item(&pt)) == AMI_ERROR_NO_ERROR) {
	cclabel_type root = colTree.findNextRoot(pt->label);
	assert(root <= pt->label);
	pt->label = root;
	ae = nodataStream->write_item(*pt);
	assert(ae == AMI_ERROR_NO_ERROR);
  }

  delete sortedInStream;
}


/* ********************************************************************** */
AMI_STREAM<elevation_type> *
detectEdgeNodata::merge() {
 
  if (stats)
    NODATA_DEBUG *stats << "sort  nodataStream (by ij): ";
  /*
    AMI_STREAM<nodataType> *sortedNodataStream;
    sortedNodataStream = sort(nodataStream, ijCmpNodataType());
    delete nodataStream;
    nodataStream=sortedNodataStream;
  */
  sort(&nodataStream, ijCmpNodataType());
  //note: nodataStream gets deleted and replaced with the sorted stream

  AMI_STREAM<elevation_type> *mergeStr;
  mergeStr = mergeStream2Grid(elevStream, nrows, ncols, 
			      nodataStream, nodataType2elevation_type());

  return mergeStr;
}



/* ********************************************************************** */
/* ********************************************************************** */


AMI_STREAM<elevation_type> *
classifyNodata(AMI_STREAM<elevation_type> *elstr) {
  Rtimer rt;

  rt_start(rt);
  if (stats)
    stats->comment("finding nodata", opt->verbose);
  detectEdgeNodata md(nrows, ncols, nodataType::ELEVATION_NODATA);
  md.generateNodata(*elstr);
  if (stats)
    *stats << "nodata stream length = " << md.getNodata()->stream_len() << endl;
  {
    char * foo;
    md.getNodata()->name(&foo); 
    if (stats)
      *stats << "nodata stream name: " << foo << endl;
  }
  rt_stop(rt);
  if (stats)
    stats->recordTime("classifyNodata::generate nodata", rt);

  rt_start(rt);
  if (stats)
    stats->comment("relabeling nodata",  opt->verbose);
  md.relabelNodata();  /* re-assign labels (combine connected plateaus) */
  rt_stop(rt);
  if (stats)
    stats->recordTime("classifyNodata::relabeling",  rt);
  
  rt_start(rt);
  if (stats)
    stats->comment("merging relabeled grid",  opt->verbose);
  AMI_STREAM<elevation_type> *mergeStr;
  mergeStr = md.merge();
  rt_stop(rt);
  if (stats)
    stats->recordTime("classifyNodata::merge",  rt);

  mergeStr->seek(0);
  return mergeStr;
}

/* ********************************************************************** */
