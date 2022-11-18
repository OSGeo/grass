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

#ifndef _genericwindow_H
#define _genericwindow_H

#include <stdio.h>

#include <grass/iostream/ami.h>
#include "types.h"


/* ************************************************************* *
 * class 'genericWindow' implements a 3x3 window in a grid; 
 * ************************************************************* */

template<class T>
class genericWindow {
protected:
  T data[9]; 
public:

  /***************************************************************/
  /* initialize a window to 0 */
  genericWindow() {
    for (int i=0; i<9;i++) {
	  data[i] = T();
	}
  }

  /***************************************************************/
  /* initialize a window from an array of 9 values */
  genericWindow(T* a) {
    assert(a);
    for (int i=0; i<9;i++) {
	  data[i] = a[i];
	}
  }

  /***************************************************************/
  /* initialize a window from 3 arrays of 3 elements each */
  genericWindow(T *a, T *b, T*c) {
    int i;
    assert(a); assert(b); assert(c);
    for (i=0;i<3;i++) {
	  data[i] = a[i];
	  data[i+3] = b[i];
	  data[i+6] = c[i];	  
    }
  }

  /***************************************************************/
  /* initialize a window from 3 arrays of 3 elements each */
  template<class C>
  genericWindow(C *a, C *b, C*c) {
    int i;
    assert(a); assert(b); assert(c);
    for (i=0;i<3;i++) {
	  data[i] = a[i];
	  data[i+3] = b[i];
	  data[i+6] = c[i];	  
    }
  }

  /***************************************************************/
  genericWindow(const genericWindow<T> &win) {
    for (int i=0;i<9;i++) {
	  data[i] = win.data[i];
    }
  }

  /***************************************************************/
  /* get specified neighbour di,dj in {-1,0,1} */
  T get(short di, short dj) const {
    assert (di>=-1 && di<=1);
    assert(dj>=-1 && dj<=1);
	return data[4+dj+di*3];
  }

  /***************************************************************/
  /* get specified neighbour i in 0..8 */
  T get(unsigned short i=4) const {
	assert(i <= 8);
	return data[i];
  }

  /***************************************************************/
  /* set specified neighbour i in 0..8 */
  void set(unsigned short i, T val) {
    assert(i <= 8);
	data[i] = val;
  }
  
  /***************************************************************/
  /* set specified neighbour di,dj in {-1,0,1} */
  void set(int di, int dj, T val) {
    assert (di>=-1 && di<=1);
    assert(dj>=-1 && dj<=1);
	data[4+dj+di*3] = val;
  }

  /***************************************************************/
  /*  multiply all elements by a scalar */
  void scalarMultiply(T mult) {
	for(int i=0; i<9; i++) {
	  data[i] *= mult;
	}
  }

  /***************************************************************/
  inline friend ostream& operator<<(ostream& s, const genericWindow<T> &x) {
	s << "[" << x.data[0] << "," << x.data[1] << "," << x.data[2] << "]\n";
	s << "[" << x.data[3] << "," << x.data[4] << "," << x.data[5] << "]\n";
	s << "[" << x.data[6] << "," << x.data[7] << "," << x.data[8] << "]\n";
	return s;
  }

};

typedef genericWindow<elevation_type> ElevationWindow;

void fillPit(ElevationWindow& win);

#endif

