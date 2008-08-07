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

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

//#include <rtimer.h>
#include <grass/iostream/rtimer.h>

char *
rt_sprint_safe(char *buf, Rtimer rt) {
  if(rt_w_useconds(rt) == 0) {
	sprintf(buf, "[%4.2fu (%.0f%%) %4.2fs (%.0f%%) %4.2f %.1f%%]",
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  } else {
	sprintf(buf, "[%4.2fu (%.0f%%) %4.2fs (%.0f%%) %4.2f %.1f%%]",
			rt_u_useconds(rt)/1000000,
			100.0*rt_u_useconds(rt)/rt_w_useconds(rt),
			rt_s_useconds(rt)/1000000,
			100.0*rt_s_useconds(rt)/rt_w_useconds(rt),
			rt_w_useconds(rt)/1000000,
			100.0*(rt_u_useconds(rt)+rt_s_useconds(rt)) / rt_w_useconds(rt));
  }
  return buf;
}




