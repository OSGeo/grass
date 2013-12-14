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


#include <sys/types.h>
#include <ctype.h>
#include <ostream>
#include <iostream>
#include <stdio.h>

//#include <mm.h>
#include <grass/iostream/mm.h>

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
MEMORY_LOG(std::string str) {
  printf("%s", str.c_str());
  fflush(stdout);
}
