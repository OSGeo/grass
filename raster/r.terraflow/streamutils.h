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

#ifndef STREAMUTILS_H
#define STREAMUTILS_H

#include <fstream>

#include <grass/iostream/ami.h>
#include "types.h"
#include "common.h"




template<class T>
void
printStream(ostream &s, AMI_STREAM<T> *str) {
  T *elt;
  AMI_err ae;

  str->seek(0);
  while((ae = str->read_item(&elt)) == AMI_ERROR_NO_ERROR) {
	s << *elt << endl;
  }
  str->seek(0);
}



/* laura note: this works that class T has an empty contructor which
   initializes it to the nodata value */ 
template<class T, class FUN>
void
printStream2Grid(AMI_STREAM<T> *str, 
		 dimension_type nrows, dimension_type ncols,
		 const char *name,
		 FUN fmt) {
  T *elt, nodata;
  AMI_err ae;
  ofstream fstrm(name);

  if (stats)
    stats->comment("saving grid: ", name);

  fstrm << "rows=" << nrows << endl;
  fstrm << "cols=" << ncols << endl;
  
  str->seek(0);
  ae = str->read_item(&elt);
  assert(ae == AMI_ERROR_NO_ERROR || ae == AMI_ERROR_END_OF_STREAM);
  for(dimension_type i=0; i<nrows; i++) {
    for(dimension_type j=0; j<ncols; j++) {
    
      if(ae == AMI_ERROR_NO_ERROR && elt->i == i && elt->j == j) {
	fstrm << " " << fmt(*elt);
	ae = str->read_item(&elt);
	assert(ae == AMI_ERROR_NO_ERROR || ae == AMI_ERROR_END_OF_STREAM);
      } else {
	fstrm << " " << fmt(nodata);
      }
    } /* for j */
    fstrm << endl;
  }
  assert(ae == AMI_ERROR_END_OF_STREAM); /* stream must have finished */
  str->seek(0);
}


template<class T, class FUN>
void printGridStream(AMI_STREAM<T> *str, 
		     dimension_type nrows, dimension_type ncols,
		     char *name,
		     FUN fmt) {
  T *elt;
  AMI_err ae;
  ofstream fstrm(name);

  if (stats)
    stats->recordLength("saving grid", str);
  fstrm << "rows=" << nrows << endl;
  fstrm << "cols=" << ncols << endl;

  assert(str->stream_len() == nrows * ncols);
  str->seek(0);
  for(dimension_type i=0; i<nrows; i++) {
	for(dimension_type j=0; j<ncols; j++) {
	  ae = str->read_item(&elt);
	  assert(ae == AMI_ERROR_NO_ERROR);
	  fstrm << " " << fmt(*elt);
	}
	fstrm << endl;
  }
  str->seek(0);
}





/* ********************************************************************** */

/* assume sorted... */
template<class T, class FUN>
AMI_STREAM<T> *
removeDuplicates(AMI_STREAM<T> *str, FUN fo) {
  AMI_err ae;

  AMI_STREAM<T> *newStr = new AMI_STREAM<T>();
  if(str->stream_len() == 0) return newStr;	/* empty stream */

  str->seek(0);
  T prev, *elp;
  ae = str->read_item(&elp);
  assert(ae == AMI_ERROR_NO_ERROR);
  prev = *elp;
  while((ae = str->read_item(&elp)) == AMI_ERROR_NO_ERROR) {
	if(fo.compare(*elp, prev)) {	/* differ */
	  newStr->write_item(prev);
	  prev = *elp;
	} else {
	  /* cout << "duplicate: " << *elp << " of " << prev << endl; */
	}
  }
  newStr->write_item(prev);		/* last one */
  return newStr;
}

/* ********************************************************************** */

template<class T, class FUN>
void
removeDuplicatesEx(AMI_STREAM<T> **str, FUN fo) {
  AMI_STREAM<T> *tmp = removeDuplicates(*str, fo);
  delete *str;
  *str = tmp;
}



/* ********************************************************************** */

/* 
 * merge a grid and a stream together to form an new grid of the original type
 * str should be sorted in ij order 
 */
template<class T, class TT, class FUN>
AMI_STREAM<T> *
mergeStream2Grid(AMI_STREAM<T> *grid, 
		 dimension_type rows, dimension_type cols, 
		 AMI_STREAM<TT> *str,
		 FUN fo) {
  AMI_err ae, aeS;
  T *gep;						/* grid element */
  TT *sep;						/* stream element */

  AMI_STREAM<T> *mergeStr = new AMI_STREAM<T>();
  str->seek(0);
  grid->seek(0);
  aeS = str->read_item(&sep);
  assert(aeS == AMI_ERROR_NO_ERROR || aeS == AMI_ERROR_END_OF_STREAM);

  for(dimension_type i=0; i<rows; i++) {
	for(dimension_type j=0; j<cols; j++) {
	  ae = grid->read_item(&gep);
	  assert(ae == AMI_ERROR_NO_ERROR);
	  if((aeS == AMI_ERROR_NO_ERROR) && (sep->i == i) && (sep->j == j)) {
		ae = mergeStr->write_item(fo(*sep));
		assert(ae == AMI_ERROR_NO_ERROR);
		aeS = str->read_item(&sep);
		assert(aeS == AMI_ERROR_NO_ERROR || aeS == AMI_ERROR_END_OF_STREAM);
	  } else {
		ae = mergeStr->write_item(fo(*gep));
		assert(ae == AMI_ERROR_NO_ERROR);
	  }
	}
  }

  return mergeStr;
}




#endif
