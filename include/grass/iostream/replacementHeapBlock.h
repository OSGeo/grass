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


#ifndef REPLACEMENT_HEAPBLOCK_H
#define REPLACEMENT_HEAPBLOCK_H

#include <assert.h>

#include "mem_stream.h"
#include "replacementHeap.h"


#define RBHEAP_DEBUG if(0)



/*****************************************************************/
/* encapsulation of the element and the run it comes from;
 */
template<class T> 
class BlockHeapElement {
public:
  T value;
  MEM_STREAM<T> *run;
  
  BlockHeapElement(): run(NULL) {};
  
  friend ostream& operator << (ostream& s, const BlockHeapElement &p) {
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
class ReplacementHeapBlock { 
private: 
  BlockHeapElement<T>* mergeHeap;  //the heap; 
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
  void addRun(MEM_STREAM<T> *run);
  
  /* delete the i-th run (and the element); that is, swap the ith run
     with the last one, and decrement size; just like in a heap, but
     no heapify.  Note: this function messes up the heap order. If the
     user wants to maintain heap property should call heapify
     specifically.
  */
  void deleteRun(size_t i);
  
public:
  //allocate array mergeHeap, where the streams are stored in runList
  ReplacementHeapBlock<T,Compare>(queue <MEM_STREAM<T>*> *runList); 
  
  //delete array mergeHeap 
  ~ReplacementHeapBlock<T,Compare>();
  
  //is heap empty?
  int empty() const { 
    return (size == 0);
  }
  
  //delete mergeHeap[0].value, replace it with the next element from
  //the same stream, and re-heapify
  T extract_min();


  ostream  & print(ostream& s) const {
	s << "ReplacementheapBlock " << this << ": " << size << " runs";
#if(0)
	char* runname;
	off_t runlen;
	for(size_t i=0; i<size; i++) {
      s << endl << "  <-  i=" << i<< ": " <<  mergeHeap[i].run;
      assert(mergeHeap[i].run);
      mergeHeap[i].run->name(&runname);
      runlen = mergeHeap[i].run->stream_len();
      s << ", " << runname << ", len=" << runlen; 
      delete runname; //this should be safe
    }
#endif
    s << endl;
    return s;
  }

};




/*****************************************************************/
//allocate array mergeHeap, where the streams are stored in runList
template<class T,class Compare>
ReplacementHeapBlock<T,Compare>
::ReplacementHeapBlock(queue <MEM_STREAM<T>*> *runList) {
  
  RBHEAP_DEBUG cerr << "ReplacementHeapBlock " << endl;
  
  arity = runList->length();

  size = 0; //no run yet
  
  MEM_STREAM<T>* str;
  mergeHeap = new BlockHeapElement<T>[arity];
  for (unsigned int i=0; i< arity; i++) {
    //pop a stream from the list  and add it to heap
    str = NULL;
    runList->dequeue(&str);
    assert(str);
    addRun(str);
  }
  init();
}


/*****************************************************************/
template<class T,class Compare>
ReplacementHeapBlock<T,Compare>::~ReplacementHeapBlock<T,Compare>() {

  if (!empty()) {
    cerr << "warning: ~ReplacementHeapBlock: heap not empty!\n";
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
ReplacementHeapBlock<T,Compare>::addRun(MEM_STREAM<T> *r) {
  
  assert(r);

  if(size == arity) {
    cerr << "ReplacementHeapBlockBlock::addRun size =" << size << ",arity=" << arity 
	 << " full, cannot add another run.\n";
    assert(0);
    exit(1);
  }
  assert(size < arity);
  
  mergeHeap[size].run = r;
  size++;
  
  RBHEAP_DEBUG 
    {char* strname;
    r->name(&strname);
    cerr << "ReplacementHeapBlock::addRun added run " << strname 
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
ReplacementHeapBlock<T,Compare>::deleteRun(size_t i) {

  assert(i >= 0 && i < size && mergeHeap[i].run);
  
  RBHEAP_DEBUG 
    {
      cerr << "ReplacementHeapBlock::deleteRun  deleting run " << i << ", "
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
ReplacementHeapBlock<T,Compare>::init() {
  AMI_err err;
  T* elt;
  size_t i;

  RBHEAP_DEBUG cerr << "ReplacementHeapBlock::init " ;
  
  i=0; 
  while (i<size) {
    
    assert(mergeHeap[i].run);
    
    // Rewind run i 
    err = mergeHeap[i].run->seek(0);   
     if (err != AMI_ERROR_NO_ERROR) {
       cerr << "ReplacementHeapBlock::Init(): cannot seek run " << i << "\n";
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
	 cerr << "ReplacementHeapBlock::Init(): cannot read run " << i << "\n";
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




/*****************************************************************/
template<class T,class Compare>
void
ReplacementHeapBlock<T,Compare>::heapify(size_t i) {
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
    BlockHeapElement<T> tmp = mergeHeap[min_index];
    
    mergeHeap[min_index] = mergeHeap[i];
    mergeHeap[i] = tmp;
    
    
    heapify(min_index);
  }

  return;
}

/*****************************************************************/
template<class T,class Compare>
void
ReplacementHeapBlock<T,Compare>::buildheap() {
  
  if (size > 1) {
    for (int i = rheap_parent(size-1); i>=0; i--) {
      heapify(i);
    }
  }
  RBHEAP_DEBUG cerr << "Buildheap done\n";
  return;
}


/*****************************************************************/
template<class T,class Compare>
T
ReplacementHeapBlock<T,Compare>::extract_min() {
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
      RBHEAP_DEBUG cerr << "rheap extract_min: run " << mergeHeap[0].run 
		       << " empty. deleting\n ";
      deleteRun(0);
    } else {
      cerr << "ReplacementHeapBlock::extract_min: cannot read\n";
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
