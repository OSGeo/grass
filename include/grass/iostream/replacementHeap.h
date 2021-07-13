
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

#ifndef REPLACEMENT_QUEUE_H
#define REPLACEMENT_QUEUE_H 

#include <assert.h>

#include "queue.h"

#define RHEAP_DEBUG if(0)



/*****************************************************************/
/* encapsulation of the element and the run it comes from;
 */
template<class T> 
class HeapElement {
public:
  T value;
  AMI_STREAM<T> *run;
  
  HeapElement(): run(NULL) {};
  
  friend ostream& operator << (ostream& s, const HeapElement &p) {
    return s << "[" << p.value << "]";
  };
};






/*****************************************************************/
/* 
This is a heap of HeapElements, i.e. elements which come from streams;
when an element is consumed, the heap knows how to replace it with the
next element from the same stream.

Compare is a class that has a member function called "compare" which
is used to compare two elements of type T
*/
template<class T,class Compare> 
class ReplacementHeap { 
private: 
  HeapElement<T>* mergeHeap;  //the heap; 
  size_t arity; //max size
  size_t size;  //represents actual size, i.e. the nb of (non-empty)
		//runs; they are stored contigously in the first
		//<size> positions of mergeHeap; once a run becomes
		//empty, it is deleted and size is decremented.  size
		//stores the next position where a HeapElement can be
		//added. 
    

protected: 
  void heapify(size_t i);
 
  void buildheap();

  /* for each run in the heap, read an element from the run into the heap */
  void init();
  
  /* add a run; make sure total nb of runs does not exceed heap arity */
  void addRun(AMI_STREAM<T> *run);
  
  /* delete the i-th run (and the element); that is, swap the ith run
     with the last one, and decrement size; just like in a heap, but
     no heapify.  Note: this function messes up the heap order. If the
     user wants to maintain heap property should call heapify
     specifically.
  */
  void deleteRun(size_t i);
  
public:
  //allocate array mergeHeap and the runs in runList
  ReplacementHeap<T,Compare>(size_t arity, queue<char*>* runList);
  
  //delete array mergeHeap 
  ~ReplacementHeap<T,Compare>();
  
  //is heap empty?
  int empty() const { 
    return (size == 0);
  }
  
  //delete mergeHeap[0].value, replace it with the next element from
  //the same stream, and re-heapify
  T extract_min();


  ostream  & print(ostream& s) const {
    char* runname;
    off_t runlen;
    s << "Replacementheap " << this << ": " << size << " runs";
    for(size_t i=0; i<size; i++) {
      s << endl << "  <-  i=" << i<< ": " <<  mergeHeap[i].run;
      assert(mergeHeap[i].run);
      mergeHeap[i].run->name(&runname);
      runlen = mergeHeap[i].run->stream_len();
      s << ", " << runname << ", len=" << runlen; 
      delete runname; //this should be safe
    }
    s << endl;
    return s;
  }

};




/*****************************************************************/
template<class T,class Compare>
ReplacementHeap<T,Compare>::ReplacementHeap(size_t g_arity, 
					    queue<char*>* runList) {
  char* name=NULL;
  
  assert(runList && g_arity > 0);

  RHEAP_DEBUG cerr << "ReplacementHeap arity=" << g_arity << "\n";  
  
  arity = g_arity;
  size = 0; //no run yet
  
  mergeHeap = new HeapElement<T>[arity];
  for (unsigned int i=0; i< arity; i++) {
    //pop a stream from the list  and add it to heap
    runList->dequeue(&name);
    AMI_STREAM<T>* str = new AMI_STREAM<T>(name);
    assert(str);
    delete name; //str makes its own copy
    addRun(str);
  }
  init();
}


/*****************************************************************/
template<class T,class Compare>
ReplacementHeap<T,Compare>::~ReplacementHeap<T,Compare>() {

  if (!empty()) {
    cerr << "warning: ~ReplacementHeap: heap not empty!\n";
  }
  //delete the runs first
  for(size_t i=0; i<size; i++) {
    if (mergeHeap[i].run) 
      delete mergeHeap[i].run;
  }
  delete [] mergeHeap;
}



/*****************************************************************/
/* add a run; make sure total nb of runs does not exceed heap arity
 */
template<class T,class Compare>
void
ReplacementHeap<T,Compare>::addRun(AMI_STREAM<T> *r) {

  assert(r);

  if(size == arity) {
    cerr << "ReplacementHeap::addRun size =" << size << ",arity=" << arity 
	 << " full, cannot add another run.\n";
    assert(0);
    exit(1);
  }
  assert(size < arity);

  mergeHeap[size].run = r;
  size++;
  
  RHEAP_DEBUG 
    {char* strname;
    r->name(&strname);
    cerr << "ReplacementHeap::addRun added run " << strname 
	 << " (rheap size=" << size << ")" << endl;
    delete strname;
    cerr.flush();
    }
}





/*****************************************************************/
/* delete the i-th run (and the value); that is, swap ith element with
   the last one, and decrement size; just like in a heap, but no
   heapify.  Note: this function messes up the heap order. If the user
   wants to maintain heap property should call heapify specifically.
 */
template<class T,class Compare>
void
ReplacementHeap<T,Compare>::deleteRun(size_t i) {

  assert(i >= 0 && i < size && mergeHeap[i].run);
  
  RHEAP_DEBUG 
    {
      cerr << "ReplacementHeap::deleteRun  deleting run " << i << ", "
	   << mergeHeap[i].run << endl;
      print(cerr);
    }


  //delete it 
  delete mergeHeap[i].run;
  //and replace it with 
  if (size > 1) { 
    mergeHeap[i].value = mergeHeap[size-1].value;
    mergeHeap[i].run = mergeHeap[size-1].run;
  }
  size--;
}




/*****************************************************************/
/* for each run in the heap, read an element from the run into the
   heap; if ith run is empty, delete it
*/
template<class T,class Compare>
void
ReplacementHeap<T,Compare>::init() {
  AMI_err err;
  T* elt;
  size_t i;

  RHEAP_DEBUG cerr << "ReplacementHeap::init " ;
  
  i=0; 
  while (i<size) {
    
    assert(mergeHeap[i].run);
    
    // Rewind run i 
    err = mergeHeap[i].run->seek(0);   
     if (err != AMI_ERROR_NO_ERROR) {
       cerr << "ReplacementHeap::Init(): cannot seek run " << i << "\n";
       assert(0);
       exit(1);
     }
     //read first item from run i
     err = mergeHeap[i].run->read_item(&elt); 
     if (err != AMI_ERROR_NO_ERROR) {
       if (err == AMI_ERROR_END_OF_STREAM) {
	 deleteRun(i);
	 //need to iterate one more time with same i; 
       } else {
	 cerr << "ReplacementHeap::Init(): cannot read run " << i << "\n";
	 assert(0);
	 exit(1);
       }
     } else {
       //copy.... can this be avoided? xxx
       mergeHeap[i].value = *elt;
       
       i++;
     }
   }
  buildheap();
}

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
static inline size_t rheap_lchild(size_t index) {
  return 2 * index;
}

static inline size_t rheap_rchild(size_t index) {
  return 2 * index + 1;
}

// The parent of an element.
static inline size_t rheap_parent(size_t index) {
  return index >> 1;
}
// this functions are duplicated in pqheap.H...XXX



/*****************************************************************/
template<class T,class Compare>
void
ReplacementHeap<T,Compare>::heapify(size_t i) {
  size_t min_index = i;
  size_t lc = rheap_lchild(i);
  size_t rc = rheap_rchild(i);

  Compare cmpobj;
  assert(i >= 0 && i < size);
  if ((lc < size) && 
      (cmpobj.compare(mergeHeap[lc].value, mergeHeap[min_index].value) == -1)) {
    min_index = lc;
  }
  if ((rc < size) && 
      (cmpobj.compare(mergeHeap[rc].value, mergeHeap[min_index].value) == -1)) {
    min_index = rc;
  }
  
  if (min_index != i) {
    HeapElement<T> tmp = mergeHeap[min_index];
    
    mergeHeap[min_index] = mergeHeap[i];
    mergeHeap[i] = tmp;
    
    
    heapify(min_index);
  }

  return;
}

/*****************************************************************/
template<class T,class Compare>
void
ReplacementHeap<T,Compare>::buildheap() {
  
  if (size > 1) {
    for (int i = rheap_parent(size-1); i>=0; i--) {
      heapify(i);
    }
  }
  RHEAP_DEBUG cerr << "Buildheap done\n";
  return;
}


/*****************************************************************/
template<class T,class Compare>
T
ReplacementHeap<T,Compare>::extract_min() {
  T *elt, min;
  AMI_err err;
  
  assert(!empty()); //user's job to check first if it's empty
  min = mergeHeap[0].value;
  

  //read a new element from the same run 
  assert(mergeHeap[0].run);
  err = mergeHeap[0].run->read_item(&elt); 
  if (err != AMI_ERROR_NO_ERROR) {
    //if run is empty, delete it
    if (err == AMI_ERROR_END_OF_STREAM) {
      RHEAP_DEBUG cerr << "rheap extract_min: run " << mergeHeap[0].run 
		       << " empty. deleting\n ";
      deleteRun(0);
    } else {
      cerr << "ReplacementHeap::extract_min: cannot read\n";
      assert(0);
      exit(1);
    }
  } else {
    //copy...can this be avoided?
    mergeHeap[0].value = *elt;
  }

  //restore heap 
  if (size > 0) heapify(0);

  return min;
}



#endif
