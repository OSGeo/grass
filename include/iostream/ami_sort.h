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


#ifndef _AMI_SORT_H
#define _AMI_SORT_H

#include "ami_sort_impl.h"

#define SORT_DEBUG if(0)


/* ---------------------------------------------------------------------- */

// A version of AMI_sort that takes an input streamof elements of type
// T, creates an output stream and and uses the < operator to sort

// instream is allocated;  *outstream is created
// template<class T>
// AMI_err 
// AMI_sort(AMI_STREAM<T> *instream, AMI_STREAM<T> **outstream) {

//   cout << "Not implemented yet!\n";
//   exit(1);
//   return AMI_ERROR_NO_ERROR;
// }



/* ---------------------------------------------------------------------- */

// A version of AMI_sort that takes an input stream of elements of
// type T, creates an output stream, and a user-specified comparison
// function

// instream is allocated;  *outstream is created
// template<class T>
// AMI_err AMI_sort(AMI_STREAM<T> *instream, AMI_STREAM<T> **outstream,
//                  int (*cmp)(const T&, const  T&)) {

//   cout << "Not implemented yet!\n";
//   exit(1);
//   return AMI_ERROR_NO_ERROR;
// }



/* ---------------------------------------------------------------------- */
// A version of AMI_sort that takes an input stream of elements of
// type T, creates an output stream, and a user-specified comparison
// object. 

//The comparison object "cmp", of (user-defined) class represented by
//CMPR, must have a member function called "compare" which is used for
//sorting the input stream.




//  create  *outstream 
template<class T, class Compare>
AMI_err 
AMI_sort(AMI_STREAM<T> *instream, AMI_STREAM<T> **outstream, Compare *cmp, 
	 int deleteInputStream = 0) {
  char* name;
  queue<char*>* runList;
  int instreamLength;

  assert(instream && outstream && cmp); 
  instreamLength = instream->stream_len();

  if (instreamLength == 0) {
    *outstream = new AMI_STREAM<T>();
    if (deleteInputStream) {
      delete instream;
    }
    return AMI_ERROR_NO_ERROR;
  }
  
  SORT_DEBUG {
    instream->name(&name);
    cout << "AMI_sort: sorting stream" << name <<", len=" 
	 << instreamLength << endl;
    delete name;
    MM_manager.print();
  }
  
  //run formation
  runList = runFormation(instream, cmp);
  assert(runList && runList->length() > 0); 

  if (deleteInputStream) {
    delete instream;
  }

  if (runList->length() == 1) {
    //if 1 run only
    runList->dequeue(&name);
    *outstream = new AMI_STREAM<T>(name);
    delete name; //should be safe, stream makes its own copy
  } else {
    *outstream = multiMerge<T,Compare>(runList,  cmp);
    //i thought the templates are not needed in the call, but seems to
    //help the compiler..laura
  }

  assert(runList->length() == 0);
  delete runList;
  
  SORT_DEBUG {
    cout << "AMI_sort: done" << endl << endl;
    MM_manager.print();
  }

  assert(*outstream);  
  assert((*outstream)->stream_len() == instreamLength);
  return AMI_ERROR_NO_ERROR;
  
}



template<class  T, class Compare>
int
isSorted(AMI_STREAM<T> *str, Compare cmp) {
  T *prev, *crt;
  AMI_err ae;   

  assert(str);
  str->seek(0);
  
  if (str->stream_len() <2) return 1;
  
  ae = str->read_item(&crt);
  cout << "reading: " << *crt << endl;
  prev = new T (*crt);
  ae = str->read_item(&crt);
  while (ae == AMI_ERROR_NO_ERROR) {
    cout << "reading: " << *crt << endl;
    if (cmp.compare(*prev, *crt) != -1)
      assert(0);
      return 0;
    prev = crt;
    ae = str->read_item(&crt);
  }
  return 1;
}
             
                          
#endif // _AMI_SORT_H 
