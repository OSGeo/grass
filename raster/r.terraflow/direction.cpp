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


#include "direction.h"
#include "nodata.h"
#include "common.h" /*for global opt->d8 flag*/

#define TF_ROOTTWO 1.4142135623

/* directions:
   32 64 128
   16 *   1
   8  4   2
*/
direction_type 
encodeDirection(const genericWindow<elevation_type>& elevwin,
				const dimension_type nrows, const dimension_type ncols,
				dimension_type row, 
				dimension_type col) {
  
  /*check global opt flag and call appropriate model*/
	if(opt->d8){
    return encodeDirectionSFD(elevwin, nrows, ncols, row, col);
	}
  return encodeDirectionMFD(elevwin, nrows, ncols, row, col);
}

/***************************************************************/
/* returns the direction corresponding to the window using MFD */
direction_type 
encodeDirectionMFD(const genericWindow<elevation_type>& elevwin,
				const dimension_type nrows, const dimension_type ncols,
				dimension_type row, 
				dimension_type col) {
  
  direction_type dir = DIRECTION_UNDEF;
  
  if(!is_nodata(elevwin.get())) {
    dir = 0;
    if (elevwin.get(5) < elevwin.get() && !is_void(elevwin.get(5))) dir |= 1;
    if (elevwin.get(3) < elevwin.get() && !is_void(elevwin.get(3))) dir |= 16;
    for(int i=0; i<3; i++) {
      if(elevwin.get(i) < elevwin.get() && !is_void(elevwin.get(i))) dir |= 32<<i;
      if(elevwin.get(i+6) < elevwin.get() && !is_void(elevwin.get(6+i))) dir |= 8>>i;
    }
  }
  
  /* if no direction, check for boundary */
  if(dir==0 || dir==DIRECTION_UNDEF) {
    if(row==0) {
      dir = 32 | 64 | 128;
    }
    if(row==nrows-1) {
      dir = 2 | 4 | 8;
    }
    if(col==0) {
      if(row==0) dir = 32;
      else if(row==nrows-1) dir = 8;
      else dir = 8 | 16 | 32;
    }
    if(col==ncols-1) {
      if(row==0) dir = 128;
      else if(row==nrows-1) dir = 2;
      else dir = 128 | 1 | 2;
    }
  }
  return dir;
}

/***************************************************************/
/* returns the direction corresponding to the window using SFD */
direction_type 
encodeDirectionSFD(const genericWindow<elevation_type>& elevwin,
				const dimension_type nrows, const dimension_type ncols,
				dimension_type row, 
				dimension_type col) {

  direction_type dir = DIRECTION_UNDEF;
	float tdrop, max_drop;
	int max_indx;
  
  if(!is_nodata(elevwin.get())) {
    dir = 0;
		max_drop = 0;
		max_indx = -1;
		for(int i=0; i<9; i++){
			if(i%2==1){ /*cardinal direction*/
				tdrop=elevwin.get()-elevwin.get(i);
				if(tdrop>max_drop){ max_drop=tdrop; max_indx=i; }
			}
			else if(i!=4){ /*diagonal*/
				tdrop=(elevwin.get()-elevwin.get(i))/TF_ROOTTWO;
				if(tdrop>max_drop){ max_drop=tdrop; max_indx=i; }
			}
		}
		switch(max_indx){
			case 0:
			case 1:
			case 2:
				dir=32<<max_indx; break;
		  case 3:
				dir=16; break;
			case 5:
				dir=1; break;
			case 6:
			case 7:
			case 8:
				dir=8>>(max_indx-6); break;
			default:
				dir=0; break;
		}
  }
  
  /* if no direction, check for boundary */
  if(dir==0 || dir==DIRECTION_UNDEF) {
    if(row==0) {
      dir = 64;
    }
    if(row==nrows-1) {
      dir = 48;
    }
    if(col==0) {
      if(row==0) dir = 32;
      else if(row==nrows-1) dir = 8;
      else dir = 16;
    }
    if(col==ncols-1) {
      if(row==0) dir = 128;
      else if(row==nrows-1) dir = 2;
      else dir = 1;
    }
  }
  return dir;
}

direction_type
findDominant(direction_type dir) {
  switch(dir) {
  case 1:
  case 2:
  case 4:
  case 8:
  case 16:
  case 32:
  case 64:
  case 128:
	return dir;

  case 1+2:
  case 128+1:
	return 1;
  case 2+4:
  case 4+8:
	return 4;
  case 8+16:
  case 16+32:
	return 16;
  case 32+64:
  case 64+128:
	return 64;

  case 1+2+4:
	return 2;
  case 2+4+8:
	return 4;
  case 4+8+16:
	return 8;
  case 8+16+32:
	return 16;
  case 16+32+64:
	return 32;
  case 32+64+128:
	return 64;
  case 64+128+1:
	return 128;
  case 128+1+2:
	return 1;

  case 128+1+2+4:
  case 64+128+1+2:
	return 1;
  case 1+2+4+8:
  case 2+4+8+16:
	return 4;
  case 8+16+32+64:
  case 4+8+16+32:
	return 16;
  case 32+64+128+1:
  case 16+32+64+128:
	return 64;
	
  case 64+128+1+2+4:
	return 1;
  case 128+1+2+4+8:
	return 2;
  case 1+2+4+8+16:
	return 4;
  case 2+4+8+16+32:
	return 8;
  case 4+8+16+32+64:
	return 16;
  case 8+16+32+64+128:
	return 32;
  case 16+32+64+128+1:
	return 64;
  case 32+64+128+1+2:
	return 128;
  }

  /* Otherwise, there is no dominant direction.
	 * SFD must output a single direction 1,2,4,8,16,32,64, or 128
	 * pick the first matching one, with preference to cardinal direction
	 */
	if(dir & 85 ){ 
		/* at least one cardinal direction (1+4+16+64=E+S+W+N) matches */
		if(dir & 1){ return 1;}
		if(dir & 4){ return 4;}
		if(dir & 16){ return 16;}
		if(dir & 64){ return 64;}
	}
	else{ 
		/* 2 8 32 128 = SE SW NW NE*/
		if(dir & 2){ return 2;}
		if(dir & 8){ return 8;}
		if(dir & 32){ return 32;}
		if(dir & 128){ return 128;}
  }
  return dir; /* shouldn't get here unless dir <= 0 */
}


char
directionSymbol(direction_type dir) {
  char c='?';
  int cnt=0;
  char symbols[] = ">\\v/<\\^/";

  if(dir == 0) return '.';
  
  dir = findDominant(dir);

  for(int i=0; i<8; i++) {
	if(dir & (1<<i)) {
	  cnt++;
	  c = symbols[i];
	}
  }  
  if(cnt>1) c = 'X';

  switch(dir) {
  case 1+16:
  case 128+1+2+8+16+32:
	c = '-';
	break;
  case 1+2+8+16+32:
  case 128+1+8+16+32:
	c = '<';
	break;
  case 128+1+2+16+32:
  case 128+1+2+8+16:
	c = '>';
	break;
  case 4+64:
  case 2+4+8+32+64+128:
	c = '|';
	break;
  case 4+8+32+64+128:
  case 2+4+32+64+128:
	c = '^';
	break;
  case 2+4+8+64+128:
  case 2+4+8+32+64:
	c = 'v';
	break;
  case 255:
	c = '*';
	break;
  default:
	break;
  }
  return c;
} 


#undef TF_ROOTTWO
