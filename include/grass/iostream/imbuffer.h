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



#ifndef __IMBUFFER_H
#define __IMBUFFER_H


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "ami_config.h" //for SAVE_MEMORY
#include "ami_stream.h"
#include "mm.h"
#include "mm_utils.h"
#include "pqheap.h"





/* to do: - iterative sort */



/***************************************************************** 
 ***************************************************************** 
 ***************************************************************** 
 
 in memory buffer (level-0 buffer):
 
 Functionality: 
 
 Stores an array of data in memory; when it becomes full, sorts the
 data and copies it on secondary storage in a level-1 buffer; data is
 stored contiguously from left to right;
 
 assume: class T supports < and getPriority and getValue; elements in
 buffer are sorted according to < relation

 ***************************************************************** 
 ***************************************************************** 
 *****************************************************************/

template<class T> 
class im_buffer { 

private: 
  //maximum capacity of buffer
  unsigned long maxsize;
 
  //index of next empty entry in buffer; between 0 and maxsize;
  //initialized to 0
  unsigned long size; 

  //stored data 
  T* data; 

  bool sorted; //true if it is sorted; set when the buffer is sorted
  //to prevent sorting it twice

public: 
  //create a buffer of maxsize n
  im_buffer(long n): maxsize(n), size(0), sorted(false) {
    assert(n >= 0);
    
    char str[100];
    sprintf(str, "im_buffer: allocate %ld\n",(long)(maxsize*sizeof(T)));
    MEMORY_LOG(str);

    data = new T[maxsize];
    assert(data);
  }
  
  //copy constructor
  im_buffer(const im_buffer &b);

  //free memory
  ~im_buffer() {
    if (data) delete [] data;
  }
  
  //insert an item in buffer in next free position; fail if buffer full
  bool insert(T &x);
     
  //insert n items in buffer; return the number of items acually inserted
  unsigned long insert(T*x, unsigned long n);
   
  //(quick)sort (ascending order) the buffer (in place); 
  //the buffer is overwritten; recursive for the time being..
  void sort();
  
  //return maxsize of the buffer (number of elements);
  unsigned long  get_buf_maxlen() const { return maxsize;}

  //return current size of the buffer(number of elements);
  unsigned long get_buf_len() const { return size;}
  
  //return true if buffer full
  bool is_full() const { return (size == maxsize);}
  
  //return true if buffer empty
  bool is_empty() const { return (size == 0);}
  
  //return i'th item in buffer
  T get_item(unsigned long i) const {
    assert((i>=0) && (i < size));
    return data[i];
  }

  //return data
  T* get_array() const { return data;}

  //write buffer to a stream; create the stream  and return it
  AMI_STREAM<T>* save2str() const;
  
  //set i'th item in buffer
  void set_item(unsigned long i, T& item) {
    assert((i>=0) && (i < size));
    data[i] = item;
    sorted = false;
  }
  
  //reset buffer (delete all data); if SAVE_MEMORY is on, free also the space
  void reset() { 
    size = 0; 
    sorted = false;
#ifdef SAVE_MEMORY
    delete [] data;   
    data = NULL;
#endif
  }

  //reset buffer (delete all data); don't delete memory
  void clear() { 
    size = 0; 
    sorted = false;
  }

  //reset buffer: keep n elements starting at position start
  void reset(unsigned long start, unsigned long n);

  //shift n items to the left: in effect this deletes the first n items
  void shift_left(unsigned long n);
  
  //print the elements in the buffer
  friend ostream& operator << (ostream& s, const im_buffer &b) { 
    s << "(buffer:) [";
    for (int i=0; i < b.size; i++) {
      s << b.data[i] << ", ";
    }
    return s << "]";
  }

  //print range: prints the range of the items in buffer
  void print_range() const;
  
  //print
  void print() const;

private:

  //sort the buffer (recursively)
  void sort_rec(unsigned long start, unsigned long end);  

  //partition the buffer and return the the position of the pivot
  unsigned long partition(unsigned long start, unsigned long end); 

};


/************************************************************/
//copy constructor
template<class T>
im_buffer<T>::im_buffer(const im_buffer &b) {
  
  MEMORY_LOG("im_buffer: copy constructor start\n");
  maxsize = b.maxsize;
  size = b.size;
  sorted = b.sorted;
  assert(data);
  for (unsigned long i=0; i<size; i++) {
    data[i] = b.data[i];
  }
  MEMORY_LOG("im_buffer: copy constructor end\n");
}


/************************************************************/
//insert an item in buffer; fail if buffer full
template<class T>
bool im_buffer<T>::insert(T &x) {
 
  if (size == maxsize) {
    return false; //buffer full
  }
#ifdef SAVE_MEMORY
  if (!data) {
    data = new T [maxsize];
  }
#endif
  assert(data);
  assert(size < maxsize);
  data[size] = x;
  size++;
  sorted = false; //not worth checking..
  return true;
}
  

/************************************************************/
//insert n items in buffer; return the number of items acually inserted
template<class T>
unsigned long im_buffer<T>::insert(T*x, unsigned long n) {

  assert(x);
  for (unsigned long i=0; i<n; i++) {
    if (!insert(x[i])) {
      return i;
    }
  }
  assert(sorted == false);
  return n;
}


/************************************************************/
//(quick)sort (ascending order) the buffer (in place); 
//the buffer is overwritten; recursive for the time being..
template<class T>
void im_buffer<T>::sort () {
  if (!is_empty()) {
    if (!sorted) {
      //use my quicksort - eventually must be iterative
      //sort_rec(0, size-1);
      
      //use system quicksort
      qsort((T*)data, size, sizeof(T), T::qscompare);
    }
  }
  sorted = true;
}


/************************************************************/
template<class T>
void im_buffer<T>::sort_rec(unsigned long start, unsigned long end) {
  unsigned long q;
  if (start < end) {
    q = partition(start, end);
    sort_rec(start, q);
    sort_rec(q+1, end);
  }
}
 
/************************************************************/
//partition the buffer in place and return the the position of the
//pivot
template<class T>
unsigned long im_buffer<T>::partition(unsigned long start, unsigned long end) {
  assert((start <= end) && (end < size) && (start >=0));
  if (start == end) {
    return start;
  }
  T pivot = get_item(start), lit, rit;
  unsigned long l = start - 1, r = end + 1;
  
  while (1) {
    
    do {
      r = r - 1;
    } while (get_item(r) > pivot);
    
    do {
      l = l + 1;
    } while (get_item(l) < pivot);
    
    if (l < r) {
      lit = get_item(l);
      rit = get_item(r);
      set_item(l, rit);
      set_item(r, lit);
    } else {
      //printf("partition (%ld,%ld) return %ld\n", start, end, r);
      return r;
    }
  }
}


/************************************************************/
//reset buffer: keep n elements starting at position start
template<class T>
void im_buffer<T>::reset(unsigned long start, unsigned long n) {
  
  if (start >= size) {
    //buffer is completely reset
    assert(n==0);
    size = 0;
    sorted = false;
    return;
  }
  assert((start >= 0) && (start + n  <= size));
  size = n;
  if (n) {
    memmove(data, data + start, n*sizeof(T));
  } 
  //remains sorted
}


/************************************************************/ 
//shift n items to the left: in effect this deletes the first n items
template<class T>
void im_buffer<T>::shift_left(unsigned long n) {
  assert(n <= size);
  //remains sorted
  if (n) {
    memmove(data, data + n, (size-n)*sizeof(T));
    size -= n;
  } 
}
  

/************************************************************/
//print range of the (priority of) items in the buffer
template<class T>
void im_buffer<T>::print_range() const {

  if (size==0) {
    cout << "[]";
  } else {
#ifdef SAVE_MEMORY
    if (!data) {
      data = new T [maxsize];
    }
#endif
    assert(data);

    //determin min and  max
    T min, max;
    min = data[0];
    if (sorted) {
      max = data[size];
    } else {
      max = data[0];
      for (int i=1; i < size; i++) {
	if (data[i] < min) {
	  min = data[i];
	}
	if (data[i] > max) {
	  max = data[i];
	}
      }
    }    
    //print min and max
    cout << "[";
    cout << min.getPriority() << ".."
	 << max.getPriority();
    cout << " (sz=" << size << ")"; //print also bufsize
    cout << "]";
  } //else (size==0)
}


/************************************************************/
//print (priority of) all items in buffer 
template<class T>
void im_buffer<T>::print() const {
  cout << "[";
  for (unsigned long i=0; i < size; i++) {
    cout << data[i].getPriority() << ",";
  }
  cout << "]";
}



/************************************************************/
//write buffer to a stream; create the stream and return it;
//buffer must be sorted prior to saving (functionality of empq)
template<class T>
AMI_STREAM<T>* im_buffer<T>::save2str() const {
  
  AMI_err ae;
  
  AMI_STREAM<T>* amis = new AMI_STREAM<T>();
  assert(amis);

  assert(sorted);//buffer must be sorted prior to saving;
  for (unsigned long i=0; i< size; i++) {
    ae = amis->write_item(data[i]);
    assert(ae == AMI_ERROR_NO_ERROR);
  }
  return amis;
}



#endif
