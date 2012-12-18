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


#ifndef __EMBUFFER_H
#define __EMBUFFER_H


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "ami_config.h" //for SAVE_MEMORY
#include "ami_stream.h"
#include "mm.h"
#include "mm_utils.h"
#include "pqheap.h"




#define MY_LOG_DEBUG_ID(x) //inhibit debug printing
//#define MY_LOG_DEBUG_ID(x) LOG_DEBUG_ID(x)



/**********************************************************
                  DEBUGGING FLAGS 
***********************************************************/

//setting this enables checking that the streams/arrays inserted in
//buffers are sorted in increasing order
//#define EMBUF_CHECK_INSERT

//enable checking that stream name is the same as the one stored in
//the buffer name[]
//#define EMBUF_CHECK_NAME

//enable printing names as they are checked
//#define EMBUF_CHECK_NAME_PRINT

//enable printing when streams in  a buffer are shifted left to 
//check that names are shifted accordingly
//#define EMBUF_DELETE_STREAM_PRINT

//enable printing the name of the stream which is inserted in buff
//#define EMBUF_PRINT_INSERT

//enable printing the stream names/sizes in cleanup()
//#define EMBUF_CLEANUP_PRINT

//enable printing when get/put_stream is called (for each stream)
//#define EMBUF_PRINT_GETPUT_STREAM

//enable printing when get/put_streams is called 
//#define EMBUF_PRINT_GETPUT_STREAMS

/***********************************************************/





/*****************************************************************/
/* encapsulation of the key together with stream_id; used during
   stream merging to save space;
*/
template<class KEY> 
class merge_key {
public:
  KEY k;
  unsigned int str_id; //id of the stream where key comes from

public:
  merge_key(): str_id(0) {}

  merge_key(const KEY &x, const unsigned int sid):
    k(x), str_id(sid) {}
  
  ~merge_key() {}
  
  void set(const KEY &x, const unsigned int sid) {
    k = x;
    str_id = sid;
  }
  KEY key()  const {
    return k; 
  }
  unsigned int stream_id() const  {
    return str_id;
  }
  KEY getPriority()  const {
    return k;
  }

  friend ostream& operator<<(ostream& s, const merge_key<KEY> &x) {
    return s << "<str_id=" << x.str_id << "> " << x.k << " ";
  }
  friend int operator < (const merge_key &x, 
				const merge_key &y) {
    return (x.k < y.k);
  }
  friend int operator <= (const merge_key &x, 
				 const merge_key &y) {
    return (x.k <= y.k);
  }
  friend int operator > (const merge_key &x, 
				const merge_key &y) {
    return (x.k > y.k);
  }
  friend int operator >= (const merge_key &x, 
				 const merge_key &y) {
    return (x.k >= y.k);
  }
  friend int operator != (const merge_key &x, 
				 const merge_key &y) {
    return (x.k != y.k);
  }
  friend int operator == (const merge_key &x, 
				 const merge_key &y) {
    return (x.k == y.k);
  }
  friend merge_key operator + (const  merge_key &x, 
				      const  merge_key &y) {
    assert(0);
    return x;
    //  Key sum = x.k + y.k;
    //  merge_key f(sum, x.str_id);
    //  return f;
  }

};






/***************************************************************** 
 ***************************************************************** 
 ***************************************************************** 

 external_memory buffer
 
 Each level-i buffer can store up to <arity>^i * <basesize> items,
 where tipically <arity> is \theta(m) and <basesize> is \theta(M);
 therefore log_m{n/m} buffers are needed to store N items, one
 buffer for each level 1..log_m{n/m}. All buffers must have same
 values or <arity> and <basesize>.
 
 Functionality:
 
 A level-i on-disk buffer stores <arity>^i * <basesize> items of
 data, organized in <arity> streams of <arity>^{i-1} items each;
 <basesize> is same for all buffers and equal to the size of the
 level 0 buffer (in memory buffer).
 
 Invariant: all the <arity> streams of a level-i buffer are in
 sorted order; in this way sorting the buffer is done by merging the
 <arity> streams in linear time. 
 
 Items are inserted in level i-buffer only a whole stream at a time
 (<arity>^{i-1}*<basesize> items). When all the <arity> streams of
 the buffer are full, the buffer is sorted and emptied into a stream
 of a level (i+1)-buffer. 
 
 The <arity> streams of a buffer are allocated contigously from left
 to r ight. The unused streams are NULL; The buffer keeps the index of
 the last used(non-NULL) stream. When a buffer becomes full and is
 empty, all its buffers are set to NULL.

 ***************************************************************** 
 ***************************************************************** 
 ***************************************************************** */

/* T is a type with priority of type K and method getPriority() */
template<class T, class Key> 
class em_buffer {
private: 

  //number of streams in a buffer;
  unsigned int arity;
  
  //level of buffer: between 1 and log_arity{n/arity}; (level-0 buffer
  //has a slightly different behaviour so it is implemented as a
  //different class <im_buffer>)
  unsigned short level;

  //level-i buffer contains m streams of data, each of size
  //arity^{i-1}*basesize;
  AMI_STREAM<T>** data;
  
  //the buffers can be depleted to fill the internal pq;
  //keep an array which counts, for each stream, how many elements 
  //have been deleted (implicitely from the begining of stream)
  long* deleted;

  //nb of items in each substream; this can be found out by calling
  //stream_len() on the stream, but it is more costly esp in the case
  //when streams are on disk and must be moved in and out just to find
  //stream length; streamsize is set only at stream creation, and the
  //actual size must substract the number of iteme deleted from the
  //bos
  unsigned long* streamsize;
  
  //index of the next available(empty) stream (out of the total m
  //streams in the buffer);
  unsigned int index;
  
  //nb of items in a stream of level_1 buffer
  unsigned long basesize;


public:

  //create a level-i buffer of given basesize;
  em_buffer(const unsigned short i, const unsigned long bs, 
			const unsigned int ar);
  
  //copy constructor;
  em_buffer(const em_buffer &buf);

  //free the stream array and the streams pointers
  ~em_buffer();
  
  //return the level of the buffer;
  unsigned short get_level() const { return level;}

  //return the ith stream (load stream in memory)
  AMI_STREAM<T>* get_stream(unsigned int i);
  
  //return a pointer to the streams of the buffer (loads streams in
  //memory)
  AMI_STREAM<T>** get_streams();

  //put the ith stream back to disk
  void put_stream(unsigned int i);
  
  //called in pair with get_streams to put all streams back to disk
  void put_streams();

  //return a pointer to the array of deletion count for each stream
  long* get_bos() const { return deleted;}
  
  //return the index of the last stream in buffer which contains data;
  unsigned int laststream() const { return index -1;}

  //return the index of the next available stream in the buffer
  unsigned int nextstream() const { return index;}

  //increment the index of the next available stream in the buffer
  void incr_nextstream() { ++index;}

  //return nb of (non-empty) streams in buffer
  unsigned int get_nbstreams() const { return index;}
  
  //return arity
  unsigned int get_arity() const { return arity;}

  //return total nb of deleted elements in all active streams of the buffer
  long total_deleted() const {
    long tot = 0;
    for (unsigned int i=0; i< index; i++) {
      tot += deleted[i];
    }
    return tot;
  }
  
  //mark as deleted one more element from i'th stream
  void incr_deleted(unsigned int i) {
    assert(i<index);
    deleted[i]++;
  }


  //return the nominal size of a stream (nb of items): 
  //arity^{level-1}*basesize;
  unsigned long get_stream_maxlen() const {
    return (unsigned long)pow((double)arity,(double)level-1)*basesize;
  }
  
  //return the actual size of stream i; i must be the index of a valid
  //stream
  unsigned long get_stream_len(unsigned int i) {
    //assert(i>= 0 && i<index);
    return streamsize[i] - deleted[i];
  }

  //return the total current size of the buffer; account for the
  //deleted elements;
  unsigned long get_buf_len() {
    unsigned long tot = 0;
    for (unsigned int i=0; i< index; i++) {
      tot += get_stream_len(i);
    }
    return tot;
  }
  
  //return the total maximal capacity of the buffer
  unsigned long get_buf_maxlen() {
    return arity * get_stream_maxlen();
  }
 
  //return true if buffer is empty (all streams are empty)
  bool is_empty() {
    return ((nextstream() == 0) || (get_buf_len() == 0));
  }
  
  //return true if buffer is full(all streams are full)
  bool is_full() const {
    return (nextstream() == arity);
  }
  
  //reset
  void reset();
  
  //clean buffer: in case some streams have been emptied by deletion
  //delete them and shift streams left;
  void cleanup();
  

  //create and return a stream which contains all elements of all
  //streams of the buffer in sorted ascending order of their
  //keys(priorities);
  AMI_STREAM<T>* sort();


  // insert an array into the buffer; can only insert one
  // level-i-full-stream-len nb of items at a time; assume the length
  // of the array is precisely the streamlen of level-i buffer n =
  // (pow(arity,level-1)*basesize); assume array is sorted; return the
  // number of items actually inserted
  long insert(T* a, long n); 


  // insert a stream into the buffer; assume the length of the stream
  // is precisely the streamlen of level-i buffer n =
  // (pow(arity,level-1)*basesize); the <nextstream> pointer of buffer
  // is set to point to the argument stream; (in this way no stream
  // copying is done, just one pointer copy). The user should be aware
  // the the argument stream is 'lost' - that is a stream cannot be
  // inserted repeatedly into many buffers because this would lead to
  // several buffers pointing to the same stream.

  // stream is assumed sorted; bos = how many elements are deleted
  // from the begining of stream;
     
  // return the number of items actually inserted 
  long insert(AMI_STREAM<T>* str, 
	      long bos=0);
  
  //print range of elements in buffer
  void print_range();
  
  //print all elements in buffer
  void print();
  
 //prints the sizes of the streams in the buffer
  void print_stream_sizes();
 
  //print the elements in the buffer
  friend ostream& operator<<(ostream& s,  em_buffer &b) {
    s << "BUFFER_" << b.level << ": ";
    if (b.index ==0) {
      s << "[]";
    } 
    s << "\n";
    b.get_streams();
    for (unsigned int i=0; i < b.index; i++) {
      b.print_stream(s, i);
    }
    b.put_streams();
    return s;
  }
  
 
private:

  // merge the input streams; there are <arity> streams in total;
  // write output in <outstream>; the input streams are assumed sorted
  // in increasing order of their keys;
  AMI_err substream_merge(AMI_STREAM<T>** instreams, 
			  unsigned int arity, 
			  AMI_STREAM<T> *outstream); 

  
  //print to stream the elements in i'th stream
  void print_stream(ostream& s, unsigned int i); 



#ifdef SAVE_MEMORY
  //array of names of streams; 
  char** name;

  //return the designated name for stream i
  char* get_stream_name(unsigned int i) const;
 
  //print all stream names in buffer
  void print_stream_names();


  //checks that name[i] is the same as stream name; stream i must be in
  //memory (by a previous get_stream call, for instance) in order to
  //find its length
  void check_name(unsigned int i);
#endif

};


/************************************************************/
//create a level-i buffer of given basesize;
template <class T, class Key>
em_buffer<T,Key>::em_buffer(const unsigned short i, const unsigned long bs, 
							const unsigned int ar) : 
  arity(ar), level(i),  basesize(bs) {

  assert((level>=1) && (basesize >=0));
 
  char str[100];
  sprintf(str, "em_buffer: allocate %d AMI_STREAM*, total %ld\n",
	  arity, (long)(arity*sizeof(AMI_STREAM<T>*)));
  MEMORY_LOG(str);
  //allocate STREAM* array
  data = new AMI_STREAM<T>* [arity];
   
  //allocate deleted array
  sprintf(str, "em_buffer: allocate deleted array: %ld\n",
		  (long)(arity*sizeof(long)));
  MEMORY_LOG(str);
  deleted = new long[arity];
 
  //allocate streamsize array 
  sprintf(str, "em_buffer: allocate streamsize array: %ld\n",
		  (long)(arity*sizeof(long)));
  MEMORY_LOG(str);
  streamsize = new unsigned long[arity];
 
#ifdef SAVE_MEMORY
  //allocate name array
  sprintf(str, "em_buffer: allocate name array: %ld\n",
		  (long)(arity*sizeof(char*)));
  MEMORY_LOG(str);
  name = new char* [arity];
  assert(name);
#endif

  //assert data
  if ((!data) || (!deleted) || (!streamsize)) {
    cerr << "em_buffer: cannot allocate\n";
    exit(1);
  }
  
  //initialize the <arity> streams to NULL, deleted[], streamsize[]
  //and name[]
  for (unsigned int i=0; i< arity; i++) {
    data[i] = NULL;
    deleted[i] = 0;
    streamsize[i] = 0;
#ifdef SAVE_MEMORY
    name[i] = NULL;
#endif
  }   
  //set index
  index = 0;

#ifdef SAVE_MEMORY  
  //streams_in_memory = false;
#endif
}


/************************************************************/
//copy constructor;
template<class T, class Key>
em_buffer<T,Key>::em_buffer(const em_buffer &buf): 
  level(buf.level), basesize(buf.basesize), 
  index(buf.index), arity(buf.arity) {

  assert(0);//should not get called

  MEMORY_LOG("em_buffer: copy constr start\n");
  get_streams();
  for (unsigned int i=0; i< index; i++) {
    assert(data[i]);
    delete data[i]; //delete old stream if existing 
    data[i] = NULL;
    
    //call copy constructor; i'm not sure that it actually duplicates
    //the stream and copies the data; should that in the BTE
    //sometimes..
    data[i] = new AMI_STREAM<T>(*buf.data[i]);
    deleted[i] = buf.deleted[i];
    streamsize[i] = buf.streamsize[i];
#ifdef SAVE_MEMORY
    assert(name[i]);
    delete name[i];
    name[i] = NULL;
    name[i] = buf.name[i];
#endif
  }
  put_streams();
  MEMORY_LOG("em_buffer: copy constr end\n");
}


/************************************************************/
//free the stream array and the streams pointers
template<class T, class Key>
em_buffer<T,Key>::~em_buffer() {

  assert(data);
  //delete the m streams in the buffer
  get_streams();
  for (unsigned int i=0; i<index; i++) {
    assert(data[i]);
#ifdef SAVE_MEMORY
    check_name(i);
    delete name[i]; 
#endif
    delete data[i]; 
    data[i] = NULL;
  }
  
  delete [] data;
  delete [] deleted;
  delete [] streamsize;
#ifdef SAVE_MEMORY  
  delete [] name;
#endif
}


#ifdef SAVE_MEMORY
/************************************************************/
//checks that name[i] is the same as stream name; stream i must be in
//memory (by a previous get_stream call, for instance) in order to
//find its length
template<class T, class Key>
void em_buffer<T,Key>::check_name(unsigned int i) {

#ifdef EMBUF_CHECK_NAME
  assert(i>=0 && i < index);
  assert(data[i]);

  char* fooname;
  data[i]->name(&fooname);//name() allocates the string
#ifdef EMBUF_CHECK_NAME_PRINT
  cout << "::check_name: checking stream [" << level << "," << i << "] name:" 
       << fooname << endl; 
  cout.flush();
#endif
  if (strcmp(name[i], fooname) != 0) {
    cerr << "name[" << i << "]=" << name[i]
	 << ", streamname=" << fooname << endl;
  } 
  assert(strcmp(fooname, name[i]) == 0);
  delete fooname;
#endif  
}
#endif




/************************************************************/
//if SAVE_MEMORY flag is set, load the stream in memory; return the
//ith stream
template<class T, class Key>
AMI_STREAM<T>* em_buffer<T,Key>::get_stream(unsigned int i) {

  assert(i>=0 && i < index);
     
#ifdef SAVE_MEMORY
  MY_LOG_DEBUG_ID("em_buffer::get_stream"); 
  MY_LOG_DEBUG_ID(i);
  
  if (data[i] == NULL) {

    //stream is on disk, load it in memory
    assert(name[i]);
    MY_LOG_DEBUG_ID("load stream in memory");
    MY_LOG_DEBUG_ID(name[i]);
  
#ifdef EMBUF_PRINT_GETPUT_STREAM
    cout << "get_stream:: name[" << i << "]=" << name[i] << " from disk\n"; 
    cout.flush();
#endif
    
    //assert that file exists
    FILE* fp;
    if ((fp = fopen(name[i],"rb")) == NULL) {
      cerr << "get_stream: checking that stream " << name[i] << "exists\n";
      perror(name[i]);
      assert(0);
      exit(1);
    }
    fclose(fp);

    //create an AMI_STREAM from file
    data[i] = new AMI_STREAM<T>(name[i]);
    assert(data[i]);

  } else {

    //if data[i] not NULL, stream must be already in memory    
    MY_LOG_DEBUG_ID("stream not NULL");
    MY_LOG_DEBUG_ID(data[i]->sprint());
  }
#endif
  

  //NOW STREAM IS IN MEMORY

  //some assertion checks
  assert(data[i]);
  assert(data[i]->stream_len() == streamsize[i]);
#ifdef SAVE_MEMORY
  check_name(i);
#endif

  return data[i];
}




/************************************************************/
//if SAVE_MEMORY flag is set, put the i'th stream back on disk
template<class T, class Key>
void em_buffer<T,Key>::put_stream(unsigned int i) {

  assert(i>=0 && i < index);

#ifdef SAVE_MEMORY
  MY_LOG_DEBUG_ID("em_buffer::put_stream");
  MY_LOG_DEBUG_ID(i);
  
  if (data[i] != NULL) {

    //stream is in memory, put it on disk
    MY_LOG_DEBUG_ID("stream put to disk");
    MY_LOG_DEBUG_ID(data[i]->sprint());

    check_name(i);
#ifdef EMBUF_PRINT_GETPUT_STREAM
    cout << "put_stream:: name[" << i << "]=" << name[i] << " to disk\n"; 
    cout.flush();
#endif
  
    //make stream persistent and delete it
    data[i]->persist(PERSIST_PERSISTENT);
    delete data[i]; 
    data[i] = NULL;

  } else {

    //data[i] is NULL, so stream must be already put on disk
    MY_LOG_DEBUG_ID("stream is NULL");
  }
#endif 
  
}




/************************************************************/
//return a pointer to the streams of the buffer
template<class T, class Key>
AMI_STREAM<T>** em_buffer<T,Key>::get_streams() { 

#ifdef SAVE_MEMORY
  MY_LOG_DEBUG_ID("em_buffer::get_streams: reading streams from disk");
#ifdef EMBUF_PRINT_GETPUT_STREAMS
  cout << "em_buffer::get_streams (buffer " << level <<")"; 
  cout << ": index = " << index << "(arity=" << arity << ")\n";
  cout.flush();
#endif

  for (unsigned int i=0; i<index; i++) {
    get_stream(i);
    assert(data[i]);
  }

#endif

  return data;
}




/************************************************************/
//called in pair with load_streams to store streams on disk
//and release the memory
template<class T, class Key>
void em_buffer<T,Key>::put_streams() { 

#ifdef SAVE_MEMORY
  MY_LOG_DEBUG_ID("em_buffer::put_streams: writing streams on disk");
#ifdef EMBUF_PRINT_GETPUT_STREAMS
  cout << "em_buffer::put_streams (buffer " << level <<")"; 
  cout << ": index = " << index << "(arity=" << arity << ")\n";
  cout.flush();
#endif

  for (unsigned int i=0; i<index; i++) {
    put_stream(i);
    assert(data[i] == NULL);
  }
#endif

}



#ifdef SAVE_MEMORY
/************************************************************/
//return the name of the ith stream
template<class T, class Key>
char* em_buffer<T,Key>::get_stream_name(unsigned int i) const {
  
  assert(i>=0 && i<index);
  assert(name[i]);
  return name[i];
}
#endif




#ifdef SAVE_MEMORY
/************************************************************/
template<class T, class Key>
void em_buffer<T,Key>::print_stream_names() {
  unsigned int i;
  for (i=0; i<index; i++) {
    assert(name[i]);
    cout << "stream " << i << ": " << name[i] << endl;
  }
  cout.flush();
}
#endif




/************************************************************/
//clean buffer in case some streams have been emptied by deletion
template<class T, class Key>
void em_buffer<T,Key>::cleanup() {
 
  MY_LOG_DEBUG_ID("em_buffer::cleanup()");
#ifdef EMBUF_CLEANUP_PRINT
#ifdef SAVE_MEMORY
  if (index>0) {
    cout << "before cleanup:\n";
    print_stream_names();
    print_stream_sizes();  
    cout.flush();
  }
#endif
#endif
  
  //load all streams in memory
  get_streams(); 

  //count streams of size=0
  unsigned int i, empty=0;
  for (i=0; i<index; i++) {
   
    if (get_stream_len(i) == 0) {
      //printing..
#ifdef EMBUF_DELETE_STREAM_PRINT      
      cout<<"deleting stream [" << level << "," << i <<"]:" ;
#ifdef SAVE_MEMORY
      cout  << name[i]; 
#endif
      cout << endl;
      cout.flush();
#endif
      
#ifdef SAVE_MEMORY
      //stream is empty ==> delete its name 
	  assert(name[i]);
      delete name[i];
      name[i] = NULL;
#endif

      //stream is empty ==> reset data
      assert(data[i]); 
      //data[i]->persist(PERSIST_DELETE); //this is done automatically..
      delete data[i]; 
      data[i] = NULL;
      deleted[i] = 0;
      streamsize[i] = 0;
      empty++;
    }
  }
  //streams are in memory; all streams which are NULL must have been
  //deleted

  //shift streams to the left in case holes were introduced
  unsigned int j=0;
  if (empty) {
#ifdef EMBUF_DELETE_STREAM_PRINT 
    cout << "em_buffer::cleanup: shifting streams\n"; cout.flush();
#endif
    for (i=0; i<index; i++) {
      //if i'th stream is not empty, shift it left if necessary
      if (data[i]) {
		if (i!=j) {
		  //set j'th stream to point to i'th stream
		  //cout << j << " set to " << i << endl; cout.flush();
		  data[j] = data[i];
		  deleted[j] = deleted[i];
		  streamsize[j] = streamsize[i];
		  //set i'th stream to point to NULL
		  data[i] = NULL;
		  deleted[i] = 0;
		  streamsize[i] = 0;
#ifdef SAVE_MEMORY
		  //fix the names
/* 	already done	  assert(name[j]); */
/* 		  delete name[j]; */
		  name[j] = name[i];
		  name[i] = NULL;
		  check_name(j);
#endif
		}  else {
		  //cout << i << " left the same" << endl;
		}
		j++;
      } //if data[i] != NULL
    }//for i

    //set the index 
    assert(index == j + empty);
    index = j;
    
#ifdef EMBUF_DELETE_STREAM_PRINT 
    cout << "em_buffer::cleanup: index set to " << index << endl;
    cout.flush();
#endif  
  } //if empty

  //put streams back to disk
  put_streams();

#ifdef EMBUF_CLEANUP_PRINT
#ifdef SAVE_MEMORY
  if (index >0) {
    cout << "after cleanup:\n";
    print_stream_names();
    print_stream_sizes();  
    cout.flush();
  }
#endif
#endif
}




/************************************************************/
//delete all streams
template<class T, class Key>
void em_buffer<T,Key>::reset() {
  
  get_streams();
  
  //make streams not-persistent and delete them
  for (unsigned int i=0; i<index; i++) {
    assert(data[i]);
    assert(streamsize[i] == data[i]->stream_len()); 
#ifdef SAVE_MEMORY   
    check_name(i);
	assert(name[i]);
    delete name[i];
    name[i] = NULL;
#endif
    
    data[i]->persist(PERSIST_DELETE);

    delete data[i]; 
    data[i] = NULL;
    deleted[i] = 0;
    streamsize[i] = 0;
  }
  
  index = 0;
}



/************************************************************/
//create and return a stream which contains all elements of 
//all streams of the buffer in sorted ascending order of 
//their keys  (priorities); 
template<class T, class Key>
AMI_STREAM<T>* 
em_buffer<T,Key>::sort() {
  
  //create stream
  MEMORY_LOG("em_buffer::sort: allocate new AMI_STREAM\n");

  AMI_STREAM<T>* sorted_stream = new AMI_STREAM<T>();  /* will be deleteed in insert() */
  assert(sorted_stream);
  
  //merge the streams into sorted stream
  AMI_err aerr;
  //Key dummykey;
  //must modify this to seek after deleted[i] elements!!!!!!!!!!!!!
  // aerr = MIAMI_single_merge_Key(data, arity, sorted_stream, 
  // 				  0, dummykey);
  //could not use AMI_merge so i had to write my own..

  get_streams();

  aerr = substream_merge(data, arity, sorted_stream);
  assert(aerr == AMI_ERROR_NO_ERROR);
 
  put_streams();
  
  return sorted_stream;
}


  

/************************************************************/
/* merge the input streams; there are <arity> streams in total; write
   output in <outstream>;
   
   the input streams are assumed sorted in increasing order of their
   keys;
   
   assumes the instreams are in memory (no need for get_streams()) */
template<class T, class Key>
AMI_err em_buffer<T,Key>::substream_merge(AMI_STREAM<T>** instreams,
					  unsigned int arity,
					  AMI_STREAM<T> *outstream) {
  
  unsigned int i, j;
  
  //some assertion checks
  assert(instreams);
  assert(outstream);
  for (i = 0; i < arity ; i++ ) {
    assert(instreams[i]);
#ifdef SAVE_MEMORY    
    check_name(i);
#endif
  }

  std::vector<T*> in_objects(arity); //pointers to current leading elements of streams
  AMI_err ami_err;
  
 
  char str[200];
  sprintf(str, "em_buffer::substream_merge: allocate keys array, total %ldB\n",
		  (long)((long)arity * sizeof(merge_key<Key>)));
  MEMORY_LOG(str);

 
  //keys array is initialized with smallest key from each stream (only
  //non-null keys must be included) 
  merge_key<Key>* keys;
  //merge_key<Key>* keys = new (merge_key<Key>)[arity];
  typedef merge_key<Key> footype;
  keys = new footype[arity];
  assert(keys);
    
  //count number of non-empty streams
  j = 0; 
  //rewind and read the first item from every stream initializing
  //in_objects and keys
  for (i = 0; i < arity ; i++ ) {
    assert(instreams[i]);
    //rewind stream
    if ((ami_err = instreams[i]->seek(deleted[i])) != AMI_ERROR_NO_ERROR) {
      return ami_err;
    }
    //read first item from stream
    if ((ami_err = instreams[i]->read_item(&(in_objects[i]))) !=
	AMI_ERROR_NO_ERROR) {
      if (ami_err == AMI_ERROR_END_OF_STREAM) {
	in_objects[i] = NULL;
      } else {
	return ami_err;
      }
    } else {
      //include this key in the array of keys
      Key k = in_objects[i]->getPriority();
      keys[j].set(k, i);
      j++; 
    }
  }
  unsigned int NonEmptyRuns = j;

  //build heap from the array of keys 
  pqheap_t1<merge_key<Key> > mergeheap(keys, NonEmptyRuns);

  //repeatedly extract_min from heap, write it to output stream and
  //insert next element from same stream
  merge_key<Key> minelt;
  //rewind output buffer
  ami_err = outstream->seek(0);
  assert(ami_err == AMI_ERROR_NO_ERROR);
  while (!mergeheap.empty()) {
    //find min key and id of the stream from whereit comes
    mergeheap.min(minelt);
    i = minelt.stream_id();
    //write min item to output stream
    if ((ami_err = outstream->write_item(*in_objects[i])) 
	!= AMI_ERROR_NO_ERROR) {
      return ami_err;
    }
    //read next item from same input stream
    if ((ami_err = instreams[i]->read_item(&(in_objects[i])))
	!= AMI_ERROR_NO_ERROR) {
      if (ami_err != AMI_ERROR_END_OF_STREAM) {
	return ami_err;
      }
    }
    //extract the min from the heap and insert next key from same stream
    if (ami_err == AMI_ERROR_END_OF_STREAM) {
      mergeheap.delete_min();
    } else {
      Key k = in_objects[i]->getPriority();
      merge_key<Key> nextit(k, i);
      mergeheap.delete_min_and_insert(nextit);
    }
  } //while
  
  //delete [] keys; 
  //!!! KEYS BELONGS NOW TO MERGEHEAP, AND WILL BE DELETED BY THE
  //DESTRUCTOR OF MERGEHEAP (CALLED AUUTOMATICALLY ON FUNCTION EXIT) IF
  //I DELETE KEYS EXPLICITELY, THEY WILL BE DELETED AGAIN BY DESTRUCTOR,
  //AND EVERYTHING SCREWS UP..
  
  return AMI_ERROR_NO_ERROR;
}





/************************************************************/
// insert an array into the buffer; assume array is sorted; return the
// number of items actually inserted; if SAVE_MEMORY FLAG is on, put
// stream on disk and release its memory
template<class T, class Key>
long em_buffer<T,Key>::insert(T* a, long n) {

  assert(a);

  if (is_full()) {
    cout << "em_buffer::insert: buffer full\n";
    return 0;
  }
  
  //can only insert one full stream at a time
  //relaxed..
  //assert(n == get_stream_maxlen());
  
  //create the stream
  MEMORY_LOG("em_buffer::insert(from array): allocate AMI_STREAM\n");
  AMI_STREAM<T>* str = new AMI_STREAM<T>();
  assert(str);
  
  //write the array to stream
  AMI_err ae;
  for (long i=0; i< n; i++) {
    ae = str->write_item(a[i]);
    assert(ae == AMI_ERROR_NO_ERROR);
  }
  assert(n == str->stream_len());

  //insert the stream in the buffer
  return insert(str);
}




/************************************************************/  
/* insert a stream into the buffer; the next free entry in the buffer
   is set to point to the stream; if SAVE_MEMORY flag is on, the
   stream is put to disk;
   
   the <nextstream> pointer of buffer is set to point to the argument
   stream; (in this way no stream copying is done, just one pointer
   copy). The user should be aware the the argument stream is 'lost' -
   that is a stream cannot be inserted repeatedly into many buffers
   because this would lead to several buffers pointing to the same
   stream.
   
   stream is assume stream is sorted; bos = how many elements must be
   skipped (were deleted) from the begining fo stream;
   
   return the number of items actually inserted */
template<class T, class Key>
long em_buffer<T,Key>::insert(AMI_STREAM<T>* str, long bos) {

  assert(str);
  
  if (is_full()) {
    cout << "em_buffer::insert: buffer full\n";
    return 0;
  }
  
  //can only insert one level-i-full-stream at a time;
  //relaxed..can specify bos;
  //not only that, but the length of the stream can be smaller 
  //than nominal length, because a stream is normally obtained by 
  //merging streams which can be shorter;
  //assert(str->stream_len() == get_stream_len() - bos);


#ifdef EMBUF_CHECK_INSERT
  //check that stream is sorted
  cout << "CHECK_INSERT: checking stream is sorted\n";
  AMI_err ae;
  str->seek(0);
  T *crt=NULL, *prev=NULL;
  while (str->read_item(&crt)) {
    assert(ae == AMI_ERROR_NO_ERROR);
    if (prev) assert(*prev <= *crt);
  }
#endif
  
  //nextstream must be empty
  assert(str);
  assert(data[nextstream()] == NULL);
  assert(deleted[nextstream()] == 0);
  assert(streamsize[nextstream()] == 0);
#ifdef SAVE_MEMORY
  assert(name[nextstream()] == NULL);
#endif


  //set next entry i the buffer to point to this stream
  data[nextstream()] = str;
  deleted[nextstream()] = bos;
  streamsize[nextstream()] = str->stream_len();
#ifdef SAVE_MEMORY
  //set next name entry in buffer to point to this stream's name
  char* s;
  str->name(&s); //name() allocates the string
  name[nextstream()] = s; 
  
  //put stream on disk and release its memory
  str->persist(PERSIST_PERSISTENT);
  delete str;  //stream should be persistent; just delete it 
  data[nextstream()] = NULL;

#ifdef EMBUF_PRINT_INSERT
  cout << "insert stream " << s << " at buf [" << level 
       << "," << nextstream() << "]" << endl;
#endif
#endif
  
  //increment the index of next available stream in buffer
  incr_nextstream();

#ifdef EMBUF_PRINT_INSERT
  print_stream_sizes();
  print_stream_names();
#endif
  
#ifdef SAVE_MEMORY
  MY_LOG_DEBUG_ID("em_buffer::insert(): inserted stream ");
  MY_LOG_DEBUG_ID(name[nextstream()-1]);
#endif

  //return nb of items inserted
  return get_stream_len(nextstream()-1);
}




/************************************************************/  
//print the elements of the i'th stream of the buffer to a stream;
//assumes stream is in memory;
template<class T, class Key>
void em_buffer<T,Key>::print_stream(ostream& s, unsigned int i) {
 
  assert(data[i]);
  assert((i>=0) && (i<index));
  
  AMI_err ae;
  T* x;

  s << "STREAM " << i << ": [";      

  ae = data[i]->seek(deleted[i]);
  assert(ae == AMI_ERROR_NO_ERROR); 
  
  for (long j = 0; j < get_stream_len(i); j++) {
    ae = data[i]->read_item(&x);
    assert(ae == AMI_ERROR_NO_ERROR); 
    s << *x << ",";
  }
  s << "]\n";
}



/************************************************************/ 
//print elements range in buffer (read first and last element in each
//substream and find global min and max)
template<class T, class Key>
void em_buffer<T,Key>::print_range() {

  T *min, *max;
  AMI_err ae;
  
  get_streams();

  for (unsigned int i=0; i< index; i++) {
    cout << "[";
    //read min element in substream i
    ae = data[i]->seek(deleted[i]);
    assert(ae == AMI_ERROR_NO_ERROR);
    ae = data[i]->read_item(&min);
    assert(ae == AMI_ERROR_NO_ERROR);
    cout << min->getPriority() << "..";
    //read max element in substream i
    ae = data[i]->seek(streamsize[i] - 1);
    assert(ae == AMI_ERROR_NO_ERROR);
    ae = data[i]->read_item(&max);
    assert(ae == AMI_ERROR_NO_ERROR);
    cout << max->getPriority() 
	 << " (sz=" << get_stream_len(i) << ")] ";
  }
  for (unsigned int i=index; i< arity; i++) {
    cout << "[] ";
    }
 
  put_streams();
}



/************************************************************/ 
//print all elements in buffer
template<class T, class Key>
void em_buffer<T,Key>::print() {

  T *x;
  AMI_err ae;
  
  get_streams();

  for (unsigned int i=0; i<index; i++) {
    cout << "    [";
    ae = data[i]->seek(deleted[i]);
    assert(ae == AMI_ERROR_NO_ERROR);
    for (unsigned long j=0; j<get_stream_len(i); j++) {
      ae = data[i]->read_item(&x);
      assert(ae == AMI_ERROR_NO_ERROR);
      cout << x->getPriority() << ",";
    }
    cout << "]" << endl;
  }
  for (unsigned int i=index; i< arity; i++) {
    cout << "[] ";
  }

  put_streams();
}



/************************************************************/ 
//print the sizes of the substreams in the buffer
template<class T, class Key>
void em_buffer<T,Key>::print_stream_sizes() {

  cout << "(streams=" << index << ") sizes=[";
  for (unsigned int i=0; i< arity; i++) {
    cout << get_stream_len(i) << ",";
  }
  cout << "]" << endl;
  cout.flush();
}



#endif
