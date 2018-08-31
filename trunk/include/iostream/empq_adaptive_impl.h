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


#ifndef __EMPQ_ADAPTIVE_IMPL_H
#define __EMPQ_ADAPTIVE_IMPL_H

#include <stdio.h>
#include <assert.h>

#include "ami_config.h"
#include "ami_stream.h"
#include "mm.h"
#include "mm_utils.h"
#include "empq_adaptive.h"

#include "ami_sort.h"


// EMPQAD_DEBUG defined in "empqAdaptive.H"




//------------------------------------------------------------
//allocate an internal pqueue of size precisely twice 
//the size of the pqueue within the em_pqueue;
//
//This constructor uses a user defined amount of memory

template<class T, class Key> 
EMPQueueAdaptive<T,Key>::EMPQueueAdaptive(size_t inMem) {
  regim = INMEM;
  EMPQAD_DEBUG cout << "EMPQUEUEADAPTIVE: starting in-memory pqueue" 
		    << endl;
  
  //------------------------------------------------------------
  //set the size precisely as in empq constructor since we cannot 
  //really call the em__pqueue constructor, because we don't want 
  //the space allocated; we just want the sizes;
  //AMI_err ae;
  EMPQAD_DEBUG cout << "EMPQUEUEADAPTIVE: desired memory: " 
		    << ( (float)inMem/ (1<< 20)) << "MB" << endl;
  
  initPQ(inMem);
};


//------------------------------------------------------------
// This more resembles the original constuctor which is greedy
template<class T, class Key> 
EMPQueueAdaptive<T,Key>::EMPQueueAdaptive() {
  regim = INMEM;
  EMPQAD_DEBUG cout << "EMPQUEUEADAPTIVE: starting in-memory pqueue" 
		    << endl;
  
  //------------------------------------------------------------
  //set the size precisely as in empq constructor since we cannot 
  //really call the em__pqueue constructor, because we don't want 
  //the space allocated; we just want the sizes;
  size_t mm_avail = getAvailableMemory();
  EMPQAD_DEBUG cout << "EMPQUEUEADAPTIVE: available memory: " 
		    << ( (float)mm_avail/ (1<< 20)) << "MB" << endl;
  

  initPQ(mm_avail);

};


//------------------------------------------------------------
// This metod initialized the PQ based on the memory passed 
// into it
template<class T, class Key>
void
EMPQueueAdaptive<T,Key>::initPQ(size_t initMem) {
  AMI_err ae;
  EMPQAD_DEBUG cout << "EMPQUEUEADAPTIVE: desired memory: " 
		    << ( (float)initMem/ (1<< 20)) << "MB" << endl;
  
  /* same calculations as empq constructor in order to estimate
     overhead memory; this is because we want to allocate a pqueue of
     size exactly double the size of the pqueue inside the empq;
     switching from this pqueue to empq when the memory fills up will
     be simple */
  //------------------------------------------------------------
  //AMI_STREAM memory usage
  size_t sz_stream;
  AMI_STREAM<T> dummy;
  if ((ae = dummy.main_memory_usage(&sz_stream,
				    MM_STREAM_USAGE_MAXIMUM)) !=
      AMI_ERROR_NO_ERROR) {
    cerr << "EMPQueueAdaptive constr: failing to get stream_usage\n";
    exit(1);
  }  


  //account for temporary memory usage
  unsigned short max_nbuf = 2;
  unsigned int buf_arity = initMem/(2 * sz_stream);
  if (buf_arity > MAX_STREAMS_OPEN) buf_arity = MAX_STREAMS_OPEN; 
  unsigned long mm_overhead = buf_arity*sizeof(merge_key<Key>) + 
    max_nbuf * sizeof(em_buffer<T,Key>) +
    2*sz_stream + max_nbuf*sz_stream;
  mm_overhead *= 8; //overestimate..this should be fixed with
  //a precise accounting of the extra memory required

  EMPQAD_DEBUG cout << "sz_stream: " << sz_stream << " buf_arity: " << buf_arity <<
    " mm_overhead: " << mm_overhead << " mm_avail: " << initMem << ".\n";



  EMPQAD_DEBUG cout << "EMPQUEUEADAPTIVE: memory overhead set to " 
		    << ((float)mm_overhead / (1 << 20)) << "MB" << endl;
  if (mm_overhead > initMem) {
    cerr << "overhead bigger than available memory ("<< initMem << "); "
	 << "increase -m and try again\n";
    exit(1);
  }
  initMem -= mm_overhead;
  //------------------------------------------------------------


  long pqsize = initMem/sizeof(T);
  EMPQAD_DEBUG cout << "EMPQUEUEADAPTIVE: pqsize set to " << pqsize << endl;

  //initialize in memory pqueue and set em to NULL
  im = new MinMaxHeap<T>(pqsize);
  assert(im);
  em = NULL;
};


template<class T, class Key> 
EMPQueueAdaptive<T,Key>::~EMPQueueAdaptive() {
  switch(regim) {
  case INMEM:
	delete im;
	break;
  case EXTMEM:
	delete em; 
	break;
  case EXTMEM_DEBUG:
	delete dim;
	delete em; 
	break;
  }
};



//return the maximum nb of elts that can fit
template<class T, class Key> 
long 
EMPQueueAdaptive<T,Key>::maxlen() const {
  long m=-1;
  switch(regim) {
  case INMEM:
	assert(im);
	m = im->get_maxsize();
	break;
  case EXTMEM:
	assert(em);
	m = em->maxlen();
	break;
  case EXTMEM_DEBUG:
	m = em->maxlen();
	break;
  }
  return m;
};




//return true if empty
template<class T, class Key> 
bool
EMPQueueAdaptive<T,Key>::is_empty() const {
  bool v = false;
  switch(regim) {
  case INMEM:
	assert(im);
	v = im->empty();
	break;
  case EXTMEM:
	assert(em);
	v = em->is_empty(); 
	break;
  case EXTMEM_DEBUG:
	assert(dim->empty() == em->is_empty());
	v = em->is_empty(); 
	break;
  }
  return v;
};


//return true if full
template<class T, class Key> 
bool
EMPQueueAdaptive<T,Key>::is_full() const { 
  cerr << "EMPQueueAdaptive::is_full(): sorry not implemented\n";
  assert(0);
  exit(1);
}


//return the element with minimum priority in the structure
template<class T, class Key> 
bool
EMPQueueAdaptive<T,Key>::min(T& elt) {
  bool v=false, v1;
  T tmp;
  switch(regim) {
  case INMEM:
	assert(im);
	v = im->min(elt);
	break;
  case EXTMEM:
	assert(em);
	v = em->min(elt);
	break;
  case EXTMEM_DEBUG:
	v1 = dim->min(tmp);
	v = em->min(elt);
	//dim->verify();
	if(!(tmp==elt)) {
	  cerr << "------------------------------" << endl;
	  cerr << dim << endl;
	  cerr << "------------------------------" << endl;
	  em->print();
	  cerr << "------------------------------" << endl;
	  cerr << "tmp=" << tmp << endl;
	  cerr << "elt=" << elt << endl;
	  cerr << "------------------------------" << endl;
	  dim->destructiveVerify();
	}
	assert(v == v1);
	assert(tmp == elt);
	break;
  }
  return v;
};

/* switch over to using an external priority queue */
template<class T, class Key> 
void
EMPQueueAdaptive<T,Key>::clear() {
  switch(regim) {
  case INMEM:
    im->clear();
    break;
  case EXTMEM:
    em->clear();
    break;
  case EXTMEM_DEBUG:
    dim->clear();
    break;
  }
}


template<class T, class Key> 
void
EMPQueueAdaptive<T,Key>::verify() {
  switch(regim) {
  case INMEM:
	im->verify();
	break;
  case EXTMEM:
	break;
  case EXTMEM_DEBUG:
	dim->verify();
	break;
  }
}

//extract all elts with min key, add them and return their sum
template<class T, class Key> 
bool 
EMPQueueAdaptive<T,Key>::extract_all_min(T& elt) {
  bool v=false, v1;
  T tmp;
  switch(regim) {
  case INMEM:
	assert(im);
	v = im->extract_all_min(elt);
	break;
  case EXTMEM:
	assert(em);
	v = em->extract_all_min(elt);
	break;
  case EXTMEM_DEBUG:
	v1 =  dim->extract_all_min(tmp);
	v = em->extract_all_min(elt);
	assert(dim->size() == em->size());
	assert(v == v1);
	assert(tmp == elt);
	break;
  }
  return v;
};

//return the nb of elements in the structure 
template<class T, class Key> 
long
EMPQueueAdaptive<T,Key>::size() const {
  long v=0, v1;
  switch(regim) {
  case INMEM:
	assert(im);
	v = im->size();
	break;
  case EXTMEM:
	assert(em);
	v = em->size();
	break;
  case EXTMEM_DEBUG:
	v1 = dim->size();
	v = em->size();
	assert(v == v1);
	break;
  }
  return v;
}




// ----------------------------------------------------------------------
template<class T, class Key> 
bool 
EMPQueueAdaptive<T,Key>::extract_min(T& elt) {
    bool v=false, v1;
	T tmp;
	switch(regim) {
	case INMEM:
	  assert(im);
	  v = im->extract_min(elt);
	  break;
	case EXTMEM:
	  assert(em);
	  v = em->extract_min(elt);
	  break;
	case EXTMEM_DEBUG:
	  v1 =  dim->extract_min(tmp);
	  v = em->extract_min(elt);
	  assert(v == v1);
	  assert(tmp == elt);
	  assert(dim->size() == em->size());
	  break;
    }
	return v;
};
 



//------------------------------------------------------------
 /* insert an element; if regim == INMEM, try insert it in im, and if
    it is full, extract_max pqsize/2 elements of im into a stream,
    switch to EXTMEM and insert the stream into em; if regim is
    EXTMEM, insert in em; */
template<class T, class Key> 
bool 
EMPQueueAdaptive<T,Key>::insert(const T& elt) {
  bool v=false;
  switch(regim) {
  case INMEM:
	if (!im->full()) {
      im->insert(elt);
	  v = true;
    } else {
	  makeExternal();
	  v = em->insert(elt);      //insert the element
	} 
	break;
  case EXTMEM:
	v = em->insert(elt);
	break;
  case EXTMEM_DEBUG:
	dim->insert(elt);
	v = em->insert(elt);
	assert(dim->size() == em->size());
	break;
  }
  return v;
};

template<class T, class Key> 
void
EMPQueueAdaptive<T,Key>::makeExternalDebug() {
  assert(size() == 0);
  switch(regim) {
  case INMEM:
	makeExternal();
	break;
  case EXTMEM:
	break;
  case EXTMEM_DEBUG:
	assert(0);
	break;
  }
  dim = new UnboundedMinMaxHeap<T>();
  regim = EXTMEM_DEBUG;
}



template<class T>
class baseCmpType {
public:
  static int compare(const T& x, const T& y) {
    return  (x < y ? -1 : (x > y ? 1 : 0));
  }

};

/* switch over to using an external priority queue */
template<class T, class Key> 
void
EMPQueueAdaptive<T,Key>::makeExternal() {
  AMI_err ae;
#ifndef NDEBUG
  long sizeCheck;
  sizeCheck = size();
#endif

  assert(regim == INMEM);  
  regim = EXTMEM;

  EMPQAD_DEBUG cout << endl 
			 << "EMPQUEUEADAPTIVE: memory full: "
			 << "switching to external-memory pqueue " << endl;
  
  //create an AMI_stream and write in it biggest half elts of im;
  AMI_STREAM<T> *amis0 = new AMI_STREAM<T>();
  AMI_STREAM<T> *amis1 = NULL;
  assert(amis0);
  unsigned long pqsize = im->size();
  //assert(im->size() == im->get_maxsize());
  T x;
  for (unsigned long i=0; i< pqsize/2; i++) {
	int z = im->extract_max(x);
	assert(z);
	ae = amis0->write_item(x);
	assert(ae == AMI_ERROR_NO_ERROR);
  }
  assert(amis0->stream_len() == pqsize/2);
  EMPQAD_DEBUG { cout << "written " << pqsize/2 
			   << " elts to stream\n"; cout.flush(); }
  
  assert(im->size() == pqsize/2 + (pqsize % 2));
  
  EMPQAD_DEBUG LOG_avail_memo();
  
  //sort the stream
  baseCmpType<T> fun;
  AMI_sort(amis0, &amis1, &fun); //XXX laura: replaced this to use a cmp obj
  assert(amis1);
  delete amis0;
  EMPQAD_DEBUG { cout << "sorted the stream\n"; cout.flush(); }
  
  EMPQAD_DEBUG LOG_avail_memo();
  
  //set im to NULL and initialize em from im and amis1
  em = new em_pqueue<T,Key>(im, amis1);
  im = NULL;
  assert(em);
  EMPQAD_DEBUG { cout << "empq initialized from im\n"; cout.flush(); }
  EMPQAD_DEBUG {em->print_size();}
  
  EMPQAD_DEBUG LOG_avail_memo();
#ifndef NDEBUG
  assert(sizeCheck == size());
#endif
};

#endif
