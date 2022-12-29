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


#ifndef PLATEAU_H
#define PLATEAU_H

#include <assert.h>

#include <grass/iostream/ami.h>

#include "types.h"
#include "direction.h"
#include "genericWindow.h"

/* ---------------------------------------------------------------------- */
class plateauType : public ijBaseType {
public: /* struct, so members public */
  cclabel_type cclabel;
  direction_type dir;
  bool valid;
public:
  plateauType(dimension_type gi, dimension_type gj, direction_type gdir, 
	      cclabel_type gcclabel=LABEL_UNDEF) :
    ijBaseType(gi,gj), cclabel(gcclabel), dir(gdir), valid(true) {};
 
  plateauType() : valid(false) {};
  
  ~plateauType() {}
  
  void invalidate() { valid = false; }
  
  static char *printLabel(const plateauType &p) {
    static char buf[8];
    sprintf(buf, CCLABEL_FMT, p.cclabel);
    return buf;
  }

  friend ostream& operator << (ostream& s, const plateauType &p) {
    if(p.valid) {
      return s << "[" << (ijBaseType)p
	       << ": dir=" << p.dir 
	       << "; lbl=" << p.cclabel << "]";
    } else {
      return s << "[invalid]";
    }
  }
};

class ijCmpPlateauType {
public:
  static int compare(const plateauType &a, const plateauType &b) {
    return ijBaseType::compare(a, b);
  }
};

class labelCmpPlateauType {
public:
  static int compare(const plateauType &a, const plateauType &b) {
	if(a.cclabel < b.cclabel) return -1;
	if(a.cclabel > b.cclabel) return 1;
	return 0;
  }
};





/* ********************************************************************** */
class plateauStats {
public:
  dimension_type iMin, iMax, jMin, jMax;
  long size;
  cclabel_type label;
  bool hasSpill;
public:
 
  plateauStats() : label(LABEL_UNDEF) {}
  
  plateauStats(cclabel_type l) : 
    iMin(dimension_type_max), iMax(0), 
    jMin(dimension_type_max), jMax(0),
    size(0), label(l), hasSpill(false) {};
  
  void add(plateauType &pt) {
    assert(pt.cclabel == label);
    if(pt.i < iMin) iMin = pt.i;
    if(pt.i > iMax) iMax = pt.i;
    if(pt.j < jMin) jMin = pt.j;
    if(pt.j > jMax) jMax = pt.j;
    if(pt.dir > 0) hasSpill = true;
    size++;
  }

  SHALLOW_OP_EQ(plateauStats);
  
  friend ostream& operator << (ostream& s, const plateauStats &p) {
    return s << "[" << p.label << ": "
	     << "(" << p.iMin << "," << p.jMin << ")-"
	     << "(" << p.iMax << "," << p.jMax << "); "
	     << p.size << " " 
	     << (p.hasSpill?"S":".") << "]";
  }

};



/* ********************************************************************** */

AMI_STREAM<plateauType> *
findPlateaus(AMI_STREAM<elevation_type> *elstr,
			 const dimension_type nrows, const dimension_type ncols,
			 const elevation_type nodata_value,
			 AMI_STREAM<ElevationWindow > *winstr,
			 AMI_STREAM<direction_type> *dirStr,
			 AMI_STREAM<plateauStats> *statStr);

#endif

