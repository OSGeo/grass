/*!
  \file lib/pngdriver/graph_close.c

  \brief GRASS png display driver - close graphics processing

  (C) 2003-2014 by Glynn Clements and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Per Henrik Johansen (original contributor)
  \author Glynn Clements
*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __MINGW32__
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include <grass/gis.h>
#include "pngdriver.h"

static void unmap_file(void)
{
    size_t size = HEADER_SIZE + png.width * png.height * sizeof(unsigned int);
    void *ptr = (char *)png.grid - HEADER_SIZE;

    if (!png.mapped)
	return;

#ifdef __MINGW32__
    UnmapViewOfFile(ptr);
    CloseHandle(png.handle);
#else
    munmap(ptr, size);
#endif

    png.mapped = 0;
}

/*!
  \brief Close down the graphics processing. This gets called only at driver
         termination time.
*/
void PNG_Graph_close(void)
{
    write_image();

    if (png.mapped)
	unmap_file();
    else
	G_free(png.grid);
}
