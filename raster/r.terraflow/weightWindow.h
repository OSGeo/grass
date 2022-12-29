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

#ifndef _weight_H
#define _weight_H

#include <stdio.h>

#include "types.h"         
#include "common.h"
#include "genericWindow.h"  
#include "direction.h"  



class weightWindow {
  
public:
  float cell_dx, cell_dy;       /* dimension of cell in the grid */
  float celldiag;     /* diagonal of a cell in the grid */
  float sumweight, sumcontour;
  genericWindow<float> weight;  /* weights */

protected:
 
  /* initialize all weights to 0 */
  void init();
  /* set weight of neighbor (di,dj) */
  void computeWeight(const short di, const short dj, 
		     const elevation_type elev_crt,
		     const elevation_type elev_neighb);
  /* normalize weights */
  void normalize();

  /* computes the contour corresponding to this direction  */
  double computeContour(const short di, const short dj);
 
  /* computes the distance corresponding to this direction */
  double computeDist(const short di, const short dj); 

  /* compute the tanB corresponding to the elevation window and
     neighbor di,dj. */
  double computeTanB(const short di,const short dj, 
		     const genericWindow<elevation_type>& elevwin);
		    

public:

  weightWindow(const float gdx, const float gdy);
  
  ~weightWindow(){};

  /***************************************************************/
  /* Compute the weights of the neighbors of a cell given an elevation
     window and a direction window; if trustdir = 1 then trust
     directions; otherwise compute the downslope neighbors and use
     direction only for cells which do not have downslope neighbors */
  /***************************************************************/
  void compute(const dimension_type i, const dimension_type j,
	       const genericWindow<elevation_type>& elevwin, 
	       const direction_type dir,
	       const int trustdir);
  
  /* Find the dominant direction. Set corresponding weight to 1, and
     sets all other weights to 0. Set sumweight and sumcontour.*/
  void makeD8(const dimension_type i, const dimension_type j,
	      const genericWindow<elevation_type>& elevwin, 
	      const direction_type dir,
	      const bool trustdir);
  
  /* get specified weight di,dj in {-1,0,1} */
  float get(const short di, const short dj) const {
    return weight.get(di,dj);
  }
  
  /* get specified weight i in 0..8 */
  float get(const unsigned short i) const {
    return weight.get(i);
  }

  /* return the total contour */
  float totalContour() const {
    return sumcontour;
  }
  
  float totatWeight() const {
    return sumweight;
  }

  float dx() const {
    return cell_dx;
  }

  float dy() const {
    return cell_dy;
  }
  
  friend ostream& operator<<(ostream& s, const weightWindow &x) {
    return s << x.weight;
  }
};

#endif

