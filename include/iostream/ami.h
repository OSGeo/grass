
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

#ifndef _AMI_H
#define _AMI_H

//debug flags
#include "ami_config.h"

//typedefs, stream
#include "ami_stream.h"

//memory manager
#include "mm.h"
#include "mm_utils.h"

#include "ami_sort.h"

//data structures
#include "queue.h"
#include "pqheap.h"
//#include "empq.h"
#include "empq_impl.h"
//#include "empq_adaptive.h"
#include "empq_adaptive_impl.h"

//timer
#include "rtimer.h"

#endif // _AMI_H 
