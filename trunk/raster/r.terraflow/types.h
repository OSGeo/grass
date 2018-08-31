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

#ifndef _types_H
#define _types_H

#include <limits.h>
#include <iostream>


/* input parameters type */
/* ------------------------------------------------------------ */

typedef short dimension_type; /* represent dimension of the grid */
static const dimension_type dimension_type_max=SHRT_MAX;

typedef short direction_type;  /* represent the direction of a cell */
static const direction_type DIRECTION_UNDEF=-1;

/* one of these options must be defined at compile time */
/* #define ELEV_SHORT */
/* #define ELEV_FLOAT */

#if (!defined ELEV_SHORT && !defined ELEV_FLOAT)
#error Must define ELEVATION TYPE 
#endif

#ifdef ELEV_SHORT
typedef short elevation_type;  /* represent the elevation of a cell */
static const elevation_type elevation_type_max = SHRT_MAX;
#endif
#ifdef ELEV_FLOAT
typedef  float elevation_type;  /* represent the elevation of a cell */
static const elevation_type elevation_type_max = 1e+15;
#endif
/* static const elevation_type ELEVATION_UNDEF=SHRT_MAX;
   static const elevation_type ELEVATION_MIN=SHRT_MIN;
   also see nodata.H for ELEVATION_BOUNDARY and ELEVATION_NODATA
*/


/* represent the topological rank of a cell */
typedef int toporank_type; 



/* output parameter types */
/* ------------------------------------------------------------ */
typedef float flowaccumulation_type;    
static const flowaccumulation_type MAX_ACCU = 1e+15;
typedef float tci_type;       




typedef int cclabel_type;
#define CCLABEL_FMT "%3d"
/* the following are not arbitrary. LABEL_UNDEF should be the
 * most-negative value */
static const cclabel_type LABEL_UNDEF=-1;
static const cclabel_type LABEL_BOUNDARY=0;
static const cclabel_type LABEL_NODATA=1;
static const cclabel_type LABEL_START=1; /* the next label will be used */

typedef int bfs_depth_type;
static const bfs_depth_type DEPTH_INITIAL=1;

#define IS_BOUNDARY(i,j,nr,nc) (((i)==0) || ((i)==((nr)-1)) || \
							  ((j)==0) || ((j)==((nc)-1)))


/* ---------------------------------------------------------------------- */

class labelFactory {
protected:
  static cclabel_type label;
public:
  static cclabel_type getNewLabel() { return ++label; }
  static cclabel_type getCurrentLabel() { return label; }
  static const cclabel_type getLabelInit() { 
	return cclabel_type(LABEL_START);
  }
  static const cclabel_type getLabelCount() {
	return label+1;
  }
  static void setLabelCount(int n) {
	label = n-1;
  }
  static void reset() {	label = getLabelInit(); }
};

/* ---------------------------------------------------------------------- */

class ijBaseType {
public:
  dimension_type i,j;
  ijBaseType() : i(-1), j(-1) {};
  ijBaseType(dimension_type gi, dimension_type gj) : i(gi), j(gj) {};
  friend int operator==(const ijBaseType &a, const ijBaseType &b) {
	return (compare(a,b) == 0);
  }
  friend int operator!=(const ijBaseType &a, const ijBaseType &b) {
	return (compare(a,b) != 0);
  }
  friend std::ostream& operator << (std::ostream& s, const ijBaseType &p);
  static int compare(const ijBaseType &a, const ijBaseType &b);
};

#define SHALLOW_OP_EQ(_cls)			\
  friend int					\
  operator==(const _cls &a, const _cls &b) {	\
    const int n = sizeof(_cls);			\
    const char *ap = (const char *)&a;		\
    const char *bp = (const char *)&b;		\
    for(int i=0; i<n; i++) {			\
      if(ap[i] != bp[i]) return 0;		\
    }						\
    return 1;					\
  }



#endif

