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


#ifndef _MEM_STREAM_H
#define _MEM_STREAM_H

#include <stdlib.h>
#include <assert.h>

#include <cstring>
#include <iostream>

template<class T>
class MEM_STREAM {
private:
  T *data;
  T *curr;
  T *dataend;
  int len;

public:
  MEM_STREAM(T *data, int len);
  ~MEM_STREAM(void);

  // Read and write elements.
  AMI_err read_item(T **elt);

  AMI_err write_item(const T &elt);

  // Return the number of items in the stream.
  off_t stream_len(void);

  // Return the path name of this stream.
  AMI_err name(char **stream_name);

  // Move to a specific item in the stream.
  AMI_err seek(off_t offset);

  char *sprint();
};


/**********************************************************************/

template<class T>
MEM_STREAM<T>::MEM_STREAM(T *datap, int lenv) {

  data = datap;
  dataend = data + lenv;
  curr = datap;
  len = lenv;

}


/**********************************************************************/
// Return the number of items in the stream.
template<class T>
off_t MEM_STREAM<T>::stream_len(void) {

  return len;

}



/**********************************************************************/
// Return the path name of this stream.
template<class T>
AMI_err MEM_STREAM<T>::name(char **stream_name)  {

  const char *path = "dummy";

  *stream_name = new char [strlen(path) + 1];
  strcpy(*stream_name, path);

  return AMI_ERROR_NO_ERROR;
}


/**********************************************************************/
// Move to a specific offset within the (sub)stream.
template<class T>
AMI_err MEM_STREAM<T>::seek(off_t offset) {

  assert(offset <= len);

  curr = data + offset;

  return AMI_ERROR_NO_ERROR;
}



/**********************************************************************/
template<class T>
MEM_STREAM<T>::~MEM_STREAM(void)  {
}



/**********************************************************************/
template<class T>
AMI_err MEM_STREAM<T>::read_item(T **elt)  {

  assert(data);

  if(curr == dataend) {
    return AMI_ERROR_END_OF_STREAM;
  }
  *elt = curr;
  curr++;
  return AMI_ERROR_NO_ERROR;
}




/**********************************************************************/

template<class T>
AMI_err MEM_STREAM<T>::write_item(const T &elt) {

  assert(data);

  if(curr == dataend) {
    return AMI_ERROR_END_OF_STREAM;
  }
  *curr = elt;
  curr++;
  return AMI_ERROR_NO_ERROR;
}


/**********************************************************************/
// sprint()
// Return a string describing the stream
//
// This function gives easy access to the file name, length.
// It is not reentrant, but this should not be too much of a problem 
// if you are careful.
template<class T>
char *MEM_STREAM<T>::sprint()  {
  static char buf[BUFSIZ];
  sprintf(buf, "[MEM_STREAM %d]", stream_len());
  return buf;
}

#endif // _MEM_STREAM_H 
