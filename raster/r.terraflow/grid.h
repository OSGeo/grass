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


#ifndef GRID_H
#define GRID_H

#include <grass/iostream/ami.h>
#include "types.h"
#include "plateau.h"
#include "water.h"

/* define to keep i,j values (used in printing) */
/* #define KEEP_COORDS */


struct gridElement {
  direction_type dir;
  char valid;			 /* whether part of plateau in grid */
  bfs_depth_type depth;
#ifdef KEEP_COORDS
  dimension_type i,j;
#endif
};



class grid {
private:
  gridElement *data;
  dimension_type iMin, jMin;
  dimension_type width, height;
  cclabel_type label;
  long size;
  queue<gridElement *> boundaryQueue[2];
public:
  grid(dimension_type iMin, dimension_type jMin,
	   dimension_type iMax, dimension_type jMax,
	   long size,
	   cclabel_type label);
  ~grid();
  void load(AMI_STREAM<plateauType> &str);
  void save(AMI_STREAM<waterType> &str);
  void print();
  void assignDirections(int sfdmode); /* single flow directions */
  gridElement *getNeighbour(gridElement *datap, int k);
  direction_type getDirection(int);
};


#endif

