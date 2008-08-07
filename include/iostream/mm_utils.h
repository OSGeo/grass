/****************************************************************************
 * 
 *  MODULE:	iostream
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

#ifndef MM_UTIL_H
#define MM_UTIL_H

#include "mm.h"
#include <string>


void  LOG_avail_memo();

size_t getAvailableMemory();

void  MEMORY_LOG(std::string str);

#endif
