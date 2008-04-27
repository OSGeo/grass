/****************************************************************************
 * 
 *  MODULE:	r.terraflow
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


#ifndef _AMI_STREAM_H
#define _AMI_STREAM_H


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
using namespace std;

#include "mm.h" // Get the memory manager.

#ifdef __MINGW32__
#define getpagesize() (4096)
#endif

#define DEBUG_DELETE if(0)

// The name of the environment variable which keeps the name of the
// directory where streams are stored
#define STREAM_TMPDIR "STREAM_DIR"

// All streams will be names STREAM_*****
#define BASE_NAME "STREAM"

#define STREAM_BUFFER_SIZE (1<<15)


//
// AMI error codes are returned using the AMI_err type.
//
enum AMI_err {
  AMI_ERROR_NO_ERROR = 0,
  AMI_ERROR_IO_ERROR,
  AMI_ERROR_END_OF_STREAM,
  AMI_ERROR_OUT_OF_RANGE,
  AMI_ERROR_READ_ONLY,
  AMI_ERROR_OS_ERROR,
  AMI_ERROR_MM_ERROR,
  AMI_ERROR_OBJECT_INITIALIZATION,
  AMI_ERROR_PERMISSION_DENIED,
  AMI_ERROR_INSUFFICIENT_MAIN_MEMORY,
  AMI_ERROR_INSUFFICIENT_AVAILABLE_STREAMS,
  AMI_ERROR_ENV_UNDEFINED,
  AMI_ERROR_NO_MAIN_MEMORY_OPERATION,
};


//
// AMI stream types passed to constructors
//
enum AMI_stream_type {
    AMI_READ_STREAM = 1,	// Open existing stream for reading
    AMI_WRITE_STREAM,		// Open for writing.  Create if non-existent
    AMI_APPEND_STREAM,		// Open for writing at end.  Create if needed.
    AMI_READ_WRITE_STREAM	// Open to read and write.
};




enum persistence {
    // Delete the stream from the disk when it is destructed.
  PERSIST_DELETE = 0,
  // Do not delete the stream from the disk when it is destructed.
  PERSIST_PERSISTENT,
  // Delete each block of data from the disk as it is read.
  PERSIST_READ_ONCE
};



template<class T> 
class AMI_STREAM {
private:
  FILE * fp;
  //int fd;	//descriptor of file
  AMI_stream_type  access_mode;
  char path[BUFSIZ];
  persistence per;

  //0 for streams, positive for substreams
  unsigned int substream_level;
  
  // If this stream is actually a substream, these will be set to
  // indicate the portion of the file that is part of this stream.  If
  // the stream is the whole file, they will be set to -1. Both are in
  // T units.
  off_t logical_bos;
  off_t logical_eos;

  //stream buffer passed in the call to setvbuf when file is opened
  char* buf;

protected:
  unsigned int get_block_length();
  
public:
   T read_tmp;

  // An AMI_stream with default name
  AMI_STREAM();
  
  // An AMI stream based on a specific path name.
  AMI_STREAM(const char *path_name, 
	     //	     AMI_stream_type st = AMI_READ_WRITE_STREAM);
	     AMI_stream_type st);
  
  // A psuedo-constructor for substreams.
  AMI_err new_substream(AMI_stream_type st, off_t sub_begin, off_t sub_end,
			AMI_STREAM<T> **sub_stream);
  
  // Destructor
  ~AMI_STREAM(void);
  
  // Read and write elements.
  AMI_err read_item(T **elt);

  AMI_err write_item(const T &elt);
  
  AMI_err read_array(T *data, off_t len);
  
  AMI_err write_array(const T *data, off_t len);
  
  // Return the number of items in the stream.
  off_t stream_len(void);
  
  // Return the path name of this stream.
  AMI_err name(char **stream_name);
  
  // Move to a specific item in the stream.
  AMI_err seek(off_t offset);
  
  // Query memory usage
  AMI_err main_memory_usage(size_t *usage,
			    //MM_stream_usage usage_type= MM_STREAM_USAGE_OVERHEAD);
			    MM_stream_usage usage_type);
  
  void persist(persistence p);
  
  char *sprint();
};


/**********************************************************************/
template<class T>
unsigned int AMI_STREAM<T>::get_block_length() {
  return getpagesize();
}


/**********************************************************************/
/* creates a random file name, opens the file for reading and writing
   and and returns a file descriptor */
int ami_single_temp_name(char *base, char* tmp_path);


/**********************************************************************/
/* given fd=fide descriptor, associates with it a stream aopened in
   access_mode and returns it */
FILE*  open_stream(int fd, AMI_stream_type st);


/**********************************************************************/
/* open the file whose name is pathname in access mode */
FILE* open_stream(char* pathname, AMI_stream_type st);




/********************************************************************/
//  An  AMI stream with default name.
template<class T>
AMI_STREAM<T>::AMI_STREAM() {
  
  access_mode = AMI_READ_WRITE_STREAM;
  int fd = ami_single_temp_name(BASE_NAME, path);
  fp = open_stream(fd, access_mode);

  
  /* a stream is by default buffered with a buffer of size BUFSIZ=1K */
  buf = new char[STREAM_BUFFER_SIZE];
  if (setvbuf(fp, buf, _IOFBF, STREAM_BUFFER_SIZE) != 0) {
    cerr << "setvbuf failed (stream " << path << ")" << endl;
    exit(1);
  }
  
  // By default, all streams are deleted at destruction time.
  per = PERSIST_DELETE;

   // Not a substream.
  substream_level  = 0;
  logical_bos = logical_eos = -1;

  seek(0);

  // Register memory usage before returning.
  //size_t usage; 
  //main_memory_usage(&usage,  MM_STREAM_USAGE_CURRENT);
  //MM_manager.register_allocation(usage);
}



/**********************************************************************/
// An AMI stream based on a specific path name.
template<class T>
AMI_STREAM<T>::AMI_STREAM(const char *path_name,
			  AMI_stream_type st = AMI_READ_WRITE_STREAM) {
  
  access_mode = st;
  strcpy(path, path_name);
  fp = open_stream(path, st);
  

  /* a stream is by default buffered with a buffer of size BUFSIZ=1K */
  buf = new char[STREAM_BUFFER_SIZE];
  if (setvbuf(fp, buf, _IOFBF, STREAM_BUFFER_SIZE) != 0) {
    cerr << "setvbuf failed (stream " << path << ")" << endl;
    exit(1);
  }

  // By default, all streams are deleted at destruction time.
  per = PERSIST_DELETE;
  
  // Not a substream.
  substream_level  = 0;
  logical_bos = logical_eos = -1;
  
  seek(0);

  // Register memory usage before returning.
  //size_t usage; 
  //main_memory_usage(&usage,  MM_STREAM_USAGE_CURRENT);
  //MM_manager.register_allocation(usage);
};



/**********************************************************************/
 // A psuedo-constructor for substreams.
template<class T>
AMI_err AMI_STREAM<T>::new_substream(AMI_stream_type st,
				     off_t sub_begin,
				     off_t sub_end,
				     AMI_STREAM<T> **sub_stream) {

  //assume this for now
  assert(st == AMI_READ_STREAM);

  //check range
  if (substream_level) {
     if( (sub_begin >= (logical_eos - logical_bos)) ||
	 (sub_end >= (logical_eos - logical_bos)) ) {
       
       return AMI_ERROR_OUT_OF_RANGE;
     }
  }  else {
    off_t len = stream_len();
    if (sub_begin > len || sub_end > len) {

      return AMI_ERROR_OUT_OF_RANGE;
    }
  }

  //reopen the file 
  AMI_STREAM<T> *substr = new AMI_STREAM<T>(path, st);
  
  // Set up the beginning and end positions.
  if (substream_level) {
    substr->logical_bos = logical_bos + sub_begin;
    substr->logical_eos = logical_bos + sub_end + 1;
  } else {
    substr->logical_bos = sub_begin;
    substr->logical_eos = sub_end + 1;
  }
  
  // Set the current position.
  substr->seek(0);
  
  //set substream level
  substr->substream_level = substream_level + 1;

  //set persistence
  substr->per = per;

  //*sub_stream = (AMI_base_stream < T > *)substr;
  *sub_stream = substr;
  return  AMI_ERROR_NO_ERROR;
};



/**********************************************************************/
// Return the number of items in the stream.
template<class T>
off_t AMI_STREAM<T>::stream_len(void) {

  fflush(fp);

  struct stat buf;
  if (stat(path, &buf) == -1) {
    perror("AMI_STREAM::stream_len(): fstat failed ");
    assert(0);
    exit(1);
  }

  //fprintf(stderr, "%s: length = %lld\n", path, buf.st_size);

  return (buf.st_size / sizeof(T));
};



/**********************************************************************/
// Return the path name of this stream.
template<class T>
AMI_err AMI_STREAM<T>::name(char **stream_name)  {
  
  *stream_name = new char [strlen(path) + 1];
  strcpy(*stream_name, path);
  
  return AMI_ERROR_NO_ERROR;
};



/**********************************************************************/
// Move to a specific offset within the (sub)stream.
template<class T>
AMI_err AMI_STREAM<T>::seek(off_t offset) {

  off_t seek_offset;
  
  if (substream_level) {
    //substream
    if (offset  > (unsigned) (logical_eos - logical_bos)) {
      //offset out of range
      cerr << "AMI_STREAM::seek bos=" << logical_bos << ", eos=" << logical_eos
	   << ", offset " << offset << " out of range.\n";
      assert(0);
      exit(1);
    } else {
      //offset in range 
      seek_offset = (logical_bos + offset) * sizeof(T);
    }


  } else {
    //not a substream
    seek_offset = offset * sizeof(T);
  }

  if (fseek(fp, seek_offset, SEEK_SET) == -1) {
    cerr << "AMI_STREAM::seek offset=" << seek_offset << "failed.\n";
    assert(0);
    exit(1);
  }
  
  return AMI_ERROR_NO_ERROR;
}




/**********************************************************************/
// Query memory usage
template<class T>
AMI_err AMI_STREAM<T>::main_memory_usage(size_t *usage,
					 MM_stream_usage usage_type= MM_STREAM_USAGE_OVERHEAD) {
  
   switch (usage_type) {
   case MM_STREAM_USAGE_OVERHEAD:
     *usage = sizeof (*this);
     break;
   case MM_STREAM_USAGE_BUFFER:
     // *usage = get_block_length();
     *usage = STREAM_BUFFER_SIZE*sizeof(char);
     break;
   case MM_STREAM_USAGE_CURRENT:
   case MM_STREAM_USAGE_MAXIMUM:
     // *usage = sizeof (*this) + get_block_length();
     *usage = sizeof (*this) + STREAM_BUFFER_SIZE*sizeof(char);
     break;
   }
   return AMI_ERROR_NO_ERROR;
};



/**********************************************************************/
template<class T>
AMI_STREAM<T>::~AMI_STREAM(void)  {

  DEBUG_DELETE cerr << "~AMI_STREAM: " << path << "(" << this << ")\n";
  delete buf;
  assert(fp);
  fclose(fp);
  
  // Get rid of the file if not persistent and if not substream.
  if ((per != PERSIST_PERSISTENT) && (substream_level == 0)) {
    if (remove(path) == -1) {
      cerr << "AMI_STREAM: failed to unlink " << path << endl;
      perror("cannot unlink ");
      assert(0);
      exit(1);
    }
  }
  // Register memory deallocation before returning.
  //size_t usage; 
  //main_memory_usage(&usage,  MM_STREAM_USAGE_CURRENT);
  //MM_manager.register_deallocation(usage);
 };



/**********************************************************************/
template<class T>
AMI_err AMI_STREAM<T>::read_item(T **elt)  {

  assert(fp);
  //if we go past substream range
  if ((logical_eos >= 0) && ftell(fp) >= sizeof(T) * logical_eos) {
    return AMI_ERROR_END_OF_STREAM;
  
  } else {
    if (fread((char *) (&read_tmp), sizeof(T), 1, fp) < 1) {
      //cerr << "file=" << path << ":";
      //perror("cannot read!");
      //assume EOF --should fix this XXX
      return AMI_ERROR_END_OF_STREAM;
    }
    
    *elt = &read_tmp;
    return AMI_ERROR_NO_ERROR; 
  }
};




/**********************************************************************/
template<class T>
AMI_err AMI_STREAM<T>::read_array(T *data, off_t len) {
  
  assert(fp);
  
  //if we go past substream range
  if ((logical_eos >= 0) && ftell(fp) >= sizeof(T) * logical_eos) {
    return AMI_ERROR_END_OF_STREAM;
    
  } else {
	if (fread((void*)data, sizeof(T), len, fp) < len) {
	  cerr << "file=" << path << ":";
	  perror("cannot read!");    
	  //assume EOF --should fix this XXX
	  return AMI_ERROR_END_OF_STREAM;
	}
    return AMI_ERROR_NO_ERROR; 
  }
};




/**********************************************************************/
template<class T>
AMI_err AMI_STREAM<T>::write_item(const T &elt) {

  assert(fp);
  //if we go past substream range
  if ((logical_eos >= 0) && ftell(fp) >= sizeof(T) * logical_eos) {
    return AMI_ERROR_END_OF_STREAM;
  
  } else {
    if (fwrite((char*)(&elt), sizeof(T), 1,fp) < 1) {
      cerr << "AMI_STREAM::write_item failed.\n";
      assert(0);
      exit(1);
    }
    return AMI_ERROR_NO_ERROR;
  }
};


/**********************************************************************/
template<class T>
AMI_err AMI_STREAM<T>::write_array(const T *data, off_t len) {

  assert(fp);
  //if we go past substream range
  if ((logical_eos >= 0) && ftell(fp) >= sizeof(T) * logical_eos) {
    return AMI_ERROR_END_OF_STREAM;
    
  } else {
    if (fwrite(data, sizeof(T), len,fp) < len) {
      cerr << "AMI_STREAM::write_item failed.\n";
      assert(0);
      exit(1);
    }
    return AMI_ERROR_NO_ERROR;
  }
};
        

/**********************************************************************/
template<class T>
void AMI_STREAM<T>::persist(persistence p)  {
   per = p;
};



/**********************************************************************/
// sprint()
// Return a string describing the stream
//
// This function gives easy access to the file name, length.
// It is not reentrant, but this should not be too much of a problem 
// if you are careful.
template<class T>
char *AMI_STREAM<T>::sprint()  {
  static char buf[BUFSIZ];
  sprintf(buf, "[AMI_STREAM %s %ld]", path, (long)stream_len());
  return buf;
};

#endif // _AMI_STREAM_H 
