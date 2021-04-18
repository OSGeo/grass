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

#ifndef QUEUE_H
#define QUEUE_H

#include <assert.h>
#include <iostream>

template<class T> 
class queue {
private:
  T *data;
  int size;
  int head;  // first valid location (if data)
  int tail;  // next free location
  int len;
  void grow();
public:
  queue(int size=4096);
  ~queue();
  bool enqueue(T &);
  bool dequeue(T *);
  bool peek(int offset, T *);
  bool isEmpty() const { return len==0; };
  //int length() const { return len; };
  unsigned int length() const { return (unsigned int)len; };
};


template<class T> 
queue<T>::queue(int vsize) : size(vsize) {

  if(size <= 0) size = 64;		/* default */

  data = new T[size];
  head = 0;
  tail = 0;
  len = 0;
}


template<class T> 
queue<T>::~queue() {
  delete [] data;
}


template<class T> 
bool
queue<T>::enqueue(T &elt) {
  if(len==size) grow();
  assert(len<size);
  data[tail] = elt;
  tail = (tail+1)%size;
  len++;
  return true;
}

template<class T> 
bool 
queue<T>::dequeue(T *elt) {
  if(len>0) {
	*elt = data[head];	  
	head = (head+1)%size;
	len--;
	return true;
  } 
  return false;
}


template<class T> 
bool 
queue<T>::peek(int offset, T *elt) {
  if(len>offset) {
	int pos = (head+offset)%size;
	*elt = data[pos];	  
	return true;
  } 
  return false;
}

template<class T> 
void
queue<T>::grow() {
  T *data2 = new T[size*2];
  int k=head;
  for(int i=0; i<len; i++) {
	data2[i] = data[k];
	k = (k+1)%size;
  }
  head = 0;
  tail = len;
  delete [] data;
  data = data2;
  size *= 2;
}


#endif // QUEUE_H
