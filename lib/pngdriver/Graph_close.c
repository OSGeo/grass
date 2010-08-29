/*
 * Close down the graphics processing.  This gets called only at driver
 * termination time.
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

void PNG_Graph_close(void)
{
    write_image();

    if (png.mapped)
	unmap_file();
    else
	G_free(png.grid);
}
