
/****************************************************************************
 * 
 *  MODULE:	iostream
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


#ifndef __EMPQ_ADAPTIVE_H
#define __EMPQ_ADAPTIVE_H


#include "minmaxheap.h"
#include "empq.h"
#include "empq_impl.h"



#define EMPQAD_DEBUG if(1)


enum regim_type {
  INMEM = 0,
  EXTMEM,
  EXTMEM_DEBUG
};


template<class T, class Key> 
class EMPQueueAdaptive {
private: 
  //dictates if the structure works in the internal/external memory regim;
  regim_type regim;  
  MinMaxHeap<T> *im;
  em_pqueue<T,Key> *em;
  UnboundedMinMaxHeap<T> *dim;	// debug, internal memory pq
  void initPQ(size_t);
public:
  /* start in INMEM regim by allocating im of size precisely twice the
     size of the (pqueue within) the em_pqueue; */
  EMPQueueAdaptive(long N) : EMPQueueAdaptive() {};
  EMPQueueAdaptive();
  EMPQueueAdaptive(size_t inMem);
  ~EMPQueueAdaptive();

  void makeExternal();
  void makeExternalDebug();

  long maxlen() const;			//return the maximum nb of elts that can fit
  bool is_empty() const;		//return true if empty
  bool is_full() const;			//return true if full
  bool min(T& elt);				//return the element with min priority XXX
  //delete the element with minimum priority in the structure;
  //return false if pq is empty
  bool extract_min(T& elt);

  //extract all elts with min key, add them and return their sum XXX
  bool extract_all_min(T& elt);

  /* insert an element; if regim == INMEM, try insert it in im, and if
     it is full, extract_max pqsize/2 elements of im into a stream,
     switch to EXTMEM and insert the stream into em; if regim is
     EXTMEM, insert in em; */
  bool insert(const T& elt);  

  long size() const; //return the nb of elements in the structure

  void clear();			/* delete all contents of pq */

  void verify();
};



#endif
