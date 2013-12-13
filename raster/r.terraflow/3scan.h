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

#ifndef __3SCAN_H
#define __3SCAN_H

#include <iostream>

#include <grass/iostream/ami.h>
#include "types.h"


/* ********************************************************************** */
template<class T, class BASETYPE, class FUN>
void
scan3line(FUN &funobj, 
		  AMI_STREAM<T> *prev, 
		  AMI_STREAM<T> *crt, 
		  AMI_STREAM<T> *next,
		  BASETYPE nodata,
		  dimension_type i) {
  dimension_type j=0;
  AMI_err ae;
  BASETYPE a[3], b[3], c[3];
  T *tmp, center[2];
  int done;
  
  /* LOG_DEBUG_ID("scan3line: begin");
	 if(prev) LOG_DEBUG_ID(prev->sprint());
	 if(crt) LOG_DEBUG_ID(crt->sprint());
	 if(next) LOG_DEBUG_ID(next->sprint());
  */
  /* seek 0 */
  if (prev) {
    ae = prev->seek(0);
    assert(ae == AMI_ERROR_NO_ERROR);
  }
  assert(crt);
  ae = crt->seek(0);
  assert(ae == AMI_ERROR_NO_ERROR);
  if (next) {
    ae = next->seek(0);
    assert(ae == AMI_ERROR_NO_ERROR);
  }
  /* read first 2 elements of each line */
  /* read prev */
  a[0] = b[0] = c[0] = nodata;
  if (prev) {
    ae = prev->read_item(&tmp);
    assert(ae == AMI_ERROR_NO_ERROR);
    a[1] = *tmp;
    ae = prev->read_item(&tmp);
    assert(ae == AMI_ERROR_NO_ERROR);
    a[2] = *tmp;
  } else {
    a[1] = a[2] = nodata;
  }
  /* read crt */
  ae = crt->read_item(&tmp);
  assert(ae == AMI_ERROR_NO_ERROR);
  center[0] = *tmp;
  b[1] = *tmp;
  ae = crt->read_item(&tmp);
  assert(ae == AMI_ERROR_NO_ERROR);
  center[1] = *tmp;
  b[2] = *tmp;
  /* read next */
  if (next) {
    ae = next->read_item(&tmp);
    assert(ae == AMI_ERROR_NO_ERROR);
    c[1] = *tmp;
    ae = next->read_item(&tmp);
    assert(ae == AMI_ERROR_NO_ERROR);
    c[2] = *tmp;
  } else {
    c[1] = c[2] = nodata;
  }
  
  done = 0;
  do {
    funobj.processWindow(i, j, center[0], a, b, c); /* process current window*/
	
	/* slide one over */
    a[0] = a[1]; a[1] = a[2];
    b[0] = b[1]; b[1] = b[2];
	center[0] = center[1];
    c[0] = c[1]; c[1] = c[2];

    j++;
	
    /* read next item from crt and check for EOS */
    ae = crt->read_item(&tmp);
    if  (ae == AMI_ERROR_END_OF_STREAM) {
      done = 1;
      b[2] = nodata;
      center[1] = T();
    } else {
      assert(ae == AMI_ERROR_NO_ERROR);
      b[2] = *tmp;  
      center[1] = *tmp;
    }
    
    /* read next item from prev */
    if (prev) {
      ae = prev->read_item(&tmp);
      if (!done) {
	assert(ae == AMI_ERROR_NO_ERROR);
	a[2]=*tmp;
      } else {
	a[2] = nodata;
	assert(ae == AMI_ERROR_END_OF_STREAM);
      }
    } else {
      a[2] = nodata;
    }
    
    /* read next item from next */
    if (next) {
      ae = next->read_item(&tmp);
      if (!done) {
	assert(ae == AMI_ERROR_NO_ERROR);
	c[2] = *tmp;
      } else {
	c[2] = nodata;
	assert(ae == AMI_ERROR_END_OF_STREAM);
      }
    } else {
      c[2] = nodata;
    }
    
  } while (!done);
  
  /* write last window */
  funobj.processWindow(i, j, center[0], a, b, c); /* process current window */

  /* LOG_DEBUG_ID("scan3line: end"); */
  return;
}







/****************************************************************/
/* amis0 = data (T) in
 */
template<class T, class BASETYPE, class FUN>
void 
scan3(AMI_STREAM<T> &amis0, 
      const dimension_type nr, const dimension_type nc, BASETYPE nodata,
      FUN &funobj) {
  AMI_STREAM<T> *l_prev, *l_crt, *l_next;
  AMI_err ae;

  ae = amis0.seek(0);
  assert( ae == AMI_ERROR_NO_ERROR);  

  /* scan simultaneously 3 lines in the grid and fill in the windows
	 use a substream for each line to trigger prefetching */
  
  /* initialize first 2 lines */
  ae = amis0.new_substream(AMI_READ_STREAM, 0, nc-1, &l_crt);
  assert( ae == AMI_ERROR_NO_ERROR);
  ae = amis0.new_substream(AMI_READ_STREAM, nc, 2*nc-1, &l_next);
  assert( ae == AMI_ERROR_NO_ERROR);
  /* LOG_DEBUG_ID("substreams created"); */

  /* process lines, 3 at a time */
  l_prev = NULL;
  for (dimension_type i=0;i<nr;i++) {
    /* process current line */
    scan3line(funobj, l_prev, l_crt, l_next, nodata, i);
    
    /* advance lines */
    if (l_prev) delete l_prev;
    l_prev = l_crt;
    l_crt = l_next;
    if (i < nr-2) {
      ae = amis0.new_substream(AMI_READ_STREAM, (i+2)*nc, (i+3)*nc-1, 
			      &l_next);
      assert(ae == AMI_ERROR_NO_ERROR);
    } else {
      l_next = NULL;
    }
  }
  if (l_prev) delete l_prev;
  assert(!l_crt);  
  /* LOG_DEBUG_ID("scan3: end");   */
  return;
}




/* ********************************************************************** */
template<class T>
T *
readLine(T *buf, AMI_STREAM<T> &str, const dimension_type len, 
		 const T &nodata) {
  AMI_err ae;
  buf[0] = nodata;
  buf[len+1] = nodata;
  for(dimension_type i=0; i<len; i++) {
	T *tmp;
	ae = str.read_item(&tmp);
	assert(ae==AMI_ERROR_NO_ERROR);
	buf[i+1] = *tmp;
  }
  return buf;
}


/* ********************************************************************** */
template<class T>
T *
setNodata(T *buf, const dimension_type len, const T &nodata) {
  for(dimension_type j=0; j<len+2; j++) {
	buf[j] = nodata;
  }
  return buf;
}



/* ********************************************************************** */
/* memoryScan
 *
 * calls fo.processWindow(i,j, a,b,c) once for each element of the
 * grid.  i,j are the coordinates in the grid.  a,b,c together point
 * to the grid surrounding the cell. (b[1] corresponds to cell i,j).
 * */
template<class T, class FUN>
void
memoryScan(AMI_STREAM<T> &str, 
		   const dimension_type nrows, const dimension_type ncols,
		   const T nodata,
		   FUN &fo) {
  T *a, *b, *c;
  T *buf[3];
  
  str.seek(0);
  
  assert(nrows > 1);
  assert(nrows * ncols == str.stream_len());
  buf[0] = new T[ncols+2];
  buf[1] = new T[ncols+2];
  buf[2] = new T[ncols+2];
  unsigned int k;
  a = setNodata(buf[0], ncols, nodata);
  b = readLine(buf[1], str, ncols, nodata);
  k = 2;
  for(dimension_type i=0; i<nrows-1; i++) {
	c = readLine(buf[k], str, ncols, nodata);
	for(dimension_type j=0; j<ncols; j++) {
	  fo.processWindow(i, j, a+j, b+j, c+j);  
	}
	k = (k+1)%3;
	a = b;
	b = c;
  }
  c = setNodata(buf[k], ncols, nodata);
  for(dimension_type j=0; j<ncols; j++) {
	fo.processWindow(nrows-1, j, a+j, b+j, c+j);  
  }
  delete [] buf[2];
  delete [] buf[1];
  delete [] buf[0];
}




/* ********************************************************************** */
/* this version uses two streams to make the windows 
 * FUN should provide a function:
 * processWindow(dimension_type i, dimension_type j,
 *   T1* a1, T1* b1, T1* c1, 
 *   T2* a2, T2* b2, T2* c2)
 */

template<class T1, class T2, class FUN>
void
memoryScan(AMI_STREAM<T1> &str1, AMI_STREAM<T2> &str2, 
		   const dimension_type nrows, const dimension_type ncols,
		   const T1 nodata1, const T2 nodata2,
		   FUN &fo) {
  T1 *a1, *b1, *c1;
  T1 *buf1[3];
  T2 *a2, *b2, *c2;
  T2 *buf2[3];
  
  str1.seek(0);
  str2.seek(0);

  assert(nrows > 1);
  assert(nrows * ncols == str1.stream_len());
  assert(nrows * ncols == str2.stream_len());
  buf1[0] = new T1[ncols+2];
  buf1[1] = new T1[ncols+2];
  buf1[2] = new T1[ncols+2];
  buf2[0] = new T2[ncols+2];
  buf2[1] = new T2[ncols+2];
  buf2[2] = new T2[ncols+2];
  unsigned int k;
  a1 = setNodata(buf1[0], ncols, nodata1);
  a2 = setNodata(buf2[0], ncols, nodata2);
  b1 = readLine(buf1[1], str1, ncols, nodata1);
  b2 = readLine(buf2[1], str2, ncols, nodata2);
  k = 2;
  for(dimension_type i=0; i<nrows-1; i++) {
	c1 = readLine(buf1[k], str1, ncols, nodata1);
	c2 = readLine(buf2[k], str2, ncols, nodata2);
	for(dimension_type j=0; j<ncols; j++) {
	  fo.processWindow(i, j, a1+j, b1+j, c1+j,
					   a2+j, b2+j, c2+j);  
	}
	k = (k+1)%3;
	a1 = b1;
	a2 = b2;
	b1 = c1;
	b2 = c2;
  }
  c1 = setNodata(buf1[k], ncols, nodata1);
  c2 = setNodata(buf2[k], ncols, nodata2);
  for(dimension_type j=0; j<ncols; j++) {
	fo.processWindow(nrows-1, j, a1+j, b1+j, c1+j,
					 a2+j, b2+j, c2+j);  
  }
  delete [] buf1[2];
  delete [] buf1[1];
  delete [] buf1[0];
  delete [] buf2[2];
  delete [] buf2[1];
  delete [] buf2[0];
}



#endif

