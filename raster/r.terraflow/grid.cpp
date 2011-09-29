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


#include <string.h>
#include <assert.h>
#include "grid.h"
#include "common.h"

#define GRID_DEBUG if(0)

/* leave a border of 1 cell around */
grid::grid(dimension_type giMin, dimension_type gjMin,
		   dimension_type iMax, dimension_type jMax,
		   long gsize, cclabel_type glabel) : 
  iMin(giMin-1), jMin(gjMin-1), label(glabel), size(gsize) {
  width = jMax - jMin + 2;
  height = iMax - iMin + 2;
  assert(width*height*sizeof(gridElement) < getAvailableMemory());
  data = new gridElement[width*height];
  assert(data);
  memset(data, 0, width*height*sizeof(gridElement));
}



grid::~grid() {
  delete [] data;
}



void
grid::load(AMI_STREAM<plateauType> &str) {
  AMI_err ae;
  plateauType *pt;

  GRID_DEBUG cout << "loading grid" << endl;
  for(int i=0; i<size; i++) {
	ae = str.read_item(&pt);
	assert(ae == AMI_ERROR_NO_ERROR);
	/* cout << *pt << endl; */
	assert(pt->valid);
	assert(pt->cclabel == label);
	dimension_type pti, ptj;
	pti = pt->i - iMin;
	ptj = pt->j - jMin;
	gridElement *datap = data + pti * width + ptj;
	datap->dir = pt->dir;
	datap->depth = DEPTH_INITIAL;			/* initial depth */
	datap->valid = 1;
#ifdef KEEP_COORDS
	datap->i = pt->i;
	datap->j = pt->j;
#endif
	if(datap->dir) {			
	  /* if it has a dir, it's on the boundary */
	  boundaryQueue[0].enqueue(datap);
	}
  }
}

void
grid::save(AMI_STREAM<waterType> &str) {
  GRID_DEBUG cout << "saving grid" << endl;

  for(dimension_type i=1; i<height-1; i++) {
    gridElement *rowp = data + i * width;
    for(dimension_type j=1; j<width-1; j++) {
      gridElement *datap = rowp + j;
      if(datap->valid) {
		/* DONT save the label */
	waterType wt(i+iMin, j+jMin, datap->dir, LABEL_UNDEF, datap->depth);
	AMI_err ae = str.write_item(wt);
	assert(ae == AMI_ERROR_NO_ERROR);
      }
    }
  }
}


void 
grid::print() {
  cout << "    ";
  for(int i=0; i<width; i++) {
    printf("%2d", (jMin + i%10));
  }
  cout << endl;
  for(int j=0; j<height; j++) {
    printf("%3d ", j + iMin);
    for(int i=0; i<width; i++) {
      if(data[i+width*j].valid) {
	cout << " " << directionSymbol(data[i+width*j].dir);
      } else {
	cout << " .";
      }
    }
    cout << endl;
  }
}


gridElement *
grid::getNeighbour(gridElement *datap, int k) {
  switch(k) {
  case 0:
    datap += 1;
	break;
  case 1:
    datap += width + 1;
    break;
  case 2:
    datap += width;
    break;
  case 3:
    datap += width - 1;
    break;
  case 4:
    datap -= 1;
    break;
  case 5:
    datap -= (width + 1);
    break;
  case 6:
    datap -= width;
    break;
  case 7:
    datap -= (width - 1);
    break;
  default:
    assert(0);
    break;
  }
  return datap;
}


direction_type
grid::getDirection(int k) {
  return 1<<((k+4)%8);			/* converse direction */
}




void
grid::assignDirections(int sfdmode) {
  gridElement *datap, *np;
  
#ifdef KEEP_COORDS	
  GRID_DEBUG cout << "points in queue=" << boundaryQueue[0].length() << endl;
  GRID_DEBUG for(int i=0; i<boundaryQueue[0].length(); i++) {
    boundaryQueue[0].peek(i,&datap);
    cout << datap->i << "," << datap->j << endl;
  }
  GRID_DEBUG cout << endl;
#endif
  
  int k1=0, k2=1;
  while(!boundaryQueue[k1].isEmpty()) {
    while(boundaryQueue[k1].dequeue(&datap)) {
      /* should only find dominant if not on edge */
      if(sfdmode && datap->depth > DEPTH_INITIAL) {
	datap->dir = findDominant(datap->dir);
      }
#ifdef KEEP_COORDS
      GRID_DEBUG cout << "(" << datap->i << "," << datap->j <<  ") "
		 << "my direction is " << datap->dir;
#endif
      for(int i=0; i<8; i++) {
	np = getNeighbour(datap, i);
	if(np->valid) {
	  if(!np->dir) {
	    np->depth = datap->depth + 1;
	    boundaryQueue[k2].enqueue(np);
#ifdef KEEP_COORDS
	    GRID_DEBUG cout << " pushing "  << "(" << np->i << "," << np->j <<  ")";
#endif
	  }
	  if(np->depth == datap->depth + 1) { /* can only update ifin othr list */
		np->dir |= getDirection(i);	/*  neighbor points to us */
		/* if(!np->dir) np->dir |= getDirection(i); */
	  }
	}
      }
      GRID_DEBUG cout << endl;
    }
    k1 ^= 1;
    k2 ^= 1;
  }
}


