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


#ifndef _MINMAXHEAP_H
#define _MINMAXHEAP_H

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

#ifdef log2
#undef log2
#endif

#include <sstream>

#include "mm_utils.h"
#include "ami_config.h"  //for SAVE_MEMORY flag
/* this flag is set if we are stingy on memory; in that case 'reset'
   deletes the pq memory and the subsequent insert reallocates it; if
   the operation following a reset is not an insert or an operation
   which does not touch the array A, behaviour is unpredictable (core
   dump probably) */





/***************************************************************** 
 ***************************************************************** 
 ***************************************************************** 

Priority queue templated on a single type (assumed to be a class with
getPriority() and getValue() implemented);

Supported operations: min, extract_min, insert, max, extract_max in
O(lg n)

***************************************************************** 
***************************************************************** 
*****************************************************************/

#undef XXX
#define XXX if(0)


#define MY_LOG_DEBUG_ID(x) //inhibit debug printing
//#define MY_LOG_DEBUG_ID(x) LOG_DEBUG_ID(x)

typedef unsigned int HeapIndex;


template <class T>
class BasicMinMaxHeap {
protected:  
  HeapIndex maxsize;
  HeapIndex lastindex;			// last used position (0 unused) (?)
  T *A;

protected:
  /* couple of memory mgt functions to keep things consistent */
  static T *allocateHeap(HeapIndex n);
  static void freeHeap(T *);

public:
  BasicMinMaxHeap(HeapIndex size) : maxsize(size) { 
    char str[100];
    sprintf(str, "BasicMinMaxHeap: allocate %ld\n", 
			(long)((size+1)*sizeof(T)));
    // MEMORY_LOG(str);
    
    lastindex = 0;
    MY_LOG_DEBUG_ID("minmaxheap: allocation");
	A = allocateHeap(maxsize);
  };
  
  virtual ~BasicMinMaxHeap(void) { 
    MY_LOG_DEBUG_ID("minmaxheap: deallocation");
	freeHeap(A);
  };

  bool empty(void) const { return size() == 0; };
  HeapIndex size() const { 
	assert(A ||  !lastindex);
    return lastindex; 
  };

  T get(HeapIndex i) const { assert(i <= size()); return A[i]; }
   
  //build a heap from an array of elements; 
  //if size > maxsize, insert first maxsize elements from array;
  //return nb of elements that did not fit;
  
  void insert(const T& elt);

  bool min(T& elt) const ;
  bool extract_min(T& elt);
  bool max(T& elt) const;
  bool extract_max(T& elt);
  //extract all elts with min key, add them and return their sum
  bool extract_all_min(T& elt);

  void reset();
  void clear();		/* mark all the data as deleted, but don't do free */

  void destructiveVerify();

  void verify();
  
  void print() const;
  void print_range() const;
  friend ostream& operator<<(ostream& s, const BasicMinMaxHeap<T> &pq) {
    HeapIndex i;
    s <<  "[";
    for(i = 1; i <= pq.size(); i++) {
      s << " " << pq.get(i);
    }
    s << "]";
    return s;
  }

protected:
  virtual void grow()=0;

private:
  long log2(long n) const;
  int isOnMaxLevel(HeapIndex i) const { return (log2(i) % 2); };
  int isOnMinLevel(HeapIndex i) const { return !isOnMaxLevel(i); };

  HeapIndex leftChild(HeapIndex i) const { return 2*i; };
  HeapIndex rightChild(HeapIndex i) const { return 2*i + 1; };
  int hasRightChild(HeapIndex i) const { return (rightChild(i) <= size()); };
  int hasRightChild(HeapIndex i, HeapIndex *c) const { return ((*c=rightChild(i)) <= size()); };
  HeapIndex parent(HeapIndex i) const { return (i/2); };
  HeapIndex grandparent(HeapIndex i) const { return (i/4); };
  int hasChildren(HeapIndex i) const { return (2*i) <= size(); }; // 1 or more
  void swap(HeapIndex a, HeapIndex b);

  T leftChildValue(HeapIndex i) const;
  T rightChildValue(HeapIndex i) const;
  HeapIndex smallestChild(HeapIndex i) const;
  HeapIndex smallestChildGrandchild(HeapIndex i) const;
  HeapIndex largestChild(HeapIndex i) const;
  HeapIndex largestChildGrandchild(HeapIndex i) const;
  int isGrandchildOf(HeapIndex i, HeapIndex m) const;
  
  void trickleDownMin(HeapIndex i);
  void trickleDownMax(HeapIndex i);
  void trickleDown(HeapIndex i);
  
  void bubbleUp(HeapIndex i);
  void bubbleUpMin(HeapIndex i);
  void bubbleUpMax(HeapIndex i);
};


// index 0 is invalid
// index <= size

// ----------------------------------------------------------------------

template <class T> 
long BasicMinMaxHeap<T>::log2(long n) const {
  long i=-1;
  // let log2(0)==-1
  while(n) {
	n = n >> 1;
	i++;
  }
  return i;
}


// ----------------------------------------------------------------------

template <class T> 
void BasicMinMaxHeap<T>::swap(HeapIndex a, HeapIndex b) {
  T tmp;
  tmp = A[a];
  A[a] = A[b];
  A[b] = tmp;
}


// ----------------------------------------------------------------------

// child must exist
template <class T>
T BasicMinMaxHeap<T>::leftChildValue(HeapIndex i) const {
  HeapIndex p = leftChild(i);
  assert(p <= size());
  return A[p];
}

// ----------------------------------------------------------------------

// child must exist
template <class T>
T BasicMinMaxHeap<T>::rightChildValue(HeapIndex i) const {
  HeapIndex p = rightChild(i);
  assert(p <= size());
  return A[p];
}


// ----------------------------------------------------------------------

// returns index of the smallest of children of node
// it is an error to call this function if node has no children
template <class T>
HeapIndex BasicMinMaxHeap<T>::smallestChild(HeapIndex i) const {
  assert(hasChildren(i));
  if(hasRightChild(i) && (leftChildValue(i) > rightChildValue(i))) {
	return rightChild(i);
  } else {
	return leftChild(i);
  }
}

// ----------------------------------------------------------------------

template <class T>
HeapIndex BasicMinMaxHeap<T>::largestChild(HeapIndex i) const {
  assert(hasChildren(i));
  if(hasRightChild(i) && (leftChildValue(i) < rightChildValue(i))) {
	return rightChild(i);
  } else {
	return leftChild(i);
  }
}

// ----------------------------------------------------------------------

// error to call on node without children
template <class T>
HeapIndex BasicMinMaxHeap<T>::smallestChildGrandchild(HeapIndex i) const {
  HeapIndex p,q;
  HeapIndex minpos = 0;

  assert(hasChildren(i));

  p = leftChild(i);
  if(hasChildren(p)) {
	q = smallestChild(p);
	if(A[p] > A[q]) p = q;
  }
  // p is smallest of left child, its grandchildren
  minpos = p;

  if(hasRightChild(i,&p)) {
	//p = rightChild(i);
	if(hasChildren(p)) {
	  q = smallestChild(p);
	  if(A[p] > A[q]) p = q;
	}
	// p is smallest of right child, its grandchildren
	if(A[p] < A[minpos]) minpos = p;
  }
  return minpos;
}

// ----------------------------------------------------------------------

template <class T>
HeapIndex BasicMinMaxHeap<T>::largestChildGrandchild(HeapIndex i) const {
  HeapIndex p,q;
  HeapIndex maxpos = 0;

  assert(hasChildren(i));

  p = leftChild(i);
  if(hasChildren(p)) {
	q = largestChild(p);
	if(A[p] < A[q]) p = q;
  }
  // p is smallest of left child, its grandchildren
  maxpos = p;

  if(hasRightChild(i,&p)) {
	//p = rightChild(i);
	if(hasChildren(p)) {
	  q = largestChild(p);
	  if(A[p] < A[q]) p = q;
	}
	// p is smallest of right child, its grandchildren
	if(A[p] > A[maxpos]) maxpos = p;
  }
  return maxpos;
}

// ----------------------------------------------------------------------

// this is pretty loose - only to differentiate between child and grandchild
template <class T>
int BasicMinMaxHeap<T>::isGrandchildOf(HeapIndex i, HeapIndex m) const {
  return (m >= i*4);
}

// ----------------------------------------------------------------------

template <class T>
void BasicMinMaxHeap<T>::trickleDownMin(HeapIndex i) {
  HeapIndex m;
  bool done = false;
  
  while (!done) {
    
    if (!hasChildren(i)) {
      done = true;
      return;
    }
    m = smallestChildGrandchild(i);
    if(isGrandchildOf(i, m)) {
      if(A[m] < A[i]) {
	swap(i, m);
	if(A[m] > A[parent(m)]) {
	  swap(m, parent(m));
	}
	//trickleDownMin(m);
	i = m;
      } else {
	done = true;
      }
    } else {
      if(A[m] < A[i]) {
	swap(i, m);
      }
      done = true;
    }
  }//while
}

// ----------------------------------------------------------------------

// unverified
template <class T>
void BasicMinMaxHeap<T>::trickleDownMax(HeapIndex i) {
  HeapIndex m;
  bool done = false;

  while (!done) {
    if(!hasChildren(i)) {
     done = true;
     return;
    }
    
    m = largestChildGrandchild(i);
    if(isGrandchildOf(i, m)) {
      if(A[m] > A[i]) {
	swap(i, m);
	if(A[m] < A[parent(m)]) {
	  swap(m, parent(m));
	}
	//trickleDownMax(m);
	i = m;
      } else {
	done = true;
      }
    } else {
      if(A[m] > A[i]) {
	swap(i, m);
      }
      done = true;
    }
  } //while
}


// ----------------------------------------------------------------------


template <class T>
void BasicMinMaxHeap<T>::trickleDown(HeapIndex i) {
  if(isOnMinLevel(i)) {
	trickleDownMin(i);
  } else {
	trickleDownMax(i);
  }
}

// ----------------------------------------------------------------------
template <class T>
void BasicMinMaxHeap<T>::bubbleUp(HeapIndex i) {
  HeapIndex m;
  m = parent(i);
  
  if(isOnMinLevel(i)) {
	if (m && (A[i] > A[m])) {
	  swap(i, m);
	  bubbleUpMax(m);
	} else {
	  bubbleUpMin(i);
	} 
  } else {
	if (m && (A[i] < A[m])) {
	  swap(i, m);
	  bubbleUpMin(m);
	} else {
	  bubbleUpMax(i);
	}
  }
}


// ----------------------------------------------------------------------
template <class T>
void BasicMinMaxHeap<T>::bubbleUpMin(HeapIndex i) {
  HeapIndex m;
  m = grandparent(i);

  while (m && (A[i] < A[m])) {
	 swap(i,m);
	 //bubbleUpMin(m);
	 i = m;
	 m = grandparent(i);
	 
  }
}



// ----------------------------------------------------------------------
template <class T>
void BasicMinMaxHeap<T>::bubbleUpMax(HeapIndex i) {
  HeapIndex m;
  m = grandparent(i);
  
  while(m && (A[i] > A[m])) {
	swap(i,m);
	//bubbleUpMax(m);
	i=m;
	m = grandparent(i);
  }
}


#if(0)
// ----------------------------------------------------------------------
template <class T>
void BasicMinMaxHeap<T>::print_rajiv() const {
  HeapIndex i;
  ostrstream *ostr = new ostrstream();
  
  *ostr << "[1]";
  for(i=1; i<=size(); i++) {
	*ostr << " " << A[i];
	if(ostr->pcount() > 70) {
	  cout << ostr->str() << endl;
	  delete ostr;
	  ostr = new ostrstream();
	  *ostr << "[" << i << "]";
	}
  }
  cout << ostr->str() << endl;
}
#endif



// ----------------------------------------------------------------------
template <class T>
void BasicMinMaxHeap<T>::print() const {
  cout << "[";
  for (unsigned int i=1; i<=size(); i++) {
    cout << A[i].getPriority() <<",";
  }
  cout << "]" << endl;
}

// ----------------------------------------------------------------------
template <class T>
void BasicMinMaxHeap<T>::print_range() const {
  cout << "[";
  T a, b;
  min(a);
  max(b);
  if (size()) {
    cout << a.getPriority() << ".."
	 << b.getPriority();
  }
  cout << " (" << size() << ")]";
}


// ----------------------------------------------------------------------
template <class T>
void BasicMinMaxHeap<T>::insert(const T& elt) {
#ifdef SAVE_MEMORY 
  if (!A) {
    MY_LOG_DEBUG_ID("minmaxheap: re-allocation");
    A = allocateHeap(maxsize);
  }
#endif

  if(lastindex == maxsize) grow();

  XXX cerr << "insert: " << elt << endl;

  lastindex++;
  A[lastindex] = elt;
  bubbleUp(lastindex);
}

// ----------------------------------------------------------------------
template <class T>
bool BasicMinMaxHeap<T>::extract_min(T& elt) {

  assert(A);

  if(lastindex == 0) return false;

  elt = A[1];
  A[1] = A[lastindex];
  lastindex--;
  trickleDown(1);
  
  return true;
}

// ----------------------------------------------------------------------
//extract all elts with min key, add them and return their sum
template <class T>
bool BasicMinMaxHeap<T>::extract_all_min(T& elt) {
  T next_elt;
  bool done = false;
  
  //extract first elt
  if (!extract_min(elt)) {
    return false; 
  } else {
    while (!done) {
      //peek at the next min elt to see if matches
      if ((!min(next_elt)) || 
	  !(next_elt.getPriority() == elt.getPriority())) {
	done = true; 
      } else {
	extract_min(next_elt);
	elt = elt + next_elt;
      }
    }
  }
  return true;
}

// ----------------------------------------------------------------------
template <class T>
bool BasicMinMaxHeap<T>::extract_max(T& elt) {

  assert(A);
  
  HeapIndex p;					// max
  if(lastindex == 0) return false;
  
  if(hasChildren(1)) {
	p = largestChild(1);
  } else {
	p = 1;
  }
  elt = A[p];
  A[p] = A[lastindex];
  lastindex--;
  trickleDown(p);
  
  return true;
}

// ----------------------------------------------------------------------
template <class T>
bool BasicMinMaxHeap<T>::min(T& elt) const {
  
  assert(A);
  
  if(lastindex == 0) return false;

  elt = A[1];
  return true;
}

// ----------------------------------------------------------------------
template <class T>
bool BasicMinMaxHeap<T>::max(T& elt) const {
  
  assert(A);
  
  HeapIndex p;					// max
  if(lastindex == 0) return false;
  
  if(hasChildren(1)) {
	p = largestChild(1);
  } else {
	p = 1;
  }
  elt = A[p];
  return true;
}



// ----------------------------------------------------------------------
//free memory if SAVE_MEMORY is set
template <class T>
void BasicMinMaxHeap<T>::reset() {
#ifdef SAVE_MEMORY
  assert(empty());
  MY_LOG_DEBUG_ID("minmaxheap: deallocation");
  freeHeap(A);
  A = NULL;
#endif
}

// ----------------------------------------------------------------------

template <class T> 
void
BasicMinMaxHeap<T>::clear() {
  lastindex = 0;
}

// ----------------------------------------------------------------------
template <class T> 
T *
BasicMinMaxHeap<T>::allocateHeap(HeapIndex n) {
  T *p;
#ifdef USE_LARGEMEM
  p = (T*)LargeMemory::alloc(sizeof(T) * (n+1));
#else
  p = new T[n+1]; 
#endif
  return p;
}

// ----------------------------------------------------------------------
template <class T> 
void
BasicMinMaxHeap<T>::freeHeap(T *p) {
  if (p) {
#ifdef USE_LARGEMEM
	LargeMemory::free(p);
#else
	delete [] p;
#endif
  }
}
  

// ----------------------------------------------------------------------

template <class T> 
void
BasicMinMaxHeap<T>::destructiveVerify() {
  HeapIndex n = size();
  T val, prev;
  bool ok;

  if(!n) return;

  XXX print();

  /* make sure that min works */
  extract_min(prev);
  for(HeapIndex i=1; i<n; i++) {
	ok = min(val);
	assert(ok);
	XXX cerr << i << ": " << val << endl;
	if(val.getPriority() < prev.getPriority()) { // oops!
	  print();
	  cerr << "n=" << n << endl;
	  cerr << "val=" << val << endl;
	  cerr << "prev=" << prev << endl;
	  cerr << "looks like minmaxheap.min is broken!!" << endl;
	  assert(0);
	  return;
	}
	prev = val;
	ok = extract_min(val);
	assert(ok);
	assert(prev == val);
  }
}


// ----------------------------------------------------------------------

template <class T> 
void
BasicMinMaxHeap<T>::verify() {
  long n = size();
  T *dup;
  
  if(!n) return;

  dup = allocateHeap(maxsize);
  for(HeapIndex i=0; i<n+1; i++) {
	dup[i] = A[i];
  }
  destructiveVerify();
  freeHeap(A);
  /* restore the heap */
  A = dup;
  lastindex = n;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

template <class T>
class MinMaxHeap : public BasicMinMaxHeap<T> {
public:
  MinMaxHeap(HeapIndex size) : BasicMinMaxHeap<T>(size) {};
  virtual ~MinMaxHeap() {};
  bool full(void) const { return this->size() >= this->maxsize; };
  HeapIndex get_maxsize() const { return this->maxsize; };
  HeapIndex fill(T* arr, HeapIndex n);
  
protected:
  virtual void grow() { fprintf(stderr, "MinMaxHeap::grow: not implemented\n"); assert(0); exit(1); };
};

// ----------------------------------------------------------------------
//build a heap from an array of elements; 
//if size > maxsize, insert first maxsize elements from array;
//return nb of elements that did not fit;
template <class T>
HeapIndex MinMaxHeap<T>::fill(T* arr, HeapIndex n) {
  HeapIndex i;
  //heap must be empty
  assert(this->size()==0);
  for (i = 0; !full() && i<n; i++) {
    this->insert(arr[i]);
  }
  if (i < n) {
    assert(i == this->maxsize);
    return n - i;
  } else {
    return 0;
  }
}



#define MMHEAP_INITIAL_SIZE 1024

template <class T>
class UnboundedMinMaxHeap : public BasicMinMaxHeap<T> {
public:
  UnboundedMinMaxHeap() : BasicMinMaxHeap<T>(MMHEAP_INITIAL_SIZE) {};
  UnboundedMinMaxHeap(HeapIndex size) : BasicMinMaxHeap<T>(size) {};
  virtual ~UnboundedMinMaxHeap() {};
protected:
  virtual void grow();
};

template <class T>
void UnboundedMinMaxHeap<T>::grow() {
  T *old = this->A;
  this->maxsize *= 2;

  assert(this->maxsize > 0);

  if(old) {
	HeapIndex n = this->size();
	this->A = this->allocateHeap(this->maxsize);  /* allocate a new array */
	/* copy over the old values */
	assert(this->maxsize > n);
	for(HeapIndex i=0; i<=n; i++) {	/* why extra value? -RW */
	  this->A[i] = old[i];
	}	
	this->freeHeap(old);			/* free up old storage */
  }

}


#endif
