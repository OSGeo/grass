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


#include <assert.h>
#include <iostream>

#include <grass/iostream/ami.h>


#include "3scan.h"
#include "water.h"
#include "streamutils.h"
#include "sortutils.h"

#define WATER_DEBUG if(0)
#define XXX if(0)


char *
labelElevType::printLabel(const  labelElevType  &p) {
  static char buf[8];
  sprintf(buf, CCLABEL_FMT, p.label);
  return buf;
}






/* smaller elevation, depth is smaller priority */
int 
fillPriority::compare(const fillPriority &a, const fillPriority &b) {
  if(a.el < b.el) return -1;
  if(a.el > b.el) return 1;
  
  if(a.depth < b.depth) return -1;
  if(a.depth > b.depth) return 1;
  
  if(a.i < b.i) return -1;
  if(a.i > b.i) return 1;
  
  if(a.j < b.j) return -1;
  if(a.j > b.j) return 1;
  
  return 0;
}

int
fillPriority::qscompare(const void *a, const void *b) {
  fillPriority *x = (fillPriority*)a;
  fillPriority *y = (fillPriority*)b;
  return compare(*x, *y);
}

int 
operator<(const fillPriority &a, const fillPriority &b) {
  if(a.el < b.el) return 1;
  if(a.el > b.el) return 0;
  
  if(a.depth < b.depth) return 1;
  if(a.depth > b.depth) return 0;
  
  if(a.i < b.i) return 1;
  if(a.i > b.i) return 0;
  
  if(a.j < b.j) return 1;
  if(a.j > b.j) return 0;
  
  return 0;
}

int
operator<=(const fillPriority &a, const fillPriority &b) {
  if(a.el < b.el) return 1;
  if(a.el > b.el) return 0;
  
  if(a.depth < b.depth) return 1;
  if(a.depth > b.depth) return 0;
  
  if(a.i < b.i) return 1;
  if(a.i > b.i) return 0;
  
  if(a.j < b.j) return 1;
  if(a.j > b.j) return 0;
  
  return 1;
}

int 
operator>(const fillPriority &a, const fillPriority &b) {
  if(a.el < b.el) return 0;
  if(a.el > b.el) return 1;
  
  if(a.depth < b.depth) return 0;
  if(a.depth > b.depth) return 1;
  
  if(a.i < b.i) return 0;
  if(a.i > b.i) return 1;
  
  if(a.j < b.j) return 0;
  if(a.j > b.j) return 1;
  
  return 0;
}


int 
operator==(const fillPriority &a, const fillPriority &b) {
  return (a.el == b.el) 
	&& (a.depth == b.depth) 
	&& (a.i == b.i)
	&& (a.j == b.j);
}


int 
operator!=(const fillPriority &a, const fillPriority &b) {
  return (a.el != b.el) 
	|| (a.depth != b.depth) 
	|| (a.i != b.i)
	|| (a.j != b.j);	
}


ostream&
operator << (ostream& s, const fillPriority &p) {
  return s << "[fillPriority el=" << p.el 
		   << ", d=" << p.depth << ", "
		   << p.i  << ","
		   << p.j << "]";
}



/* ********************************************************************** */


ostream&
operator << (ostream& s, const labelElevType &p) {
  return s << (ijBaseType)p << " "
		   << "el=" << p.el << ", "
		   << p.label;
}

/* ********************************************************************** */

char *
waterType::printLabel(const waterType &p) {
  static char buf[8];
  sprintf(buf, CCLABEL_FMT, p.label);
  return buf;
}

/* ********************************************************************** */


char *
boundaryType::print(const boundaryType &p) {
  static char buf[4];
  if(p.isValid()) {
	buf[0] = '1';
  } else {
	buf[0] = '0';
  }
  buf[1] = '\0';
  
  return buf;
}


ostream&
operator << (ostream& s, const boundaryType &p) {
  return s << "[boundaryType "  
		   << (labelElevType)p << ", "
		   << p.label2 << "]";
}



/* ********************************************************************** */
/* ********************************************************************** */

class waterWindower {
private:
  AMI_STREAM<waterWindowType> *waterWindows;
public:
  waterWindower(AMI_STREAM<waterWindowType> *str) :
	waterWindows(str) {};
  void processWindow(dimension_type i, dimension_type j, 
		     waterGridType &point,
		     waterWindowBaseType *a,
		     waterWindowBaseType *b,
		     waterWindowBaseType *c);
};

void
waterWindower::processWindow(dimension_type i, dimension_type j, 
			     waterGridType &point,
			     waterWindowBaseType *a,
			     waterWindowBaseType *b,
			     waterWindowBaseType *c) {
  waterWindowType win = waterWindowType(i, j, point.getLabel(), a, b, c);
  AMI_err ae = waterWindows->write_item(win);
  assert(ae == AMI_ERROR_NO_ERROR);
}

void
createWaterWindows(AMI_STREAM<waterGridType> *mergedWaterStr, 
		   const dimension_type nrows, const dimension_type ncols,
		   AMI_STREAM<waterWindowType> *waterWindows) {
  if (stats)
    stats->comment("creating windows", opt->verbose);
  waterWindower winfo(waterWindows);
  waterWindowBaseType nodata;
  assert(mergedWaterStr->stream_len() > 0);
  if (stats)
    stats->comment("warning: using slower scan", opt->verbose);
  scan3(*mergedWaterStr, nrows, ncols, nodata, winfo);
}


/* ********************************************************************** */
/* ********************************************************************** */


/*
 * push labels to upslope neighbors 
 */
void
generateWatersheds(AMI_STREAM<waterWindowType> **waterWindows,
				   const dimension_type nrows, const dimension_type ncols,
				   AMI_STREAM<labelElevType> *labeledWater, 
				   AMI_STREAM<boundaryType> *boundaryStr) {
  AMI_err ae;
  waterWindowType *winp, prevWin;
  assert(prevWin.getDepth() == DEPTH_INITIAL);
  EMPQueueAdaptive<fillPLabel, fillPriority> *pq;

  if (stats)
    stats->comment("generateWatersheds", opt->verbose);

  assert((*waterWindows)->stream_len() == (nrows * ncols));

  WATER_DEBUG cout << "sort waterWindowsStream (by priority): ";
  sort(waterWindows, priorityCmpWaterWindowType());
  
  pq = new EMPQueueAdaptive<fillPLabel, fillPriority>();
  
/*   if(GETOPT("alwaysUseExternalPQ")) { */
/*      pq->makeExternal(); */
/*    } */
/*    if(GETOPT("useDebugPQ")) { */
/*      pq->makeExternalDebug(); */
/*    } */
  
  if (stats)
    stats->comment("starting generate watersheds main loop", opt->verbose);
  
  assert((*waterWindows)->stream_len() == (nrows * ncols));
  /* not really in a grid, so row, col are not valid (but count correct) */
  for(dimension_type row=0; row<nrows; row++) {
    for(dimension_type col=0; col<ncols; col++) {
      ae = (*waterWindows)->read_item(&winp);
      assert(ae == AMI_ERROR_NO_ERROR);
      
      /* make sure it's sorted; prevWin default constructor should be ok */
      assert(winp->getPriority() > prevWin.getPriority());
      prevWin = *winp;
      
      XXX winp->sanityCheck();
      /* cout << "--- PROC: " << *winp << endl; */
      /* get my label(s) */
      fillPLabel plabel;		/* from the PQ */
      fillPriority prio;
      cclabel_type label = winp->getLabel();
#ifndef NDEBUG
      {
	/* check to see if things are coming out of the pq in
	   order. just peek at the next one */
	fillPLabel tmp;
	XXX winp->sanityCheck();
	pq->min(tmp);
	/* XXX pq->verify(); */
	XXX winp->sanityCheck();
	assert(pq->is_empty() || winp->getPriority() <= tmp.getPriority());
      }
#endif
      while(pq->min(plabel) &&
			((prio=plabel.getPriority()) == winp->getPriority())) {
		/* XXX pq->verify(); */
		XXX winp->sanityCheck();
		pq->extract_min(plabel);
		/* XXX pq->verify(); */
		XXX winp->sanityCheck();
		if(label == LABEL_UNDEF) label = plabel.getLabel();
      } 
      /* no label! assign a new one */
      if((label==LABEL_UNDEF) && (!is_nodata(winp->getElevation()))) {
#ifndef NDEBUG
	{
	  /* check to see if things are coming out of the pq in
	     order. just peek at the next one */
	  fillPLabel tmp;		
	  XXX winp->sanityCheck();
	  pq->min(tmp);
	  /* XXX pq->verify(); */
	  XXX winp->sanityCheck();
	  assert(pq->is_empty() || winp->getPriority() <= tmp.getPriority());
	}
#endif
	if(IS_BOUNDARY(winp->i,winp->j,nrows, ncols)) {  /* edge of grid */
	  assert(!is_nodata(winp->getElevation()));
	  label = LABEL_BOUNDARY; /* reserved for watersheds draining
				     out of grid */
	} else {
	  label = labelFactory::getNewLabel();
	}
      }
      winp->setLabel(label);
      
      /* push label to 'upslope' neighbors.  let's assume that the
       * edges cause no problems, since they have no directions...  */
      if(label != LABEL_UNDEF) {
	int k=0;
	for(int i=-1; i<2; i++) {
	  for(int j=-1; j<2; j++) {
	    assert(k==linear(i,j));
	    if(!is_nodata(winp->getElevation(k))
	       && winp->drainsFrom(i,j)) { /* points to me */
	      assert(i || j);
	      prio = fillPriority(winp->getElevation(k),
				  winp->getDepth(k),
				  winp->i + i, winp->j + j);
#ifndef NDEBUG
	      /* dont insert if precedes us */
	      if(winp->getPriority() < prio) {
		fillPLabel plabel(prio, label);
		pq->insert(plabel);
	      } else {			/* trying to send a label backward */
		cerr << "WARNING: time travel attempted" << endl; 
		cerr << "inst priority is " << prio << endl;
		cerr << "source is " << *winp << "; prio=" 
		     << winp->getPriority() << endl;
		assert(0);
	      }
#else
	      fillPLabel plabel(prio, label);
	      pq->insert(plabel);
#endif
	    }
	    k++;
	  }
	}
      }
      
      /* write myself to output */
      ae = labeledWater->write_item(winp->getCenter());
      assert(ae == AMI_ERROR_NO_ERROR);
    }
  }
  
  assert(pq->is_empty());
  delete pq;
  
  if (stats)
    stats->comment("done with generate watersheds", opt->verbose);
}



/* ********************************************************************** */


class boundaryDetector {
private:
  const dimension_type nrows, ncols;
  AMI_STREAM<boundaryType> *boundaryStr;
  void processPair(labelElevType &pt, 
		   dimension_type i, dimension_type j, labelElevType &n);
public:
  boundaryDetector(AMI_STREAM<boundaryType> *str, 
				   const dimension_type gnrows, const dimension_type gncols) 
    : nrows(gnrows), ncols(gncols), boundaryStr(str) {};
  
  void processWindow(dimension_type i, dimension_type j, 
		     labelElevType &point,
		     labelElevType *a,
		     labelElevType *b,
		     labelElevType *c);
};

template<class T>
T mymax(T a, T b) {
  return (a>b?a:b);
}

void
boundaryDetector::processPair(labelElevType &pt, 
			      dimension_type i, dimension_type j,
			      labelElevType &n) {
  if(n.getLabel() != LABEL_UNDEF && pt.getLabel() != n.getLabel()) {
    boundaryType bt(pt, mymax(pt.getElevation(), n.getElevation()), 
		    n.getLabel());
    AMI_err ae = boundaryStr->write_item(bt);
    assert(ae == AMI_ERROR_NO_ERROR);
  } else if(IS_BOUNDARY(i,j,nrows, ncols) && pt.getLabel() != LABEL_BOUNDARY) {
    /* this part makes sure that regions on the grid edge
       are considered 'boundary' */
    boundaryType bt(pt, LABEL_BOUNDARY);
    AMI_err ae = boundaryStr->write_item(bt);
    assert(ae == AMI_ERROR_NO_ERROR);
  }
}

void
boundaryDetector::processWindow(dimension_type i, dimension_type j, 
				labelElevType &point,
				labelElevType *a,
				labelElevType *b,
				labelElevType *c) {
  if(point.getLabel() == LABEL_UNDEF) return;
  /* NODATA_FIX */
  /* don't use the nodata as the boundary. */
  /* if(point.getLabel() == LABEL_NODATA) return; */
  assert(point.getLabel() != LABEL_NODATA);
  
  for(int k=0; k<3; k++) {
	processPair(point, i, j, a[k]);
	processPair(point, i, j, b[k]);
	processPair(point, i, j, c[k]);
  }
  /* processPair(point, i, j, b[0]); */
}


/* ********************************************************************** */

void
findBoundaries(AMI_STREAM<labelElevType> *labeledWater,
	       const dimension_type nrows, const dimension_type ncols,	 
	       AMI_STREAM<boundaryType> *boundaryStr) {
  if (stats)
    stats->comment("creating windows", opt->verbose);
  boundaryDetector det(boundaryStr, nrows, ncols);
  /* cerr << "WARNING: using scan3 instead of scan2" << endl; */
  scan3(*labeledWater, nrows, ncols, labelElevType(), det);
  
  /*  NODATA_FIX 
	  assert(LABEL_BOUNDARY < LABEL_NODATA);
	  boundaryType bt(-1, -1, ELEVATION_MIN, LABEL_BOUNDARY, LABEL_NODATA);
	  AMI_err ae = boundaryStr->write_item(bt);
	  assert(ae == AMI_ERROR_NO_ERROR);
  */
}
  

/* ********************************************************************** */

int
compressedWaterWindowBaseType::computeDelta(waterWindowBaseType *center,
					    int index,
					    waterWindowBaseType *p) const{
  if(center->el != p->el) {
    assert(p->depth == 1 || center->el > p->el);
    return 0;
  }
  if(index > 7) return 0;		/* we store our depth elsewhere */
  
  int d = p->depth - center->depth + 1;
  assert(d >= 0);
#ifndef NDEBUG
  if(d>2) {
	cerr << "whoops - assertion failure" << endl;
	cerr << "center = " << *center << endl;
	cerr << "p = " << *p << endl;
	cerr << "this = " << *this << endl;
  }
#endif
  assert(d <= 2);
  return d<<(2*index);
}

compressedWaterWindowBaseType::compressedWaterWindowBaseType(dimension_type gi,
							     dimension_type gj,
							     waterWindowBaseType *a, 
							     waterWindowBaseType *b, 
							     waterWindowBaseType *c) 
  : ijBaseType(gi, gj) {
  
  for(int i=0; i<3; i++) {
    el[i] = a[i].el;
    el[i+3] = b[i].el;
    el[i+6] = c[i].el;
  }
  
  for(int i=0; i<3; i++) {
    const direction_type mask_a[] = {2, 4, 8};
    const direction_type mask_b[] = {1, 0, 16};
    const direction_type mask_c[] = {128, 64, 32};
    points.setBit(i, a[i].dir & mask_a[i]);
    points.setBit(norm(i+3), b[i].dir & mask_b[i]);
    points.setBit(i+5, c[i].dir & mask_c[i]);
  }
  dir = b[1].dir;
  depth = b[1].depth;
  depth_delta = 0;
  
  /* nodata is not processed. */
  if(is_nodata(b[1].el)) {
    return;
  }
  
  for(int i=0; i<3; i++) {
    depth_delta |= computeDelta(b+1, norm(-1,i-1), a+i);
    depth_delta |= computeDelta(b+1, norm(0,i-1), b+i);
    depth_delta |= computeDelta(b+1, norm(1,i-1), c+i);
  }
}

fillPriority
compressedWaterWindowBaseType::getPriority() const {
  return fillPriority(getElevation(), getDepth(), i, j);
}


bfs_depth_type
compressedWaterWindowBaseType::getDepth(int k) const {
  if(getElevation() != getElevation(k)) return DEPTH_INITIAL;
  return depth + ((depth_delta >> (norm(k)*2)) & 0x3) - 1;
}


void 
compressedWaterWindowBaseType::sanityCheck() {
  assert(i >= -1);
  assert(j >= -1);
  assert(depth >= DEPTH_INITIAL);
}
  

ostream&
operator<<(ostream& s, const compressedWaterWindowBaseType &p) {
  return s << "[compressedWaterWindowBaseType " 
	   << p.i << "," << p.j 
	   << " " << directionSymbol(p.getDirection())
	   << " e=" << p.getElevation() 
	   << " d =" << p.getDepth() << "]";
}

/* ********************************************************************** */

labelElevType 
compressedWaterWindowType::getCenter() const {
  return labelElevType(i, j, getElevation(), label);
}

void 
compressedWaterWindowType::sanityCheck() {
  assert(label >= LABEL_UNDEF);
  compressedWaterWindowBaseType::sanityCheck();
}


ostream&
operator<<(ostream& s, const compressedWaterWindowType &p) {
  return s << "[compressedWaterWindowType " 
	   << p.i << "," <<  p.j 
	   << "  " << directionSymbol(p.getDirection())
	   << " e=" << p.getElevation()
	   << " d=" << p.getDepth() 
	   << " l=" << p.label;
}


