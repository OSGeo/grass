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

#ifndef SORTUTILS_H
#define SORTUTILS_H

#include <fstream>

#include <grass/iostream/ami.h>
#include "common.h"




/* ********************************************************************** */
/* deletes input stream *str and replaces it by the sorted stream */

template<class T, class FUN>
void
sort(AMI_STREAM<T> **str, FUN fo) {
  Rtimer rt;
  AMI_STREAM<T> *sortedStr;

  if (stats)
    stats->recordLength("pre-sort", *str);
  rt_start(rt);

  /* let AMI_sort create its output stream and delete the inout stream */
  int eraseInputStream = 1;
  AMI_sort(*str,&sortedStr, &fo, eraseInputStream);
  rt_stop(rt);

  if (stats) {
      stats->recordLength("sort", sortedStr);
      stats->recordTime("sort", rt);
  }

  sortedStr->seek(0);
  *str = sortedStr;

}





/* ********************************************************************** */

/* warning - creates a new stream and returns it !! */
template<class T, class FUN>
AMI_STREAM<T> *
sort(AMI_STREAM<T> *strIn, FUN fo) {
  Rtimer rt;
  AMI_STREAM<T> *strOut;

  if (stats)
    stats->recordLength("pre-sort", strIn);
  rt_start(rt);

  AMI_sort(strIn, &strOut, &fo);
  assert(strOut);

  rt_stop(rt);
  if (stats) {
      stats->recordLength("sort", strOut);
      stats->recordTime("sort", rt);
  }

  strOut->seek(0);
  return strOut;
}

#endif

