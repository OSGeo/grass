
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

#ifndef _ami_config_h
#define _ami_config_h



//CHOOSE PQUEUE IMPLEMENTATION 
//------------------------------------------------------------
//#define IM_PQUEUE
//#define EM_PQUEUE
#define EMPQ_ADAPTIVE


//maximize memory usage by keeping streams on disk
//------------------------------------------------------------
#if (defined EM_PQUEUE || defined EMPQ_ADAPTIVE)
//enables keeping streams on disk, rather than in memory;
#define SAVE_MEMORY
#endif


#if (defined EMPQ_ADAPTIVE && !defined SAVE_MEMORY)
#error  EMPQ_ADAPTIVE requires SAVE_MEMORY set
#endif

#endif
