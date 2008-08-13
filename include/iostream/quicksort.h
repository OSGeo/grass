/****************************************************************************
 * 
 *  MODULE:     iostream
 *

 *  COPYRIGHT (C) 2007 Laura Toma
 *   
 * 

 *  Iostream is a library that implements streams, external memory
 *  sorting on streams, and an external memory priority queue on
 *  streams. These are the fundamental components used in external
 *  memory algorithms.  

 * Credits: The library was developed by Laura Toma.  The kernel of
 * class STREAM is based on the similar class existent in the GPL TPIE
 * project developed at Duke University. The sorting and priority
 * queue have been developed by Laura Toma based on communications
 * with Rajiv Wickremesinghe. The library was developed as part of
 * porting Terraflow to GRASS in 2001.  PEARL upgrades in 2003 by
 * Rajiv Wickremesinghe as part of the Terracost project.

 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.  *
 *  **************************************************************************/



#ifndef _QUICKSORT_H
#define _QUICKSORT_H
 
#include <stdlib.h> //for random()


// The class represented by CMPR, must have a member function called
// "compare" which is used for sorting



/* ---------------------------------------------------------------------- */
// On return from partition(), everything at or below pivot will be
// less that or equal to everything above it.  Furthermore, it will
// not be 0 since this will leave us to recurse on the whole array
// again.
template<class T, class CMPR>
void partition(T *data, size_t n, size_t &pivot, CMPR &cmp) {
    T *ptpart, tpart;
    T *p, *q;
    T t0;
    
    // Try to get a good partition value and avoid being bitten by already
    // sorted input.
    //ptpart = data + (random() % n);
#ifdef __MINGW32__
    ptpart = data + (rand() % n);
#else
    ptpart = data + (random() % n);
#endif

    tpart = *ptpart;
    *ptpart = data[0];
    data[0] = tpart;
    
    // Walk through the array and partition it.
    for (p = data - 1, q = data + n; ; ) {
      
      do {
	q--;
      } while (cmp.compare(*q, tpart) > 0);
      do {
	p++;
      } while (cmp.compare(*p, tpart) < 0);
      
      if (p < q) {
	t0 = *p;
	*p = *q;
	*q = t0;
      } else {
	pivot = q - data;            
	break;
      }
    }
}




/* ---------------------------------------------------------------------- */
template<class T, class CMPR>
void insertionsort(T *data, size_t n, CMPR &cmp) {
  T *p, *q, test;
  
  for (p = data + 1; p < data + n; p++) {
    for (q = p - 1, test = *p; (cmp.compare(*q, test) > 0); q--) {
      *(q+1) = *q;
      if (q==data) {
	q--; // to make assignment below correct
	break;
      }
    }
    *(q+1) = test;
  }
}




/* ---------------------------------------------------------------------- */
template<class T, class CMPR>
void quicksort(T *data, size_t n, CMPR &cmp, size_t min_len = 20)  {

  size_t pivot;
  if (n < min_len) {
    insertionsort(data, n, cmp);
    return;
  }
  //else
  partition(data, n, pivot, cmp);
  quicksort(data, pivot + 1, cmp, min_len);
  quicksort(data + pivot + 1, n - pivot - 1, cmp, min_len);
}




#endif // _QUICKSORT_H 














































