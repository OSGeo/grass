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

#include <sys/types.h>
#include <ctype.h>

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#include <ostream>
#else
#include <ostream.h>
#endif

#include <iostream>
using namespace std;
#include <stdio.h>

#include <mm.h>


void 
LOG_avail_memo() {
  size_t sz_avail=0;
  sz_avail = MM_manager.memory_available();
  printf("available memory: %.2fMB\n", sz_avail/(float)(1<<20));
}

size_t
getAvailableMemory() {
  size_t fmem;
  fmem = MM_manager.memory_available();
  return fmem;
}

void 
MEMORY_LOG(char* str) {
  printf("%s", str);
  fflush(stdout);
}
