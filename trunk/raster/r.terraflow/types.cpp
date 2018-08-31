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


#include "types.h"


cclabel_type labelFactory::label = labelFactory::getLabelInit();

/* ---------------------------------------------------------------------- */

int 
ijBaseType::compare(const ijBaseType &a, const ijBaseType &b) {
  if(a.i < b.i) return -1;
  if(a.i > b.i) return 1;
  
  if(a.j < b.j) return -1;
  if(a.j > b.j) return 1;
  
  return 0;
}



std::ostream&
operator << (std::ostream& s, const ijBaseType &p) {
  return s << "(" << p.i << "," << p.j << ")";
}

/* ---------------------------------------------------------------------- */
