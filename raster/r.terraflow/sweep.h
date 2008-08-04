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


#ifndef _SWEEP_H
#define _SWEEP_H



#include <grass/iostream/ami.h>

#include "option.h"
#include "types.h"
#include "weightWindow.h"



class sweepOutput;
class gridPosition;
class flowStructure;
class flowValue;
class flowPriority;


/* ---------------------------------------------------------------------- */
class sweepOutput {
 public:
  dimension_type i,j;  /* position in the grid */
  flowaccumulation_type   accu;    /* count  */
#ifdef OUTPUT_TCI
  tci_type     tci;      /* tci */
#endif

 public:
  sweepOutput();
  
  /* computes output parameters given a flow value */
  void compute(elevation_type elev, dimension_type i, dimension_type j, 
	       const flowValue &flow, const weightWindow &wght, 
	       const  elevation_type nodata);
  
  friend ostream& operator << (ostream& s, const sweepOutput &x) {
    return s << "[("  << x.i << ',' << x.j << "):"  
      //<< form(" accu=%7.3f", x.accu)
	     << " accu=" << x.accu
#ifdef OUTPUT_TCI
      //<< form(", tci=%3.1f", x.tci)
	     << ", tci=" << x.tci
#endif
	     << "]";
  }
  
};



/* ---------------------------------------------------------------------- */
class ijCmpSweepOutput {
public: 
  static int compare(const sweepOutput &a, const sweepOutput &b) {
    if(a.i < b.i) return -1;
    if(a.i > b.i) return 1;
    
    if(a.j < b.j) return -1;
    if(a.j > b.j) return 1;
    
    return 0;   
  }
};


/***************************************************************/
class printAccumulation {
 public:
  flowaccumulation_type operator()(const sweepOutput &p) {
    return p.accu;
  }
};



/***************************************************************/
class printAccumulationAscii {
public:
  char *operator()(const sweepOutput &p) {
    static char buf[20];
    sprintf(buf, "%7.3f", p.accu);
    return buf;
  }
};

/***************************************************************/
#ifdef OUTPUT_TCI
class printTci {
public:
  tci_type operator()(const sweepOutput &p) {
    return p.tci;
  }
};

class printTciAscii {
public:
  char *operator()(const sweepOutput &p) {
    static char buf[20];
    sprintf(buf, "%7.3f", p.tci);
    return buf;
  }
};
#endif


/***************************************************************/
/***************************************************************/

/* class 'grid_position' defines position in the grid */

/***************************************************************/
/***************************************************************/

class gridPosition {

public:
  dimension_type i,j;
  
public:
  friend int operator == (const gridPosition &x, 
			  const gridPosition &y) {
    return ((x.i == y.i) && (x.j == y.j));
  }
  friend int operator != (const gridPosition &x, 
			  const gridPosition &y) {
    return (!(x == y));
  }
  friend int operator < (const gridPosition &x, 
			 const gridPosition &y) {
    return ((x.i < y.i) || ((x.i == y.i) && (x.j < y.j)));
  }
  friend int operator <= (const gridPosition &x, 
			  const gridPosition &y) {
    return ((x < y) || (x == y));
  }
  friend int operator > (const gridPosition &x, 
			 const gridPosition &y) {
    return ((x.i > y.i) || ((x.i == y.i) && (x.j > y.j)));
  }
  friend int operator >= (const gridPosition &x, 
			  const gridPosition &y) {
    return ((x > y) || (x == y));
  }
};



/*************************************************************/
class flowPriority {
public:
  elevation_type h;
  toporank_type toporank;
  /* points at same heights are processed in increasing order of their
     topological rank; overall, this gives topological order and
     guarantees that flow is never puhsed backwards. Note: of course,
     this is a way of waving hands on topological sorting.  */
  dimension_type i,j;
  
public:
  flowPriority(elevation_type a=0, toporank_type b=0, 
	       dimension_type c=0, dimension_type d=0): 
    h(a), toporank(b), i(c), j(d) {}
  
  flowPriority(const flowPriority &p):
    h(p.h), toporank(p.toporank), i(p.i), j(p.j) {}
  
  ~flowPriority() {}
  
  elevation_type field1() const {
    return h;
  }
  
  dimension_type coord1() const {
    return i;
  }
  dimension_type coord2() const {
    return j;
  }
  void set (elevation_type g_h, toporank_type g_a, 
	    dimension_type g_i, dimension_type g_j) {
    h = g_h;
    toporank = g_a;
    i = g_i;
    j = g_j;
  }

  static int compare (const flowPriority &a, const flowPriority &b) {
    if (a.h > b.h) return -1;
    if (a.h < b.h) return 1;

    if (a.toporank < b.toporank) return -1;
    if (a.toporank > b.toporank) return 1;

    if (a.i < b.i) return -1;
    if (a.i > b.i) return 1;

    if (a.j < b.j) return -1;
    if (a.j > b.j) return 1;

    return 0;
  } 

  friend int operator < (const flowPriority &p1, 
			 const flowPriority &p2) {
    if (p1.h > p2.h) return 1;
    if ((p1.h == p2.h) && (p1.toporank < p2.toporank)) return 1;
    if ((p1.h == p2.h) && (p1.toporank == p2.toporank) 
	&& (p1.i < p2.i)) return 1;
    if ((p1.h == p2.h) && (p1.toporank == p2.toporank) 
	&& (p1.i == p2.i) && (p1.j < p2.j)) return 1;
    return 0;
  }
  
  friend int operator <= (const flowPriority &p1, 
			  const flowPriority &p2) {
    return ((p1 < p2) || (p1 == p2));
  }
  
  friend int operator > (const flowPriority &p1, 
			 const flowPriority &p2) {
    /* return !(p1 < p2); */
    if (p1.h < p2.h) return 1;
    if ((p1.h == p2.h) && (p1.toporank > p2.toporank)) return 1;
    if ((p1.h == p2.h) && (p1.toporank ==  p2.toporank) 
	&& (p1.i > p2.i)) return 1;
    if ((p1.h == p2.h) && (p1.toporank == p2.toporank)
	&& (p1.i == p2.i) && (p1.j > p2.j)) return 1;
    return 0;
  }
  
  friend int operator >= (const flowPriority &p1, 
			  const flowPriority &p2) {
    return ((p1 > p2) || (p1 == p2));
  }
  
  friend ostream& operator<<(ostream& s, const flowPriority &p) {
    return s << "(" << p.h << "," << p.toporank << ","
	     << p.i << "," << p.j << ")";
  } 
  friend istream& operator>>(istream& s, flowPriority &p) {
    return s >>  p.h >> p.toporank >> p.i >> p.j;
  } 
  
  friend bool operator==(const flowPriority &p1, 
			 const flowPriority &p2) {
    return ((p1.h == p2.h) && (p1.toporank == p2.toporank) 
	    && (p1.i == p2.i) && (p1.j == p2.j));
  }
  friend bool operator!=(const flowPriority &p1, 
			 const flowPriority &p2) {
    return (!(p1 == p2));
  }
  
};


/***************************************************************/
/***************************************************************/
/* class 'sweepItem' defines the structure of an element in the
   sweeping stream; it incorporates all info necessary during
   sweeping. */

/***************************************************************/
/***************************************************************/

#define sweepItem  sweepItemBaseType<toporank_type>


template <class T>
class sweepItemBaseType {
public:
  dimension_type i,j;  
  direction_type dir;    /* precomputed direction */
  genericWindow<elevation_type> elevwin;  /* elevation window */
  genericWindow<T> toporwin;   /* topological_rank window */
  
  
public:
  
  /***************************************************************/
  sweepItemBaseType(): i(0), j(0), dir(0), elevwin(), toporwin() {}
  
  /***************************************************************/
  sweepItemBaseType(const dimension_type &gi, const dimension_type &gj, 
		    const direction_type  &gdir, 
		    elevation_type* a1, 
		    elevation_type* b1, 
		    elevation_type* c1, 
		    T* a2,
		    T* b2,
		    T* c2):  
    i(gi), j(gj), dir(gdir), elevwin(a1,b1,c1), toporwin(a2,b2,c2) {}


  /***************************************************************/
  ~sweepItemBaseType() {}
  
  /***************************************************************/
  /* return the elevation window */
  genericWindow<elevation_type> getElevWindow() const {
    return elevwin;
  }
  
  /***************************************************************/
  /* return the elevation window */
  genericWindow<T> getTopoRankWindow() const {
    return toporwin;
  }
  
  /***************************************************************/
  /* return coordinates */
  dimension_type getI() const {
    return i;
  }
  
  /***************************************************************/
  dimension_type getJ() const {
    return j;
  }
  
  /***************************************************************/
  /* return the elevation of the item */
  elevation_type getElev() const {
    return elevwin.get();
  }
  
  /***************************************************************/
  /* return elevation of one of neighbours: di,dj in {-1,0,1} */
  elevation_type getElev(short di, short dj) const {
    return elevwin.get(di,dj);
  }
  
  /***************************************************************/
  /* return elevation of one of neighbours: index in {0..8} */
  elevation_type getElev(unsigned short index) const {
    return elevwin.get(index);
  }

  /***************************************************************/
  direction_type getDir() const {
    return dir;
  }
  
  
  /***************************************************************/
  /* return the toporank (arcinfo accumulation) of the item */
  T getTopoRank() const {
    return toporwin.get();
  }

  /***************************************************************/
  /* return the topoRank (arcinfo accumulation) of one of neighbours:
     di,dj in {-1,0,1} */
  T getTopoRank(short di, short dj) const { 
    return toporwin.get(di,dj); 
  }
  
  /***************************************************************/
  /* return the topoRank (arcinfo accumulation) of one of neighbours:
     index in {0..8} */
  T getTopoRank(unsigned short index) const {
    return toporwin.get(index);
  }

  /***************************************************************/
  flowPriority getPriority() const {
    return flowPriority(elevwin.get(), toporwin.get(), i,j);
  }

  /***************************************************************/
  /* define the operators which will be used to sort the sweeping
  stream the sweep elements should be ordered in increasing orders of
  flowPriority */
  friend int operator < (const sweepItemBaseType<T> &x, 
			 const sweepItemBaseType<T> &y) {
    if (x.getPriority() < y.getPriority()) return 1;
    return (0);
  }
  
  /***************************************************************/
  friend int operator > (const sweepItemBaseType<T> &x, 
			 const sweepItemBaseType<T> &y) {
    if (x.getPriority() > y.getPriority()) return 1;
    return (0);
  } 
  
  /***************************************************************/
  friend ostream& operator<<(ostream& s, const sweepItemBaseType<T> &x) {
    return  s << x.getPriority() << ", " 
	      << x.getDir() <<"\n"
	      << "elev:\n" << x.getElevWindow()
	      << "topo rank:\n" << x.getTopoRankWindow();
  } 
  
};



class PrioCmpSweepItem {
public: 
  static int compare(const sweepItem &a, const sweepItem &b) {
    return flowPriority::compare(a.getPriority(), b.getPriority());
  }
};






/************************************************************/
class flowValue {
public:
  flowaccumulation_type value;

public:
  flowValue(flowaccumulation_type x=0): value(x) {}
  
  ~flowValue() {}
  
  flowaccumulation_type get() const {
    return value;
  }
  friend ostream& operator<<(ostream& s, const flowValue &elt) {
    return s << elt.value;
  }
  friend istream& operator>>(istream& s, flowValue &elt) {
    return s >> elt.value;
  }
  friend flowValue operator +(const flowValue &elt1, 
				       const flowValue &elt2) {
    flowValue elt(elt1.value + elt2.value);
    return elt;
  }
  flowValue operator =(const flowValue &elt) {
    value = elt.value;
    return *this;
  } 
  flowValue operator != (const flowValue &elt) {
    return value != elt.value;
  }
  flowValue operator == (const flowValue &elt) {
    return value == elt.value;
  }
  
  friend int operator > (const flowValue &p1, const flowValue &p2) {
    return (p1.value > p2.value);
  }
  friend int operator >= (const flowValue &p1, 
			  const flowValue &p2) {
    return (p1.value >= p2.value);
  }
  friend int operator < (const flowValue &p1, 
			 const flowValue &p2) {
    return (p1.value < p2.value);
  }
  friend int operator <= (const flowValue &p1, 
			  const flowValue &p2) {
    return (p1.value <= p2.value);
  }

};



/************************************************************/
class flowStructure  {
private:
  flowPriority prio;
  flowValue val;

public:
  
  flowStructure(const flowPriority &p = 0, const flowValue &e = 0):
    prio(p), val(e) {}
    
  /* flowStructure(const flowValue &e, const flowPriority &p):
	 prio(p), val(e) {}
  */ 
  flowStructure(const flowStructure &fl) {
    prio = fl.prio;
    val = fl.val;
  }
  
  ~flowStructure() {}
  
  flowPriority getPriority() const {
    return prio;
  }
  
  flowValue getValue() const {
    return val;
  }
  
  friend ostream& operator<<(ostream& s, const flowStructure &fl) {
    return s << "[<prio=" << fl.prio << "> " << fl.val <<"]";
  }
  
  friend int operator < (const flowStructure &f1, 
			 const flowStructure &f2) {
    return (f1.prio < f2.prio);
  }
  
  friend int operator <= (const flowStructure &f1, 
			  const flowStructure &f2) {
    return (f1.prio <= f2.prio);
  }
  friend int operator > (const flowStructure &f1, 
			 const flowStructure &f2) {
    return (f1.prio > f2.prio);
  }
  friend int operator >= (const flowStructure &f1,
			  const flowStructure &f2) {
    return (f1.prio >= f2.prio);
  }
  friend bool operator == (const flowStructure &f1, 
			   const flowStructure &f2) {
    return (f1.prio == f2.prio);
  }
  friend bool operator != (const flowStructure &f1, 
			   const flowStructure &f2) {
    return (f1.prio != f2.prio);
  }

  friend flowStructure operator +(const flowStructure &x, 
				  const flowStructure &y) {
    flowValue val = x.val + y.val;
    flowStructure f(x.prio, val);
    return f;
  }

  static int qscompare(const void *a, const void *b) {
	flowStructure* x, *y;
	x = (flowStructure*) a;
	y = (flowStructure*) b;
	if (*x < *y) return -1;
	if (*x == *y) return 0;
	return 1;
  }
};
  





/***************************************************************/
/* Read the points in order from the sweep stream and process them.
   If trustdir = 1 then trust and use the directions contained in the
   sweep stream. Otherwise push flow to all downslope neighbors and
   use the direction only for points without downslope neighbors. */
/***************************************************************/
AMI_STREAM<sweepOutput>* sweep(AMI_STREAM<sweepItem> *sweepstr,
			       flowaccumulation_type D8CUT,
			       const int trustdir);

#endif


