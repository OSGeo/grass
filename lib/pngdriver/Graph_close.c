/*
 * Close down the graphics processing.  This gets called only at driver
 * termination time.
 */

#ifndef __MINGW32__
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#include <grass/gis.h>
#include "pngdriver.h"

static void unmap_file(void)
{
#ifndef __MINGW32__
    size_t size = HEADER_SIZE + width * height * sizeof(unsigned int);
    void *ptr = (char *)grid - HEADER_SIZE;

    if (!mapped)
	return;

    munmap(ptr, size);

    mapped = 0;
#endif
}

void PNG_Graph_close(void)
{
    write_image();

    if (mapped)
	unmap_file();
    else
	G_free(grid);
}
