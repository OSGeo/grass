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

#include <grass/iostream/ami.h>
#include <grass/iostream/minmaxheap.h>

#if(0)
#define TEST_SIZE (1<<20)

int main() {
  int m = TEST_SIZE/100;
  MinMaxHeap<int> foo(TEST_SIZE);

  for(int i=0; i<TEST_SIZE; i++) {
	foo.insert(i);
  }
  int z;
  cout << " ------------------------------" << endl;
  for(int i=0; i<TEST_SIZE; i++) {
	bool r;
	r = foo.extract_min(z);
	r = foo.extract_max(z);
	if(i%m == 0) {
	  cerr << i << endl;
	}
  }

}
#endif
