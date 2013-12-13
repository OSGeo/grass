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


#ifndef NODATA_H
#define NODATA_H

#include <assert.h>

#include <grass/iostream/ami.h>
#include "types.h"
#include "option.h"



/* somewhat of a hack. should read the GRASS nodata value instead
   (which normally is MAX_INT), but, is it worth it? */
#define TERRAFLOW_INTERNAL_NODATA_VALUE -9999



int is_nodata(elevation_type el);
int is_nodata(int x);
int is_nodata(float x);
int is_void(elevation_type el);


class nodataType : public ijBaseType {
public: /* struct, so members public */
  cclabel_type label;
  bool valid;
  static elevation_type ELEVATION_BOUNDARY; /* means this cell is on	
					     * the grid boundary, 
					     * directly, or via
					     * connect nodata cells */

  static elevation_type ELEVATION_NODATA; /* means we have no data for	
					   * this cell */
public:
  nodataType(dimension_type gi, dimension_type gj, cclabel_type glab) :
	ijBaseType(gi,gj), label(glab), valid(true) {};
  nodataType() : valid(false) {};
  void invalidate() { valid = false; }
  elevation_type getElevation() {
	return (label == LABEL_BOUNDARY ? ELEVATION_BOUNDARY : ELEVATION_NODATA);
  }

  static char *printLabel(const nodataType &p) {
	static char buf[8];
	sprintf(buf, CCLABEL_FMT, p.label);
	return buf;
  }

  static void init(elevation_type nodata) {
	/*  somewhat of a hack... */
	ELEVATION_NODATA = nodata;
	ELEVATION_BOUNDARY = nodata+1;
	/* ELEVATION_BOUNDARY = ELEVATION_MIN; */
	/* assert(ELEVATION_NODATA != ELEVATION_MIN); */
  }
  
  /* LAURA: i added this polymorph because Terraflow has now a FIXED
     internal value for nodata; it does not read GRASS nodata value */
  static void init() {
	/* somewhat of a hack... */
	ELEVATION_NODATA = TERRAFLOW_INTERNAL_NODATA_VALUE;
	ELEVATION_BOUNDARY = TERRAFLOW_INTERNAL_NODATA_VALUE + 1;
	/* ELEVATION_BOUNDARY = ELEVATION_MIN; */
	/* assert(ELEVATION_NODATA != ELEVATION_MIN); */
  }
  
  friend ostream& operator << (ostream& s, const nodataType &p) {
	if(p.valid) {
	  return s << "[" << p.i << "," << p.j 
			   << "; lbl=" << p.label << "]";
	} else {
	  return s << "[invalid]";
	}
  }
  
};

class nodataType2elevation_type {
public:
  elevation_type operator()(nodataType n) { return n.getElevation(); }
  elevation_type operator()(elevation_type n) { return n; }
};

class labelCmpNodataType {
public:
  static int compare(const nodataType &a, const nodataType &b) {
	if(a.label < b.label) return -1;
	if(a.label > b.label) return 1;
	return 0;
  }
};

class ijCmpNodataType {
public:
  static int compare(const nodataType &a, const nodataType &b) {
	return ijBaseType::compare(a, b);
  }
};


AMI_STREAM<elevation_type> *
classifyNodata(AMI_STREAM<elevation_type> *elstr);
			   

#endif
