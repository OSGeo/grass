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


#ifndef _PQHEAP_H
#define _PQHEAP_H

#include <assert.h>
#include <stdlib.h>

#define PQHEAP_MEM_DEBUG if(0)


//HEAPSTATUS can be defined at compile time



//this flag is currently off; we used it at some point for checking
//how many times is each element in the heap accessed or something
//like that
#ifdef HEAPSTATUS
static const int PAGESIZE = 1024;
#endif


// Helper functions for navigating through a binary heap.
/* for simplicity the heap structure is slightly modified as:
   0
   |
   1
   /\
  2  3
 /\  /\
4 5 6  7

*/

// The children of an element of the heap.
static inline unsigned int heap_lchild(unsigned int index) {
  return 2 * index;
}

static inline unsigned int heap_rchild(unsigned int index) {
  return 2 * index + 1;
}

// The parent of an element.
static inline unsigned int heap_parent(unsigned int index) {
  return index >> 1;
}


// return minimum of two integers
static unsigned int mymin(unsigned int a, unsigned int b) {
  return (a<=b)? a:b;
}



/**************************************************************
***************************************************************
***************************************************************

Priority queue templated on a single type 

assume T to be a class with getPriority() and getValue() implemented;

Supported operations: min, extract_min, insert in O(lg n)


***************************************************************
***************************************************************
***************************************************************/
template <class T>
class pqheap_t1 {
  // A pointer to an array of elements
  T* elements;
  
  // The number of elements currently in the queue.
  unsigned int cur_elts;
  
  // The maximum number the queue can hold.
  unsigned int max_elts;

private:
  void heapify(unsigned int root);

public:
  inline pqheap_t1(unsigned int size);
  
  //build heap from an array of elements; array a is REUSED, and NOT
  //COPIED, for efficiency; it'd better not be used after this
  //outside!!!
  inline pqheap_t1(T* a, unsigned int size);

  inline ~pqheap_t1(void); 

  //build a heap from an array of elements; 
  //if size > max_elts, insert first maxsize elements from array;
  //return nb of elements that did not fit;
  unsigned int fill(T* a, unsigned int size);

  // Is it full?
  inline bool full(void);

  //Is it empty?
  inline bool empty(void);
  inline bool is_empty() { return empty(); };

  // How many elements?
  inline unsigned int num_elts(void);
  
  // How many elements? sorry - i could never remember num_elts
  inline unsigned int size(void) const { return cur_elts; };
  
  // Min
  inline bool min(T& elt);
  T min();

  // Extract min and set elt = min
  inline bool extract_min(T& elt);

  //extract all elts with min key, add them and return their sum
  inline bool extract_all_min(T& elt);

  //delete min; same as extract_min, but ignore the value extracted
  inline bool delete_min();

  // Insert
  inline bool insert(const T& elt);

  //Delete the current minimum and insert the new item x; 
  //the minimum item is lost (i.e. not returned to user); 
  //needed to optimize merge 
  inline void delete_min_and_insert(const T &x);

  //this function is a dirty way to allow building faster the heap 
  //in case we build it from a sorted array; in that case we dont need 
  //to 'insert' and then 'heapify', but it is enough to 'set'
  void set(long i, T& elt);

  //print
  inline friend ostream& operator<<(ostream& s, const pqheap_t1<T> &pq) {
    s << "PQ: "; s.flush();
    for (unsigned int i=0; i< mymin(10, pq.cur_elts); i++) {
      s <<  "[" 
	//<< pq.elements[i].getPriority() << "," 
	//<< pq.elements[i].getValue() 
	<< pq.elements[i]
	<< "]";
    }
    return s;
  }
  //print
  void print();

  //print
  void print_range();


#ifdef HEAPSTATUS
  inline void heapstatus(int d);
  inline void heaptouch(unsigned int pos);
  unsigned int *numtouch;
#endif
};


//************************************************************/
template <class T>
inline 
pqheap_t1<T>::pqheap_t1(unsigned int size) {


  elements = new T [size];
  cout << "pqheap_t1: register memory\n"; 
  cout.flush();
  PQHEAP_MEM_DEBUG cout << "pqheap_t1::pq_heap_t1: allocate\n";
  //  PQHEAP_MEM_DEBUG MMmanager.print();

  
  if (!elements) {
	cerr << "could not allocate priority queue: insufficient memory..\n";
    exit(1);
  }
  assert(elements);
  
  max_elts = size;
  cur_elts = 0;
  
#ifdef HEAPSTATUS
  numtouch = new unsigned int[size/PAGESIZE];
  assert(numtouch);
  for(int i=0; i<size/PAGESIZE; i++) {
	numtouch[i] = 0;
  }
#endif
}


//************************************************************/
/* (this constructor is a bit nasty) Build heap from an array of
   elements; array a is reused, and not copied, for efficiency; it'd
   better not be used after this outside!!! */
template <class T>
inline 
pqheap_t1<T>::pqheap_t1(T* a, unsigned int size) {
  {
	static int flag = 0;
	if(!flag) {
	  cerr << "Using slow build in pqheap_t1" << endl;
	  flag = 1;
	}
  }

  elements = a;
  max_elts = size;
  cur_elts = size;

  if (max_elts) {
    for (int i = heap_parent(max_elts-1); i>=0; i--) {
      //cout << "heapify i=" << i<<"\n";
      heapify(i);
    }
  }
}

//************************************************************/
template <class T>
inline 
pqheap_t1<T>::~pqheap_t1() {
#ifdef HEAPSTATUS
  cout << endl << "pagesize = " << PAGESIZE << endl;
  cout << "max_elts = " << max_elts << endl;
  unsigned int n = max_elts / PAGESIZE;
  for(unsigned int i=0; i<n; i++) {
    cout << form("PQTEMP %d\t%d", i, numtouch[i]) << endl;
  }
  delete [] numtouch;
#endif
  
  delete [] elements;
  cur_elts = 0;
  max_elts = 0;
  return;
}
 

//************************************************************/
//build a heap from an array of elements; 
//if size > max_elts, insert first maxsize elements from array;
//return nb of elements that did not fit;
template <class T>
inline unsigned int
pqheap_t1<T>::fill(T* a, unsigned int size) {
  unsigned int i;
  assert(cur_elts == 0);
  for (i = 0; i<size; i++) {
    if (!insert(a[i])) { 
      break;
    }
  }
  if (i < size) {
    assert(i == max_elts);
    return size - i;
  } else {
    return 0;
  }
}



//************************************************************/
template <class T>
inline bool 
pqheap_t1<T>::full(void) {
  return cur_elts == max_elts;
}

//************************************************************/
template <class T>
inline bool 
pqheap_t1<T>::empty(void) {
  return cur_elts == 0;
}

//************************************************************/
template <class T>
inline unsigned int 
pqheap_t1<T>::num_elts(void) {
  return cur_elts;
}

//************************************************************/
template <class T>
inline bool 
pqheap_t1<T>::min(T& elt) {
  if (!cur_elts) {
    return false;
  }
  elt = elements[0];
  return true;
}


//************************************************************/
template <class T>
T
pqheap_t1<T>::min() {
  T elt;
  if(min(elt)) {
	return elt;
  } else {
	cerr << "unguarded min failed" << endl;
	assert(0);
	exit(1);
  }
  return elt;
}




//************************************************************/
//this function is a dirty hack to allow building faster the heap 
//in case we build it from a sorted array; in thiat case we dont need 
//to 'insert' and then 'heapify', but it is enough to 'set'
template <class T>
inline void
pqheap_t1<T>::set(long i, T& elt) {
  //must always set precisely the next element
  assert(i == cur_elts);
  elements[i] = elt;
  cur_elts++;
}


//************************************************************/
#ifdef HEAPSTATUS
template <class T>
inline void pqheap_t1<T>::heaptouch(unsigned int pos) {
  numtouch[pos/PAGESIZE]++;
  assert(numtouch[pos/PAGESIZE] > 0);
}
#endif

#ifdef HEAPSTATUS
template <class T>
inline void pqheap_t1<T>::heapstatus(int d) {
  static int count = 0;
  static int delta = 0;
  
  delta += d;
  count++;
  
  if((count % 10000) == 0) {
    cout << endl << form("PQHEAP %d\t%d", cur_elts, delta) << endl;
    count = 0;
    delta = 0;
  }
}    
#endif



//************************************************************/
template <class T>
inline bool 
pqheap_t1<T>::extract_min(T& elt) {
  if (!cur_elts) {
    return false;
  }
  elt = elements[0];
  elements[0] = elements[--cur_elts];
  heapify(0);
  
#ifdef HEAPSTATUS
  heaptouch(cur_elts);
  heaptouch(0);
  heapstatus(-1);
#endif
  
  return true;
}

//************************************************************/
//extract all elts with min key, add them and return their sum
template <class T>
inline bool 
pqheap_t1<T>::extract_all_min(T& elt) {
  
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




//************************************************************/
template <class T>
inline bool 
pqheap_t1<T>::delete_min() {
  T dummy;
  return extract_min(dummy);
}


//************************************************************/
template <class T>
inline bool 
pqheap_t1<T>::insert(const T& elt) {
  unsigned int ii;
  
  if (full()) {
    return false;
  }
  
  for (ii = cur_elts++;
       ii && (elements[heap_parent(ii)].getPriority() > elt.getPriority());
       ii = heap_parent(ii)) {
    elements[ii] = elements[heap_parent(ii)];
  }
  elements[ii] = elt;
    
#ifdef HEAPSTATUS
  heaptouch(ii);
  heapstatus(+1);
#endif
  
  return true;
}                                       


//************************************************************/
template <class T>
inline void 
pqheap_t1<T>::heapify(unsigned int root) {
  unsigned int min_index = root;
  unsigned int lc = heap_lchild(root);
  unsigned int rc = heap_rchild(root);
  
#ifdef HEAPSTATUS
  // already did the root, so dont do it again
  if(lc < cur_elts) {
    heaptouch(lc);
  }
  if(rc < cur_elts) {
    heaptouch(rc);
  }
#endif
  if ((lc < cur_elts) && 
      ((elements[lc].getPriority()) < elements[min_index].getPriority())) {
    min_index = lc;
  }
  if ((rc < cur_elts) && 
      ((elements[rc].getPriority()) < elements[min_index].getPriority())) {
    min_index = rc;
  }
  
  if (min_index != root) {
    T tmp_q = elements[min_index];
    
    elements[min_index] = elements[root];
    elements[root] = tmp_q;
    
    heapify(min_index);
  }
}   


//************************************************************/
//Delete the current minimum and insert the new item; 
//the minimum item is lost (i.e. not returned to user); 
//needed to optimize merge 
template <class T>
inline void
pqheap_t1<T>::delete_min_and_insert(const T &x) {
  assert(cur_elts);
  elements[0] = x;
  heapify(0);
}




/************************************************************/
template <class T>
void pqheap_t1<T>::print() {
  cout << "[";
  for (unsigned int i=0; i<cur_elts; i++) {
    cout << elements[i].getPriority().field1() <<",";
  }
  cout << "]";
}


/************************************************************/
template <class T>
void pqheap_t1<T>::print_range() {
  cout << "[";
  T a, b;
  min(a);
  max(b);
  if (cur_elts) {
    cout << a.getPriority().field1() << ".."
	 << b.getPriority().field1();
  }
  cout << " (" << cur_elts << ")]";
}







#endif // _PQUEUE_HEAP_H 
