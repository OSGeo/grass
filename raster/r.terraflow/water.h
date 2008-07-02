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


#ifndef WATER_H
#define WATER_H

/* watershed analysis related structures */

#include "types.h"
#include "plateau.h"
#include "nodata.h"
#include "genericWindow.h"

char directionSymbol(direction_type dir);

class labelElevTypePrintLabel;

/* ---------------------------------------------------------------------- */

class labelElevType : public ijBaseType {
  friend class labelElevTypePrintLabel;
 protected:
  elevation_type el;
  cclabel_type label;
 public:
  labelElevType() : label(LABEL_UNDEF) {};
  
  labelElevType(dimension_type gi,
		dimension_type gj,
		elevation_type gel,
		cclabel_type glabel) :
    ijBaseType(gi, gj), el(gel), label(glabel) {};
  
  cclabel_type getLabel() const { return label; };
  
  elevation_type getElevation() const { return el; };
  
  friend ostream& operator << (ostream& s, const labelElevType &p);

  static char *printLabel(const labelElevType &p);

  friend class printLabel;
};


class ijCmpLabelElevType {
public:
  static int compare(const labelElevType &a, const labelElevType &b) {
	return ijBaseType::compare(a, b);
  }
};


class labelCmpLabelElevType {
public:
  static int compare(const labelElevType &a, const labelElevType &b) {
	return a.getLabel() - b.getLabel();
	/* return (a.getLabel() == b.getLabel() ? 0
	   : (a.getLabel() < b.getLabel() ? -1 : 1)); */
  }
};


class labelElevTypePrintLabel {
 public:
  cclabel_type  operator()(const labelElevType &p) {
    return p.label;
  }
};

/* ---------------------------------------------------------------------- */
class waterCmpBoundaryType;
class elevCmpBoundaryType;
class boundaryCmpBoundaryType;
class ijCmpBoundaryType;

class boundaryType : public labelElevType {
  friend class waterCmpBoundaryType;
  friend class elevCmpBoundaryType;
  friend class boundaryCmpBoundaryType;
protected:
  cclabel_type label2;
public:
  boundaryType() : label2(LABEL_UNDEF) {};
  
  boundaryType(dimension_type gi,
	       dimension_type gj,
	       elevation_type gel,
	       cclabel_type glabel1,
	       cclabel_type glabel2) :
    labelElevType(gi, gj, gel, glabel1), label2(glabel2) {
    assert(glabel1 < glabel2);
  };

  boundaryType(labelElevType let, cclabel_type glabel2) : labelElevType(let) {
    if(let.getLabel() < glabel2) {
      label2 = glabel2;
    } else {
      label2 = label;
      label = glabel2;
    }
  };

  boundaryType(labelElevType let, elevation_type gel, cclabel_type glabel2)
	: labelElevType(let) {
	el = gel;
	if(let.getLabel() < glabel2) {
	  label2 = glabel2;
	} else {
	  label2 = label;
	  label = glabel2;
	}
  };
  bool isValid() const { return label2 != LABEL_UNDEF; }
  cclabel_type getLabel1() const { return getLabel(); }
  cclabel_type getLabel2() const { return label2; }
  friend ostream& operator << (ostream& s, const boundaryType &p);
  static char *print(const boundaryType &p);
};

class ijCmpBoundaryType {
public:
  static int compare(const boundaryType &a, const boundaryType &b) {
	return ijBaseType::compare(a, b);
  }
}
;
class waterCmpBoundaryType {
public:
  static int compare(const boundaryType &a, const boundaryType &b) {
	if(a.label < b.label) return -1;
	if(a.label > b.label) return 1;

	if(a.label2 < b.label2) return -1;
	if(a.label2 > b.label2) return 1;

	if(a.el < b.el) return -1;
	if(a.el > b.el) return 1;

	return 0;
  }
};

class elevCmpBoundaryType {
public:
  static int compare(const boundaryType &a, const boundaryType &b) {
	if(a.el < b.el) return -1;
	if(a.el > b.el) return 1;

	return 0;
  }
};

class boundaryCmpBoundaryType {
public:
  static int compare(const boundaryType &a, const boundaryType &b) {
	if(a.label < b.label) return -1;
	if(a.label > b.label) return 1;

	if(a.label2 < b.label2) return -1;
	if(a.label2 > b.label2) return 1;

	return 0;
  }
};

/* ---------------------------------------------------------------------- */

class fillPriority : public ijBaseType {
protected:
  elevation_type el;
  bfs_depth_type depth;
public:
  fillPriority() : ijBaseType(-1,-1), el(-1), depth(DEPTH_INITIAL) {};
  fillPriority(elevation_type gel,
			   bfs_depth_type gdepth,
			   dimension_type gi,
			   dimension_type gj) :
    ijBaseType(gi, gj), el(gel), depth(gdepth) {}

  friend int operator<(const fillPriority &a, const fillPriority &b);
  friend int operator<=(const fillPriority &a, const fillPriority &b);
  friend int operator>(const fillPriority &a, const fillPriority &b);
  friend int operator==(const fillPriority &a, const fillPriority &b);
  friend int operator!=(const fillPriority &a, const fillPriority &b);
  friend ostream& operator << (ostream& s, const fillPriority &p);
  static int compare(const fillPriority &a, const fillPriority &b);
  static int qscompare(const void *a, const void *b);
};

/* ---------------------------------------------------------------------- */

class fillPLabel : public fillPriority {
  cclabel_type label;
public:
  fillPLabel() : label(LABEL_UNDEF) {};
  fillPLabel(const fillPriority &gpriority, const cclabel_type glabel) :
	fillPriority(gpriority), label(glabel) {}
  fillPriority getPriority() const { return (fillPriority)(*this); }
  cclabel_type getLabel() const { return label; }
  /* static char *print(const fillPLabel &p); */
  boundaryType getBoundary(cclabel_type label2) {
	if(label < label2) {
	  return boundaryType(i, j, el, label, label2);
	} else {
	  return boundaryType(i, j, el, label2, label);
	}
  }
  friend ostream& operator << (ostream& s, const fillPLabel &p) {
	return s << "[" << (fillPriority)p << ", " << p.label << "]";
  }
};


/* ---------------------------------------------------------------------- */


class waterWindowBaseType {
public:
  elevation_type el;
  direction_type dir;
  bfs_depth_type depth;
  waterWindowBaseType() : el(nodataType::ELEVATION_NODATA), dir(0), 
	depth(DEPTH_INITIAL) {};
  waterWindowBaseType(elevation_type gel, 
					  direction_type gdir, bfs_depth_type gdepth) :
    el(gel), dir(gdir), depth(gdepth) {};
  friend int 
  operator==(const waterWindowBaseType &a, const waterWindowBaseType &b) {
    return (a.el == b.el) && (a.dir == b.dir) && (a.depth == b.depth);
  }
  
  friend ostream& operator << (ostream& s, const waterWindowBaseType &p) {
	return s << "[" 
			 << "el=" << p.el << ", "
			 << "dir=" << p.dir << ", "
			 << "depth=" << p.depth << "]";
  }
};

/* ---------------------------------------------------------------------- */


class waterType;
class waterGridType;
class waterType : public ijBaseType {
  friend int ijCmp_waterType(const waterType &a, const waterType &b);
  friend class waterGridType;
protected:
  direction_type dir;
  bfs_depth_type depth;
  cclabel_type label;
public:
  waterType() : label(LABEL_UNDEF) {};		      	/* needed to sort */
  waterType(dimension_type gi,
	    dimension_type gj,
	    direction_type gdir,
	    cclabel_type glabel=LABEL_UNDEF,
	    bfs_depth_type gdepth=DEPTH_INITIAL) :
    ijBaseType(gi, gj), dir(gdir), depth(gdepth), label(glabel) {
  }
  waterType(plateauType &data) : 
    ijBaseType(data.i, data.j),
    dir(data.dir), depth(DEPTH_INITIAL), label(data.cclabel) {
  };
  direction_type getDirection() const { return dir; }
  bfs_depth_type getDepth() const { return depth; }
  cclabel_type getLabel() const { return label; }
  static char *printLabel(const waterType &p);
  friend ostream& operator << (ostream& s, const waterType &p) {
    return s << "[waterType" << (ijBaseType)p
	     << ", dir=" << p.dir
	     << ", bfs=" << p.depth 
	     << ", lab=" << p.label << "]";
  }
};

class ijCmpWaterType {
public:
  static int compare(const waterType &a, const waterType &b) {
	return ijBaseType::compare(a, b);
  }
};

#if(0)
class waterType2direction_type {
public:
  direction_type operator()(waterType p) { return p.getDirection(); }
  direction_type operator()(direction_type p) { return p; }
};
#endif

/* ---------------------------------------------------------------------- */

class waterGridType : public waterWindowBaseType {
protected:
  cclabel_type label;
public:
  waterGridType() : label(LABEL_UNDEF) {};
  waterGridType(elevation_type gel, 
		direction_type gdir=DIRECTION_UNDEF,
		cclabel_type glabel=LABEL_UNDEF,
		bfs_depth_type gdepth=DEPTH_INITIAL) :
    waterWindowBaseType(gel, gdir, gdepth), label(glabel) {
  }
  waterGridType(elevation_type gel, waterType w) :
    waterWindowBaseType(gel, w.dir, w.depth), label(w.label) {}
  
  cclabel_type getLabel() const { return label; };
  void setLabel(cclabel_type lbl) { label=lbl; };
  friend ostream& operator << (ostream& s, const waterGridType &p) {
    return s << directionSymbol(p.dir);
  }
};


/* ---------------------------------------------------------------------- */

class packed8bit {
protected:
  unsigned char value;
public:  
  packed8bit() : value(0) {};
  void setBit(int k, int v=1) { value = (int) value | ((v?1:0)<<k); };
  void resetBit(int k) { value &= ~(0x1<<k); };
  int getBit(int k) const { return (value >> k) & 1; };
};

static int linear(int i, int j) {
  assert(i>=-1 && i<=1 && j>=-1 && j<=1);
  return ((i+1)*3+(j+1));  
}

static int norm(int k) {
  return (k<4 ? k : (k>4?k-1:8)); 
}

static int norm(int i, int j) { 
  return norm(linear(i,j)); 
}




class compressedWaterWindowBaseType : public ijBaseType {
protected:						/* 4 */
  bfs_depth_type depth;			/* 4 */
  elevation_type el[9];			/* 18 */
  direction_type dir;			/* 2 */
  /* we only need the depth if the elevation is the same. if so, the
   variation is bounded by +/-1. if a cell receives a label from a
   lower elevation (ie the center is lower than the cell), then the
   recipients depth must be 1 (it's on the edge). if the center is
   higher, then we have no interest in the cell. */
  unsigned short depth_delta;	/* 2 */
  packed8bit points;			/* 1 whether neighbor points to me */
  /* cells are numbered 0,1,2/3,4,5/6,7,8 */
  /* bits are 0,1,2,3,5,6,7,8 (skip 4) */
public:
  compressedWaterWindowBaseType() : depth(DEPTH_INITIAL),
				    dir(0) {
    for(int i=0; i<9; i++) { el[i] = nodataType::ELEVATION_NODATA; }
  };
  compressedWaterWindowBaseType(dimension_type gi,
				dimension_type gj,
				waterWindowBaseType *a, 
				waterWindowBaseType *b, 
				waterWindowBaseType *c);
  fillPriority getPriority() const;
  elevation_type getElevation(int k=4) const { return el[k]; };
  elevation_type getElevation(int i,int j) const{return el[linear(i,j)];};
  direction_type getDirection() const { return dir; };
  int drainsFrom(int i, int j) const { 
	/* whether we drain water from cell i,j */
	return points.getBit(norm(i, j));
  };
  bfs_depth_type getDepth() const { return depth; };
  bfs_depth_type getDepth(int k) const;
  bfs_depth_type getDepth(int i, int j) const { return getDepth(norm(i,j)); };
  void sanityCheck();
  
  friend ostream& operator<<(ostream& s, 
			     const compressedWaterWindowBaseType &p);
private:
  int computeDelta(waterWindowBaseType *center,
		   int index, waterWindowBaseType *p) const;
};



class compressedWaterWindowType : public compressedWaterWindowBaseType {
protected:
  cclabel_type label;
public:
  compressedWaterWindowType() : compressedWaterWindowBaseType(),
				label(LABEL_UNDEF) {};
  compressedWaterWindowType(dimension_type gi,
			    dimension_type gj,
			    cclabel_type glabel,
			    waterWindowBaseType *a, 
			    waterWindowBaseType *b, 
			    waterWindowBaseType *c) 
    : compressedWaterWindowBaseType(gi, gj, a, b, c), 
	label(glabel) {
    /* if nodata, labels are either depression labels, or undefined or
	   boundary (a pseudo depression). */
    assert(!(is_nodata(getElevation())) ||
	   (label==LABEL_BOUNDARY || label==LABEL_UNDEF));
  }
  cclabel_type getLabel() const { return label; };
  void setLabel(cclabel_type lbl) { label=lbl; };
  labelElevType getCenter() const;
  
  void sanityCheck();
  
  friend ostream& operator<<(ostream& s, const compressedWaterWindowType &p);
};

typedef compressedWaterWindowType waterWindowType;



class ijCmpWaterWindowType {
public:
  static int compare(const waterWindowType &a, const waterWindowType &b) {
	return ijBaseType::compare(a, b);
  }
};

class priorityCmpWaterWindowType {
public:
  static int compare(const waterWindowType &a, const waterWindowType &b) {
    return fillPriority::compare(a.getPriority(), b.getPriority());
  }
};



/* ********************************************************************** */

void 
createWaterWindows(AMI_STREAM<waterGridType> *mergedWaterStr, 
				   const dimension_type nrows, const dimension_type ncols,
				   AMI_STREAM<waterWindowType> *waterWindows);

/* ********************************************************************** */

void
generateWatersheds(AMI_STREAM<waterWindowType> **waterWindows,
				   const dimension_type nrows, const dimension_type ncols,
				   AMI_STREAM<labelElevType> *labeledWater, 
				   AMI_STREAM<boundaryType> *boundaryStr);

void
findBoundaries(AMI_STREAM<labelElevType> *labeledWater,
			   const dimension_type nrows, const dimension_type ncols,
			   AMI_STREAM<boundaryType> *boundaryStr);

/* ********************************************************************** */

#endif

