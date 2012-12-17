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


#ifndef __EMPQ_IMPL_H
#define __EMPQ_IMPL_H

#include <ostream>
#include <vector>
using namespace std;

#include "empq.h"

#if(0)
#include "option.H"
#define MY_LOG_DEBUG_ID(x) \
  if(GETOPT("debug")) cerr << __FILE__ << ":" << __LINE__<< " " << x << endl;
#endif

#undef XXX
#define XXX if(0)

#define MY_LOG_DEBUG_ID(x)

/*****************************************************************/
/* encapsulation of the element=<key/prio, data> together with <buffer_id>
   and <stream_id>; used during stream merging to remember where each
   key comes from; 

   assumes that class T implements: Key getPriority()

   implements operators {<, <=, ...} such that a< b iff a.x.prio < b.x.prio
*/
template<class T,class Key> 
class ExtendedEltMergeType {

private:
  T x;
  unsigned short buf_id;
  unsigned int str_id;
  
public:
  ExtendedEltMergeType() {}
  
  ExtendedEltMergeType(T &e, unsigned short bid, unsigned int sid):
    x(e), buf_id(bid), str_id(sid) {}
  
  ~ExtendedEltMergeType() {}
  
  void set (T &e, unsigned short bid, unsigned int sid) {
    x = e;
    buf_id = bid;
    str_id = sid;
  }
  T elt() const {
    return x; 
  }
  unsigned short buffer_id() const  {
    return buf_id;
  }
  unsigned int stream_id()  const {
    return str_id;
  }
  Key getPriority() const {
    return x.getPriority();
  }
  //print
  friend ostream& operator<<(ostream& s, 
				    const ExtendedEltMergeType<T,Key> &elt) {
    return s << "<buf_id=" << elt.buf_id 
	     << ",str_id=" << elt.str_id << "> "
	     << elt.x << " ";
  }
  
  friend int operator < (const ExtendedEltMergeType<T,Key> &e1, 
				const ExtendedEltMergeType<T,Key> &e2) {
    return (e1.getPriority() < e2.getPriority());
  }
  friend int operator <= (const ExtendedEltMergeType<T,Key> &e1, 
				 const ExtendedEltMergeType<T,Key> &e2) {
    return (e1.getPriority() <= e2.getPriority());
  }
  friend int operator > (const ExtendedEltMergeType<T,Key> &e1, 
				const ExtendedEltMergeType<T,Key> &e2) {
    return (e1.getPriority() > e2.getPriority());
  }
  friend int operator >= (const ExtendedEltMergeType<T,Key> &e1, 
				 const ExtendedEltMergeType<T,Key> &e2) {
    return (e1.getPriority() >= e2.getPriority());
  }
  friend int operator != (const ExtendedEltMergeType<T,Key> &e1, 
				 const ExtendedEltMergeType<T,Key> &e2) {
    return (e1.getPriority() != e2.getPriority());
  }
  friend int operator == (const ExtendedEltMergeType<T,Key> &e1, 
				 const ExtendedEltMergeType<T,Key> &e2) {
    return (e1.getPriority() == e2.getPriority());
  }

};



//************************************************************/
//create an em_pqueue
template<class T, class Key>
 em_pqueue<T,Key>::em_pqueue(long pq_sz, long buf_sz , 
				    unsigned short nb_buf, 
				    unsigned int buf_ar):
  pqsize(pq_sz), bufsize(buf_sz), max_nbuf(nb_buf), 
  crt_buf(0), buf_arity(buf_ar) {
  
  //____________________________________________________________
  //ESTIMATE AVAILABLE MEMORY BEFORE ALLOCATION
  AMI_err ae;
  size_t mm_avail = getAvailableMemory();
  printf("EM_PQUEUE:available memory before allocation: %.2fMB\n",
 	       mm_avail/(float)(1<<20));
  printf("EM_PQUEUE:available memory before allocation: %ldB\n",
		 mm_avail);


  //____________________________________________________________
  //ALLOCATE STRUCTURE
   //some dummy checks..
  assert(pqsize > 0 && bufsize > 0);

  MEMORY_LOG("em_pqueue: allocating int pqueue\n");
  //initialize in memory pqueue
  pq = new MinMaxHeap<T>(pqsize);
  assert(pq);

  MEMORY_LOG("em_pqueue: allocating buff_0\n");
  //initialize in memory buffer
  buff_0 = new im_buffer<T>(bufsize);
  assert(buff_0);

  char str[200];
  sprintf(str, "em_pqueue: allocating array of %ld buff pointers\n",
		  (long)max_nbuf);
  MEMORY_LOG(str);
  
  //allocate ext memory buffers array
  buff = new em_buffer<T,Key>* [max_nbuf];
  assert(buff);
  for (unsigned short i=0; i<max_nbuf; i++) {
    buff[i] = NULL;
  }


  //____________________________________________________________
  //some memory checks- make sure the empq fits in memory !!

  //estimate available memory after allocation 
  mm_avail = getAvailableMemory();
  printf("EM_PQUEUE: available memory after allocation: %.2fMB\n", 
	       mm_avail/(float)(1<<20));
  
  //estimate AMI_STREAM memory usage
  size_t sz_stream;
  AMI_STREAM<T> dummy;
  if ((ae = dummy.main_memory_usage(&sz_stream,
				    MM_STREAM_USAGE_MAXIMUM)) !=
      AMI_ERROR_NO_ERROR) {
    cout << "em_pqueue constructor: failing to get stream_usage\n";
    exit(1);
  }  
  cout << "EM_PQUEUE:AMI_stream memory usage: " << sz_stream << endl;
  cout << "EM_PQUEUE: item size=" << sizeof(T) << endl;
  
  //estimate memory overhead
  long mm_overhead = buf_arity*sizeof(merge_key<Key>) + 
    max_nbuf * sizeof(em_buffer<T,Key>) +
    2*sz_stream + max_nbuf*sz_stream;
  
  mm_overhead *= 8; //overestimate
  cout << "EM_PQUEUE: mm_overhead estimated as " << mm_overhead << endl;
  if (mm_overhead > mm_avail) {
    cout << "overhead bigger than available memory"
	 << "increase -m and try again\n";
    exit(1);
  }
  mm_avail -= mm_overhead;
  

  //arity*sizeof(AMI_STREAM) < memory
  cout << "pqsize=" << pqsize
       << ", bufsize=" << bufsize
       << ", maximum allowed arity=" << mm_avail/sz_stream << endl;
  if (buf_arity * sz_stream > mm_avail) {
    cout << "sorry - empq excedes memory limits\n";
    cout << "try again decreasing arity or pqsize/bufsize\n";
    cout.flush();
  }
}


//************************************************************/
//create an em_pqueue capable to store <= N elements
template<class T, class Key>
em_pqueue<T,Key>::em_pqueue() {
  
  MY_LOG_DEBUG_ID("em_pqueue constructor");


  /************************************************************/
  //available memory 
  AMI_err ae;
  //available memory
  size_t mm_avail = getAvailableMemory();
  printf("EM_PQUEUE:available memory before allocation: %.2fMB\n", 
	       mm_avail/(float)(1<<20));
  cout.flush();

  //AMI_STREAM memory usage
  size_t sz_stream;
  AMI_STREAM<T> dummy;
  if ((ae = dummy.main_memory_usage(&sz_stream,
				    MM_STREAM_USAGE_MAXIMUM)) !=
      AMI_ERROR_NO_ERROR) {
    cout << "em_pqueue constructor: failing to get main_memory_usage\n";
    exit(1);
  }  
  cout << "EM_PQUEUE:AMI_stream memory usage: " << sz_stream << endl;
  cout << "EM_PQUEUE: item size=" << sizeof(T) << endl;
  cout.flush();
  //assume max_nbuf=2 suffices; check after arity is computed
  max_nbuf = 2;

  //account for temporary memory usage (set up a preliminary arity)
  buf_arity = mm_avail/(2 * sz_stream);
  long mm_overhead = buf_arity*sizeof(merge_key<Key>) + 
    max_nbuf * sizeof(em_buffer<T,Key>) +
    2*sz_stream + max_nbuf*sz_stream;
  
  mm_overhead *= 8; //overestimate
  cout << "EM_PQUEUE: mm_overhead estimated as " << mm_overhead << endl;
  if (mm_overhead > mm_avail) {
    cout << "overhead bigger than available memory"
	 << "increase -m and try again\n";
    exit(1);
  }
  mm_avail -= mm_overhead;
  
  
#ifdef SAVE_MEMORY
  //assign M/2 to pq
  pqsize = mm_avail/(2*sizeof(T));
  //assign M/2 to buff_0
  bufsize = mm_avail/(2*sizeof(T));
#else 
  //assign M/4 to pq
  pqsize = mm_avail/(4*sizeof(T));
  //assign M/4 to buff_0
  bufsize = mm_avail/(4*sizeof(T));
#endif
  
  cout << "EM_PQUEUE: pqsize set to " << pqsize << endl;
  cout << "EM_PQUEUE: bufsize set to " << bufsize << endl; 
  cout << "EM_PQUEUE: nb buffers set to " << max_nbuf << endl; 
 
  
  //assign M/2 to AMI_STREAMS and compute arity
  /* arity is mainly constrained by the size of an AMI_STREAM; the
     rest of the memory must accomodate for arity * max_nbuf
     *sizeof(AMI_STREAM); there are some temporary stuff like arity *
     sizeof(long) (the deleted array), arity * sizeof(T) (the array of
     keys for merging) and so on, but the main factor is the
     AMI_STREAM size which is roughly B * LBS * 2 (each AMI_STREAM
     allocates 2 logical blocks) */
#ifdef SAVE_MEMORY
 buf_arity = mm_avail/(2 * sz_stream);
#else
 buf_arity = mm_avail/(2 * max_nbuf * sz_stream); 
#endif

  //overestimate usage
  if (buf_arity > 3) {
    buf_arity -= 3;
  } else {
    buf_arity = 1;
  }

  cout << "EM_PQUEUE: arity set to " << buf_arity << endl;
  
  crt_buf = 0; 
  
  //initialize in memory pqueue
  MEMORY_LOG("em_pqueue: allocating int pqueue\n");
  pq = new MinMaxHeap<T>(pqsize);
  assert(pq);
  
  //initialize in memory buffer
  MEMORY_LOG("em_pqueue: allocating buff_0\n");
  buff_0 = new im_buffer<T>(bufsize);
  assert(buff_0);
  
  //allocate ext memory buffers array
  char str[200];
  sprintf(str,"em_pqueue: allocating array of %ld buff pointers\n",
	  (long)max_nbuf);
  MEMORY_LOG(str);
  //allocate ext memory buffers array
  buff = new em_buffer<T,Key>* [max_nbuf];
  assert(buff);
  for (unsigned short i=0; i<max_nbuf; i++) {
    buff[i] = NULL;
  }

  //max nb of items the structure can accomodate (constrained by max_nbuf)
  cout << "EM_PQUEUE: maximum length is " << maxlen() << "\n";
  cout.flush(); 
  
  //check that structure can accomodate N elements
  //  assert(N < buf_arity * (buf_arity + 1) * bufsize);
  //assert(N < maxlen());  
  mm_avail = getAvailableMemory();
  printf("EM_PQUEUE: available memory after allocation: %.2fMB\n", 
	       mm_avail/(float)(1<<20));
}


#ifdef SAVE_MEMORY
//************************************************************/
// create an empq, initialize its pq with im and insert amis in
// buff[0]; im should not be used/deleted after that outside empq;
//
// assumption: im was allocated such that maxsize = mm_avail/T;
// when this constructor is called im is only half full, so we must 
// free half of its space and  give to buff_0
template<class T, class Key>
em_pqueue<T,Key>::em_pqueue(MinMaxHeap<T> *im, AMI_STREAM<T> *amis) {
  AMI_err ae;
  int pqcapacity;	   /* amount of memory we can use for each of new
						  minmaxheap, and em-buffer */ 
  unsigned int pqcurrentsize;   /* number of elements currently in im */
  assert(im && amis);

  pqcapacity = im->get_maxsize()/2; // we think this memory is now available
  pqsize = pqcapacity + 1; //truncate errors  
  pqcurrentsize = im->size();
  //assert( pqcurrentsize <= pqsize); 
  if(!(pqcurrentsize <= pqsize)) {
    cout << "EMPQ: pq maxsize=" << pqsize <<", pq crtsize=" << pqcurrentsize
	 << "\n";
    assert(0);
    exit(1);
  }
  
  
  LOG_avail_memo();
  
  /* at this point im is allocated all memory, but it is only at most
     half full; we need to relocate im to half space and to allocate
     buff_0 the other half; since we use new, there is no realloc, so
     we will copy to a file...*/
  
  {
    //copy im to a stream and free its memory
    T x;
    AMI_STREAM<T> tmpstr;
    for (unsigned int i=0; i<pqcurrentsize; i++) {
      im->extract_min(x);
      ae = tmpstr.write_item(x);
      assert(ae == AMI_ERROR_NO_ERROR);
    }
    delete im; im = NULL;
    LOG_avail_memo();
    
    //allocate pq and buff_0 half size
    bufsize = pqcapacity;
    cout << "EM_PQUEUE: allocating im_buffer size=" << bufsize
	 << " total " << (float)bufsize*sizeof(T)/(1<<20) << "MB\n"; 
    cout.flush();
    buff_0 = new im_buffer<T>(bufsize);
    assert(buff_0);
    cout << "EM_PQUEUE: allocating pq size=" << pqsize
	 << " total " << (float)pqcapacity*sizeof(T)/(1<<20)  << "MB\n"; 
    cout.flush();
    pq = new MinMaxHeap<T>(pqsize);
    assert(pq);
    
    //fill pq from tmp stream
    ae =  tmpstr.seek(0);
    assert(ae == AMI_ERROR_NO_ERROR);
    T *elt;
    for (unsigned int i=0; i<pqcurrentsize; i++) {
      ae = tmpstr.read_item(&elt);
      assert(ae == AMI_ERROR_NO_ERROR);
      pq->insert(*elt);
    }
    assert(pq->size() == pqcurrentsize);
  }

  //estimate buf_arity
  //AMI_STREAM memory usage
  size_t sz_stream;
  AMI_STREAM<T> dummy;
  if ((ae = dummy.main_memory_usage(&sz_stream,
				    MM_STREAM_USAGE_MAXIMUM)) !=
      AMI_ERROR_NO_ERROR) {
    cout << "em_pqueue constructor: failing to get main_memory_usage\n";
    exit(1);
  }  
  cout << "EM_PQUEUE: AMI_stream memory usage: " << sz_stream << endl;
  cout << "EM_PQUEUE: item size=" << sizeof(T) << endl;
  //assume max_nbuf=2 suffices; check after arity is computed
  max_nbuf = 2;
  buf_arity = pqcapacity * sizeof(T) / sz_stream;
  //should account for some overhead
  if (buf_arity == 0) {
    cout << "EM_PQUEUE: arity=0 (not enough memory..)\n";
    exit(1);
  }
   if (buf_arity > 3) {
     buf_arity -= 3;
   } else {
     buf_arity = 1;
   }

   //added on 05/16/2005 by Laura
   if (buf_arity > MAX_STREAMS_OPEN) {
	 buf_arity = MAX_STREAMS_OPEN;
   }

  //allocate ext memory buffer array
  char str[200];
  sprintf(str,"em_pqueue: allocating array of %ld buff pointers\n",
	  (long)max_nbuf);
  MEMORY_LOG(str);
  buff = new em_buffer<T,Key>* [max_nbuf];
  assert(buff);
  for (unsigned short i=0; i<max_nbuf; i++) {
    buff[i] = NULL;
  }    
  crt_buf = 0;

  cout << "EM_PQUEUE: new pqsize set to " << pqcapacity << endl;
  cout << "EM_PQUEUE: bufsize set to " << bufsize << endl; 
  cout << "EM_PQUEUE: buf arity set to " << buf_arity << endl;
  cout << "EM_PQUEUE: nb buffers set to " << max_nbuf << endl; 
  cout << "EM_PQUEUE: maximum length is " << maxlen() << "\n";
  cout.flush(); 

  //estimate available remaining memory 
  size_t mm_avail = getAvailableMemory();
  printf("EM_PQUEUE: available memory after allocation: %.2fMB\n", 
	 mm_avail/(float)(1<<20));
  
  //last thing: insert the input stream in external buffers
  //allocate buffer if necessary
  //assert(crt_buf==0 && !buff[0]);// given
  if(amis->stream_len()) {
	//create buff[0] as a level1 buffer
    MEMORY_LOG("em_pqueue::empty_buff_0: create new em_buffer\n");
    buff[0] = new em_buffer<T,Key>(1, bufsize, buf_arity); 
	buff[0]->insert(amis);
	crt_buf = 1;
  }
}

#endif



//************************************************************/
//free space  
template<class T, class Key>
em_pqueue<T,Key>::~em_pqueue() {
  //delete in memory pqueue
  if (pq) {
	delete pq; pq = NULL;
  }
  //delete in memory buffer
  if (buff_0) {
	delete buff_0; buff_0 = NULL;
  }
  //delete ext memory buffers
  for (unsigned short i=0; i< crt_buf; i++) {
    if (buff[i]) delete buff[i];
  }
  delete [] buff;
}


//************************************************************/
//return maximum capacity of i-th external buffer
template<class T, class Key>
long  em_pqueue<T,Key>::maxlen(unsigned short i) {
  
  if (i >= max_nbuf) {
    printf("em_pqueue::max_len: level=%d exceeds capacity=%d\n", 
	   i, max_nbuf);
    return 0;
  }
  if (i < crt_buf) {
    return buff[i]->get_buf_maxlen();
  }
  //try allocating buffer
  em_buffer<T,Key> * tmp = new em_buffer<T,Key>(i+1, bufsize, buf_arity);
  if (!tmp) {
    cout << "em_pqueue::max_len: cannot allocate\n";
    return 0;
  }
  long len = tmp->get_buf_maxlen();
  delete tmp;
  return len;
}


//************************************************************/
//return maximum capacity of em_pqueue
template<class T, class Key>
long em_pqueue<T,Key>::maxlen() {
  long len = 0;
  for (unsigned short i=0; i< max_nbuf; i++) {
    len += maxlen(i);
  }
  return len + buff_0->get_buf_maxlen();
}



//************************************************************/
//return the total nb of elements in the structure 
template<class T, class Key>
unsigned long em_pqueue<T,Key>::size() {
  //sum up the lenghts(nb of elements) of the external buffers 
  unsigned long elen = 0;
  for (unsigned short i=0; i < crt_buf; i++) {
    elen += buff[i]->get_buf_len();
  }
  return elen + pq->size() + buff_0->get_buf_len();
}


//************************************************************/
//return true if empty
template<class T, class Key>
bool em_pqueue<T,Key>::is_empty() {
  
  //return (size() == 0);
  //more efficient?
  return ((pq->size() == 0) && (buff_0->get_buf_len() == 0) && 
	  (size() == 0));
}


//************************************************************/
//called when pq must be filled from external buffers
template<class T, class Key>
bool em_pqueue<T,Key>::fillpq() {
  
#ifndef NDEBUG
  {
	int k=0;
	for (unsigned short i=0; i<crt_buf; i++) {
	  k |= buff[i]->get_buf_len();
	}
	if(!k) {
	  cerr << "fillpq called with empty external buff!" << endl;
	}
	assert(k);
  }
#endif

#ifdef EMPQ_PQ_FILL_PRINT
  cout << "filling pq\n"; cout .flush();
#endif
  XXX cerr << "filling pq" << endl; 
  MY_LOG_DEBUG_ID("fillpq");

  AMI_err ae;
  {
	char str[200];
	sprintf(str, "em_pqueue::fillpq: allocate array of %hd AMI_STREAMs\n",
			crt_buf);
	MEMORY_LOG(str);
  }
  //merge pqsize smallest elements from each buffer into a new stream
  ExtendedMergeStream** outstreams;
  outstreams = new ExtendedMergeStream* [crt_buf];

  /* gets stream of smallest pqsize elts from each level */
  for (unsigned short i=0; i< crt_buf; i++) {
    MY_LOG_DEBUG_ID(crt_buf);
	outstreams[i] = new ExtendedMergeStream();
	assert(buff[i]->get_buf_len());
    ae = merge_buffer(buff[i], outstreams[i], pqsize);
    assert(ae == AMI_ERROR_NO_ERROR);
	assert(outstreams[i]->stream_len());
    //print_stream(outstreams[i]); cout.flush();
  }
  
  /* merge above streams into pqsize elts in minstream */
  if (crt_buf == 1) {
    //just one level; make common case faster :)
    merge_bufs2pq(outstreams[0]);
	delete outstreams[0];
	delete [] outstreams;
  } else {
    //merge the outstreams to get the global mins and delete them afterwards
    ExtendedMergeStream *minstream = new ExtendedMergeStream();
    //cout << "merging streams\n";
    ae = merge_streams(outstreams, crt_buf, minstream, pqsize);
    assert(ae == AMI_ERROR_NO_ERROR);
	for (int i=0; i< crt_buf; i++) {
	  delete outstreams[i];
	}
    delete [] outstreams;

    //copy the minstream in the internal pqueue while merging with buff_0;
    //the smallest <pqsize> elements between minstream and buff_0 will be
    //inserted in internal pqueue;
    //also, the elements from minstram which are inserted in pqueue must be
    //marked as deleted in the source streams;
    merge_bufs2pq(minstream);
	delete minstream; minstream = NULL;
    //cout << "after merge_bufs2pq: \n" << *this << "\n";
  }

  XXX assert(pq->size());
  XXX cerr << "fillpq done" << endl;
  return true;
}



//************************************************************/
//return the element with minimum priority in the structure
template<class T, class Key>
bool 
em_pqueue<T,Key>::min(T& elt){
  
  bool ok;

  MY_LOG_DEBUG_ID("em_pqueue::min");

  //try first the internal pqueue
  if (!pq->empty()) {
    ok = pq->min(elt);
    assert(ok);
    return ok;
  }

  MY_LOG_DEBUG_ID("extract_min: reset pq");
  pq->reset();

  //if external buffers are empty
  if (crt_buf == 0) {
    //if internal buffer is empty too, then nothing to extract
    if (buff_0->is_empty()) {
      //cerr << "min called on empty empq" << endl;
      return false;
    } else {
#ifdef EMPQ_PRINT_FILLPQ_FROM_BUFF0
      cout << "filling pq from B0\n"; cout.flush();
#endif
      //ext. buffs empty; fill int pqueue from buff_0 
      long n = pq->fill(buff_0->get_array(), buff_0->get_buf_len());
      buff_0->reset(pqsize, n);
      ok = pq->min(elt);
      assert(ok); 
      return true;
    }
  } else {
    //external buffers are not empty;
    //called when pq must be filled from external buffers
	XXX print_size();
    fillpq();
    XXX cerr << "fillpq done; about to take min" << endl;
    ok = pq->min(elt);
    XXX cerr << "after taking min" << endl;
    assert(ok);
    return ok;
  } //end of else (if ext buffers are not empty)
  
  assert(0); //not reached
}



//************************************************************/
template<class T,class Key>
static void print_ExtendedMergeStream(ExtendedMergeStream &str) {
  
  ExtendedEltMergeType<T,Key> *x;
  str.seek(0);
  while (str.read_item(&x) == AMI_ERROR_NO_ERROR) {
    cout << *x << ", ";
  }
  cout << "\n";
}


//************************************************************/
//delete the element with minimum priority in the structure;
//return false if pq is empty
template<class T, class Key>
bool em_pqueue<T,Key>::extract_min(T& elt) {

  bool ok;

  MY_LOG_DEBUG_ID("\n\nEM_PQUEUE::EXTRACT_MIN");
  
  //try first the internal pqueue
  if (!pq->empty()) {
    //cout << "extract from internal pq\n";
    MY_LOG_DEBUG_ID("extract from internal pq");
    ok = pq->extract_min(elt);
    assert(ok);
    return ok;
  }

  //if internal pqueue is empty, fill it up
  MY_LOG_DEBUG_ID("internal pq empty: filling it up from external buffers");
  MY_LOG_DEBUG_ID("extract_min: reset pq");
  pq->reset();

  //if external buffers are empty
  if (crt_buf == 0) {
    //if internal buffer is empty too, then nothing to extract
    if (buff_0->is_empty()) {
      return false;
    } else {
#ifdef EMPQ_PRINT_FILLPQ_FROM_BUFF0
      cout << "filling pq from B0\n"; cout.flush();
#endif
      MY_LOG_DEBUG_ID("filling pq from buff_0");
      //ext. buffs empty; fill int pqueue from buff_0 
      long n = pq->fill(buff_0->get_array(), buff_0->get_buf_len());
      buff_0->reset(pqsize, n);
      ok = pq->extract_min(elt);
      assert(ok); 
      return true;
    }
  } else {
    //external buffers are not empty;
    //called when pq must be filled from external buffers
    MY_LOG_DEBUG_ID("filling pq from buffers");
#ifdef EMPQ_PRINT_SIZE
    long x = size(), y;
    y = x*sizeof(T) >> 20;
    cout << "pqsize:[" << active_streams() << " streams: ";
    print_stream_sizes();
    cout << " total " << x << "(" << y << "MB)]" << endl;
    cout.flush();
#endif
    fillpq();
    MY_LOG_DEBUG_ID("pq filled");
    XXX cerr << "about to get the min" << endl;
    assert(pq);
    ok = pq->extract_min(elt);
    if (!ok) {
      cout << "failing assertion: pq->extract_min == true\n";
      this->print();
      assert(ok);
    }

    return ok;
  } //end of else (if ext buffers are not empty)
  
  assert(0); //not reached
}



//************************************************************/
//extract all elts with min key, add them and return their sum
//delete the element with minimum priority in the structure;
//return false if pq is empty
template<class T, class Key>
bool em_pqueue<T,Key>::extract_all_min(T& elt) {
  //cout << "em_pqueue::extract_min_all(T): sorry not implemented\n";
  //exit(1);

  T next_elt;
  bool done = false;
  
  MY_LOG_DEBUG_ID("\n\nEM_PQUEUE::EXTRACT_ALL_MIN");

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
	
	MY_LOG_DEBUG_ID("EXTRACT_ALL_MIN: adding " );
	MY_LOG_DEBUG_ID(elt);
      }
    }
  }
  
#ifdef EMPQ_PRINT_EXTRACTALL
  cout << "EXTRACTED: " << elt << endl; cout.flush();
#endif
#ifdef EMPQ_PRINT_EMPQ
  this->print();
  cout << endl;
#endif
  return true;
  
}




//************************************************************/
//copy the minstream in the internal pqueue while merging with buff_0;
//the smallest <pqsize> elements between minstream and buff_0 will be
//inserted in internal pqueue;
//also, the elements from minstram which are inserted in pqueue must be
//marked as deleted in the source streams;
template<class T, class Key>
void em_pqueue<T,Key>::merge_bufs2pq(ExtendedMergeStream *minstream) {

  //cout << "bufs2pq: \nminstream: "; print_stream(minstream);
  MY_LOG_DEBUG_ID("merge_bufs2pq: enter");

  AMI_err ae;

  //sort the internal buffer
  buff_0->sort();
  //cout << "bufs2pq: \nbuff0: " << *buff_0 << endl;

  ae = minstream->seek(0);   //rewind minstream
  assert(ae == AMI_ERROR_NO_ERROR);

  bool strEmpty= false, bufEmpty=false;
  
  unsigned int bufPos = 0;
  ExtendedEltMergeType<T,Key> *strItem;
  T bufElt, strElt;

  ae = minstream->read_item(&strItem);
  if (ae == AMI_ERROR_END_OF_STREAM) {
    strEmpty = true;
  } else {
    assert(ae == AMI_ERROR_NO_ERROR);
  }
  if (bufPos < buff_0->get_buf_len()) {
    bufElt = buff_0->get_item(bufPos);
  } else {
    //cout << "buff0 empty\n";
    bufEmpty = true;
  }

  XXX cerr << "pqsize=" << pqsize << endl;
  XXX if(strEmpty) cerr << "stream is empty!!" << endl;
  for (unsigned int i=0; i< pqsize; i++) {

    if (!bufEmpty) {
      if ((!strEmpty) && (strElt = strItem->elt(), 
			  bufElt.getPriority() > strElt.getPriority())) {
	delete_str_elt(strItem->buffer_id(), strItem->stream_id());
	pq->insert(strElt);
	ae = minstream->read_item(&strItem);
	if (ae == AMI_ERROR_END_OF_STREAM) {
	  strEmpty = true;
	} else {
	  assert(ae == AMI_ERROR_NO_ERROR);
	}
	
      } else {
	bufPos++;
	pq->insert(bufElt);
	if (bufPos < buff_0->get_buf_len()) {
	  bufElt = buff_0->get_item(bufPos);
	} else {
	  bufEmpty = true;
	}
      } 
    } else {
      if (!strEmpty) { //stream not empty
	strElt = strItem->elt();
	//cout << "i=" << i << "str & buff empty\n";
	delete_str_elt(strItem->buffer_id(), strItem->stream_id());
	pq->insert(strElt);
	//cout << "insert " << strElt << "\n";
	ae = minstream->read_item(&strItem);
	if (ae == AMI_ERROR_END_OF_STREAM) {
	  strEmpty = true;
	} else {
	  assert(ae == AMI_ERROR_NO_ERROR);
	}
      } else { //both empty: < pqsize items
	break;
      }
    }
  }
  
  //shift left buff_0 in case elements were deleted from the beginning 
  buff_0->shift_left(bufPos);
  
  MY_LOG_DEBUG_ID("pq filled");
#ifdef EMPQ_PQ_FILL_PRINT
  cout << "merge_bufs2pq: pq filled; now cleaning\n"; cout .flush();
#endif
 //this->print();
  //clean buffers in case some streams have been emptied
  cleanup();
  
  MY_LOG_DEBUG_ID("merge_bufs2pq: done");
}



//************************************************************/
//deletes one element from <buffer, stream>
template<class T, class Key>
void em_pqueue<T,Key>::delete_str_elt(unsigned short buf_id,
					     unsigned int stream_id) {
  
  //check them
  assert(buf_id < crt_buf);
  assert(stream_id < buff[buf_id]->get_nbstreams());
  //update; 
  buff[buf_id]->incr_deleted(stream_id);
  
}


//************************************************************/
//clean buffers in case some streams have been emptied
template<class T, class Key>
void em_pqueue<T,Key>::cleanup() {

  MY_LOG_DEBUG_ID("em_pqueue::cleanup()");
#ifdef EMPQ_PQ_FILL_PRINT
  cout << "em_pqueue: cleanup enter\n"; cout .flush();
#endif
  //adjust buffers in case whole streams got deleted
  for (unsigned short i=0; i< crt_buf; i++) {
    //cout << "clean buffer " << i << ": "; cout.flush();
    buff[i]->cleanup();
  }
  if (crt_buf) {
    short i = crt_buf-1;
    while ((i>=0) && buff[i]->is_empty()) {
      crt_buf--;
      i--;
    }
  }
#ifdef EMPQ_PQ_FILL_PRINT
  cout << "em_pqueue: cleanup done\n"; cout .flush();
#endif
  //if a stream becomes too short move it on previous level
  //to be added..
  //cout <<"done cleaning up\n";
}


//************************************************************/
//insert an element; return false if insertion fails
template<class T, class Key>
bool em_pqueue<T,Key>::insert(const T& x) {
  bool  ok;
#ifdef EMPQ_ASSERT_EXPENSIVE
  long init_size = size();
#endif
  T val = x;

  MY_LOG_DEBUG_ID("\nEM_PQUEUE::INSERT");
  //if structure is empty insert x in pq; not worth the trouble..
  if ((crt_buf == 0) && (buff_0->is_empty())) {
    if (!pq->full()) {
      MY_LOG_DEBUG_ID("insert in pq");
	  pq->insert(x);
	  return true;
    }
  }
  if (!pq->empty()) {
    T pqmax;
    bool ok;
    ok = pq->max(pqmax);
    assert(ok);
    // cout << "insert " << x << " max: " << pqmax << "\n";
    if (x <= pqmax) {
      //if x is smaller then pq_max and pq not full, insert x in pq   
      if (!pq->full()) {
#ifdef EMPQ_ASSERT_EXPENSIVE
		assert(size() == init_size);
#endif
		pq->insert(x);
		return true;
      } else {
		//if x is smaller then pq_max and pq full, exchange x with pq_max
		pq->extract_max(val);
		pq->insert(x);
		//cout << "max is: " << val << endl;
      }
    }
  }
  /* at this point, x >= pqmax.
	 we need to insert val==x or val==old max.
  */

  //if buff_0 full, empty it 
#ifdef EMPQ_ASSERT_EXPENSIVE
  assert(size() == init_size);
#endif
  if (buff_0->is_full()) { 
#ifdef EMPQ_PRINT_SIZE
    long x = size(), y;
    y = x*sizeof(T) >> 20;
    cout << "pqsize:[" << active_streams() << " streams: ";
    print_stream_sizes();
    cout << " total " << x << "(" << y << "MB)]" << endl;
    cout.flush();
#endif
    empty_buff_0();
  }  
#ifdef EMPQ_ASSERT_EXPENSIVE
  assert(size() == init_size);
#endif
  //insert x in buff_0
  assert(!buff_0->is_full());
  MY_LOG_DEBUG_ID("insert in buff_0");
  ok = buff_0->insert(val);
  assert(ok);
  
#ifdef EMPQ_PRINT_INSERT
  cout << "INSERTED: " << x << endl; cout.flush();
#endif
#ifdef EMPQ_PRINT_EMPQ
  this->print();
  cout << endl;
#endif
  MY_LOG_DEBUG_ID("EM_PQUEUE: INSERTED");
  return true;
}


//************************************************************/
/* called when buff_0 is full to empty it on external level_1 buffer;
   can produce cascading emptying 
*/
template<class T, class Key> bool
em_pqueue<T,Key>::empty_buff_0() {
#ifdef EMPQ_ASSERT_EXPENSIVE 
  long init_size = size();
#endif

#ifdef EMPQ_EMPTY_BUF_PRINT  
  cout << "emptying buff_0\n"; cout.flush();
  print_size();
#endif
  MY_LOG_DEBUG_ID("empty buff 0");

  assert(buff_0->is_full());
  
  //sort the buffer
  buff_0->sort();
  //cout << "sorted buff_0: \n" << *buff_0 << "\n";
#ifdef EMPQ_ASSERT_EXPENSIVE 
  assert(size() == init_size);
#endif
  //allocate buffer if necessary
  if (!buff[0]) {				// XXX should check crt_buf
    //create buff[0] as a level1 buffer
    MEMORY_LOG("em_pqueue::empty_buff_0: create new em_buffer\n");
    buff[0] = new em_buffer<T,Key>(1, bufsize, buf_arity); 
  }
  //check that buff_0 fills exactly a stream of buff[0] 
  assert(buff_0->get_buf_len() == buff[0]->get_stream_maxlen());

  //save buff_0 to stream
  MY_LOG_DEBUG_ID("empty buff_0 to stream");
  AMI_STREAM<T>* buff_0_str = buff_0->save2str();
  assert(buff_0_str);
  //MY_LOG_DEBUG_ID("buff_0 emptied");

  //reset buff_0
  buff_0->reset();
  MY_LOG_DEBUG_ID("buf_0 now reset");

#ifdef EMPQ_ASSERT_EXPENSIVE 
 assert(size() + buff_0->maxlen() == init_size);
#endif 

 //copy data from buff_0 to buff[0]
 if (buff[0]->is_full()) {
   //if buff[0] full, empty it recursively
   empty_buff(0);
  }
 buff[0]->insert(buff_0_str);
 MY_LOG_DEBUG_ID("stream inserted in buff[0]");
   
 //update the crt_buf pointer if necessary
 if (crt_buf == 0) crt_buf = 1;
 
#ifdef EMPQ_ASSERT_EXPENSIVE 
   assert(size() == init_size);
#endif  

 return true;
}


//************************************************************/
/* sort and empty buff[i] into buffer[i+1] recursively; called
   by empty_buff_0() to empty subsequent buffers; i must
   be a valid (i<crt_buf) full buffer;
*/
template<class T, class Key>
void 
em_pqueue<T,Key>::empty_buff(unsigned short i) {

#ifdef EMPQ_ASSERT_EXPENSIVE 
  long init_size = size();
#endif
#ifdef EMPQ_EMPTY_BUF_PRINT  
  cout << "emptying buffer_" << i << "\n";  cout.flush();
  print_size();
#endif
  MY_LOG_DEBUG_ID("empty buff ");
  MY_LOG_DEBUG_ID(i);

  //i must be a valid, full buffer
  assert(i<crt_buf);
  assert(buff[i]->is_full());
  
  //check there is space to empty to
  if (i == max_nbuf-1) {
    cerr << "empty_buff:: cannot empty further - structure is full..\n";
    print_size();
    cerr << "ext buff array should reallocate in a future version..\n";
    exit(1);
  }

  //create next buffer if necessary
  if (!buff[i+1]) {
    //create buff[i+1] as a level-(i+2) buffer
    char str[200];
    sprintf(str, "em_pqueue::empty_buff( %hd ) allocate new em_buffer\n", i);
    MEMORY_LOG(str);
    buff[i+1] = new em_buffer<T,Key>(i+2, bufsize, buf_arity); 
  }
  assert(buff[i+1]);
  //check that buff[i] fills exactly a stream of buff[i+1];
  //extraneous (its checked in insert)
  //assert(buff[i]->len() == buff[i+1]->streamlen());

  //sort the buffer into a new stream
  MY_LOG_DEBUG_ID("sort buffer ");
  AMI_STREAM<T>* sorted_buf = buff[i]->sort();
  
  //assert(sorted_buf->stream_len() == buff[i]->len());
  //this is just for debugging
  if (sorted_buf->stream_len() != buff[i]->get_buf_len()) {
    cout << "sorted_stream_len: " << sorted_buf->stream_len()
	 << " , bufflen: " << buff[i]->get_buf_len() << endl;  cout.flush();
    AMI_err ae;
    ae = sorted_buf->seek(0);
    assert(ae == AMI_ERROR_NO_ERROR);
    T *x;
    while (sorted_buf->read_item(&x) == AMI_ERROR_NO_ERROR) {
      cout << *x << ", "; cout.flush();
    }
    cout << "\n";    
#ifdef EMPQ_ASSERT_EXPENSIVE
    assert(sorted_buf->stream_len() == buff[i]->len());
#endif
  }
#ifdef EMPQ_ASSERT_EXPENSIVE
  assert(size() == init_size);
#endif  
  //reset buff[i] (delete all its streams )
  buff[i]->reset(); 
#ifdef EMPQ_ASSERT_EXPENSIVE 
  assert(size() == init_size - sorted_buf->stream_len()); 
#endif


  //link sorted buff[i] as a substream into buff[i+1];
  //sorted_buf is a new stream, so it starts out with 0 deleted elements;
  //of ocurse, its length might be smaller than nominal;
  if (buff[i+1]->is_full()) {
    empty_buff(i+1);
  }
  buff[i+1]->insert(sorted_buf, 0);
  
  //update the crt_buf pointer if necessary
  if (crt_buf < i+2) crt_buf = i+2;

#ifdef EMPQ_ASSERT_EXPENSIVE  
  assert(size() == init_size);
#endif  
}


//************************************************************/
/* merge the first <K> elements of the streams of input buffer,
   starting at position <buf.deleted[i]> in each stream; there are
   <buf.arity> streams in total; write output in <outstream>; the
   items written in outstream are of type <merge_output_type> which
   extends T with the stream nb and buffer nb the item comes from;
   this information is needed later to distribute items back; do not
   delete the K merged elements from the input streams; <bufid> is the
   id of the buffer whose streams are being merged;

   the input streams are assumed sorted in increasing order of keys; 
*/
template<class T, class Key>
AMI_err
em_pqueue<T,Key>::merge_buffer(em_buffer<T,Key> *buf,
			       ExtendedMergeStream *outstream, long K) {
  long* bos = buf->get_bos();
  /* buff[0] is a level-1 buffer and so on */
  unsigned short bufid = buf->get_level() - 1;
  /* Pointers to current leading elements of streams */
  unsigned int arity = buf->get_nbstreams();
  AMI_STREAM<T>** instreams = buf->get_streams();
  T* in_objects[arity];
  AMI_err ami_err;
  unsigned int i, j;

  MY_LOG_DEBUG_ID("merge_buffer ");
  MY_LOG_DEBUG_ID(buf->get_level());

  assert(outstream);
  assert(instreams);
  assert(buf->get_buf_len());
  assert(K>0);

  //array initialized with first key from each stream (only non-null keys 
  //must be included)
  MEMORY_LOG("em_pqueue::merge_buffer: allocate keys array\n");
  merge_key<Key>* keys = new merge_key<Key> [arity];
  
  /* count number of non-empty runs */
  j = 0;
  /* rewind and read the first item from every stream */
  for (i = 0; i < arity ; i++ ) {
    assert(instreams[i]);
    //rewind stream
    if ((ami_err = instreams[i]->seek(bos[i])) != AMI_ERROR_NO_ERROR) {
	  cerr << "WARNING!!! EARLY EXIT!!!" << endl;
      return ami_err;
    }
    /* read first item */
    ami_err = instreams[i]->read_item(&(in_objects[i]));
	switch(ami_err) {
	case AMI_ERROR_END_OF_STREAM:
	  in_objects[i] = NULL;
	  break;
	case AMI_ERROR_NO_ERROR:
      //cout << "stream " << i << " read " << *in_objects[i] << "\n";
      //cout.flush();
      // include this key in the array of keys
	  keys[j] = merge_key<Key>(in_objects[i]->getPriority(), i);
      //  cout << "key " << j << "set to " << keys[j] << "\n";
      j++; 
	  break;
	default:
	  cerr << "WARNING!!! EARLY EXIT!!!" << endl;
	  return ami_err;
	}
  }
  unsigned int NonEmptyRuns = j;
  // cout << "nonempyruns = " << NonEmptyRuns << "\n";

  //build heap from the array of keys 
  pqheap_t1<merge_key<Key> > mergeheap(keys, NonEmptyRuns);

  //cout << "heap is : " << mergeheap << "\n";
  //repeatedly extract_min from heap and insert next item from same stream
  long extracted = 0;
  //rewind output buffer
  ami_err = outstream->seek(0);
  assert(ami_err == AMI_ERROR_NO_ERROR);
  ExtendedEltMergeType<T,Key> out;
  while (!mergeheap.empty() && (extracted < K)) {
    //find min key and id of stream it comes from
    i = mergeheap.min().stream_id();
    //write min item to output stream
	out = ExtendedEltMergeType<T,Key>(*in_objects[i], bufid, i);
    if ((ami_err = outstream->write_item(out)) != AMI_ERROR_NO_ERROR) {
	  cerr << "WARNING!!! EARLY EXIT!!!" << endl;
      return ami_err;
    }
    //cout << "wrote " << out << "\n";
    extracted++;    //update nb of extracted elements
    //read next item from same input stream
	ami_err = instreams[i]->read_item(&(in_objects[i]));
	switch(ami_err) {
	case AMI_ERROR_END_OF_STREAM:
	  mergeheap.delete_min();
	  break;
	case AMI_ERROR_NO_ERROR:
	  //extract the min from the heap and insert next key from the
	  //same stream
	  {
		Key k = in_objects[i]->getPriority();
		mergeheap.delete_min_and_insert(merge_key<Key>(k, i));
	  }
	  break;
	default:
	  cerr << "WARNING!!! early breakout!!!" << endl;
	  return ami_err;
    }
    //cout << "PQ: " << mergeheap << "\n";
  } //while
  
  //delete [] keys;  
  //!!! KEYS BELONGS NOW TO MERGEHEAP, AND WILL BE DELETED BY THE 
  //DESTRUCTOR OF MERGEHEAP (CALLED AUUTOMATICALLY ON FUNCTION EXIT)
  //IF I DELETE KEYS EXPLICITELY, THEY WILL BE DELETED AGAIN BY
  //DESTRUCTOR, AND EVERYTHING SCREWS UP..  

  buf->put_streams();
  MY_LOG_DEBUG_ID("merge_buffer: done");
  //cout << "done merging buffer\n";

  assert(extracted == outstream->stream_len());
  assert(extracted); // something in, something out
  return AMI_ERROR_NO_ERROR;
}



//************************************************************/
/* merge the first <K> elements of the input streams; there are <arity>
   streams in total; write output in <outstream>;

   the input streams are assumed sorted in increasing order of their
   keys; 
*/
template<class T, class Key>
AMI_err
em_pqueue<T,Key>::merge_streams(ExtendedMergeStream** instreams, 
				unsigned short arity,
				ExtendedMergeStream *outstream, long K) {

  MY_LOG_DEBUG_ID("enter merge_streams");
  assert(arity> 1);
    
  //Pointers to current leading elements of streams
  std::vector<ExtendedEltMergeType<T,Key> > in_objects(arity);

  AMI_err ami_err;
  //unsigned int i;
  unsigned int nonEmptyRuns=0;   //count number of non-empty runs
 
  //array initialized with first element from each stream (only non-null keys 
  //must be included)
  MEMORY_LOG("em_pqueue::merge_streams: allocate keys array\n");

  merge_key<Key>* keys = new merge_key<Key> [arity];
  assert(keys);

  //rewind and read the first item from every stream
  for (int i = 0; i < arity ; i++ ) {
    //rewind stream
    if ((ami_err = instreams[i]->seek(0)) != AMI_ERROR_NO_ERROR) {
      return ami_err;
    }
    //read first item
	ExtendedEltMergeType<T,Key> *objp;
	ami_err = instreams[i]->read_item(&objp);
	switch(ami_err) {
	case AMI_ERROR_NO_ERROR:
	  in_objects[i] = *objp;
      keys[nonEmptyRuns] = merge_key<Key>(in_objects[i].getPriority(), i);
	  nonEmptyRuns++; 
	  break;
	case AMI_ERROR_END_OF_STREAM:
	  break;
	default:
	  return ami_err;
	}
  }
 assert(nonEmptyRuns <= arity); 
 
  //build heap from the array of keys 
  pqheap_t1<merge_key<Key> > mergeheap(keys, nonEmptyRuns);	/* takes ownership of keys */

  //repeatedly extract_min from heap and insert next item from same stream
  long extracted = 0;
  //rewind output buffer
  ami_err = outstream->seek(0);
  assert(ami_err == AMI_ERROR_NO_ERROR);

  while (!mergeheap.empty() && (extracted < K)) {
    //find min key and id of stream it comes from
    int id = mergeheap.min().stream_id();
    //write min item to output stream
	assert(id < nonEmptyRuns);
	assert(id >= 0);
	assert(mergeheap.size() == nonEmptyRuns);
	ExtendedEltMergeType<T,Key> obj = in_objects[id];
    if ((ami_err = outstream->write_item(obj)) != AMI_ERROR_NO_ERROR) {
      return ami_err;
    }
    //cout << "wrote " << *in_objects[i] << "\n";

    //extract the min from the heap and insert next key from same stream
	assert(id < nonEmptyRuns);
	assert(id >= 0);
	ExtendedEltMergeType<T,Key> *objp;
    ami_err = instreams[id]->read_item(&objp);
	switch(ami_err) {
	case AMI_ERROR_NO_ERROR:
      {
		in_objects[id] = *objp;
		merge_key<Key> tmp = merge_key<Key>(in_objects[id].getPriority(), id);
		mergeheap.delete_min_and_insert(tmp);
	  }
	  extracted++;    //update nb of extracted elements
	  break;
	case AMI_ERROR_END_OF_STREAM:
	  mergeheap.delete_min();
	  break;
	default:
	  return ami_err;
	}
  } //while
  
  //delete [] keys;
  //!!! KEYS BELONGS NOW TO MERGEHEAP, AND WILL BE DELETED BY THE 
  //DESTRUCTOR OF MERGEHEAP (CALLED AUUTOMATICALLY ON FUNCTION EXIT)
  //IF I DELETE KEYS EXPLICITELY, THEY WILL BE DELETED AGAIN BY
  //DESTRUCTOR, AND EVERYTHING SCREWS UP..

  MY_LOG_DEBUG_ID("merge_streams: done");
  return AMI_ERROR_NO_ERROR;
}


//************************************************************/
template<class T, class Key>
void
em_pqueue<T,Key>::clear() {
  pq->clear();
  buff_0->clear();
  
  for(int i=0; i<crt_buf; i++) {
    if(buff[i]) {
      delete buff[i]; buff[i] = NULL;
    }
  }
  crt_buf = 0;
}


//************************************************************/
template<class T, class Key>
void
em_pqueue<T,Key>::print_range() {
  cout << "EM_PQ: [pq=" << pqsize
       << ", b=" << bufsize 
       << ", bufs=" << max_nbuf 
       << ", ar=" << buf_arity << "]\n";
  
  cout << "PQ: ";
  //pq->print_range();
  pq->print();
  cout << endl;

  cout << "B0: ";
  //  buff_0->print_range();
  buff_0->print();
  cout << "\n";
  
  for (unsigned short i=0; i < crt_buf; i++) {
    cout << "B" << i+1 << ": ";
    buff[i]->print_range();
    cout << endl;
  }
  cout.flush();
}



//************************************************************/
template<class T, class Key>
void
em_pqueue<T,Key>::print() {
  cout << "EM_PQ: [pq=" << pqsize
       << ", b=" << bufsize 
       << ", bufs=" << max_nbuf 
       << ", ar=" << buf_arity << "]\n";
  
  cout << "PQ: ";
  pq->print();
  cout << endl;

  cout << "B0: ";
  buff_0->print();
  cout << "\n";
  
  for (unsigned short i=0; i < crt_buf; i++) {
    cout << "B" << i+1 << ": " << endl;
    buff[i]->print();
    cout << endl;
  }
  cout.flush();
}

//************************************************************/
template<class T, class Key>
void em_pqueue<T,Key>::print_size() {
  //sum up the lenghts(nb of elements) of the external buffers 
  long elen = 0;
  cout << "EMPQ: pq=" << pq->size() <<",B0=" << buff_0->get_buf_len() << endl;
  cout.flush();
  for (unsigned short i=0; i < crt_buf; i++) {
    assert(buff[i]);
    cout << "B_" << i+1 << ":"; cout.flush();
    buff[i]->print_stream_sizes();
    elen += buff[i]->get_buf_len();
    //cout << endl;    cout.flush();
  }
  cout << "total: " << elen + pq->size() + buff_0->get_buf_len() << endl << endl;
  cout.flush();
}



/*****************************************************************/
template<class T,class Key> 
void em_pqueue<T,Key>::print_stream_sizes() {
  for (unsigned short i=0; i< crt_buf; i++) {
    cout << "[";
    buff[i]->print_stream_sizes();
    cout << "]";
  }
  cout.flush();
}

#undef XXX

#endif
