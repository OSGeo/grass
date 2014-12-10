/*!
  \file lib/pngdriver/graph_set.c

  \brief GRASS png display driver - set graphics processing

  (C) 2003-2014 by Glynn Clements and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Per Henrik Johansen (original contributor)
  \author Glynn Clements
*/

#include <string.h>
#include <stdlib.h>
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
#include <grass/colors.h>
#include <grass/glocale.h>
#include "pngdriver.h"

struct png_state png;

static void map_file(void)
{
    size_t size = HEADER_SIZE + png.width * png.height * sizeof(unsigned int);
    void *ptr;
    int fd;

    fd = open(png.file_name, O_RDWR);
    if (fd < 0)
	return;

#ifdef __MINGW32__
    png.handle = CreateFileMapping((HANDLE) _get_osfhandle(fd),
				   NULL, PAGE_READWRITE,
				   0, size, NULL);
    if (!png.handle)
	return;
    ptr = MapViewOfFile(png.handle, FILE_MAP_WRITE, 0, 0, size);
    if (!ptr)
	return;
#else
    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t) 0);
    if (ptr == MAP_FAILED)
	return;
#endif

    if (png.grid)
	G_free(png.grid);
    png.grid = (unsigned int *)((char *) ptr + HEADER_SIZE);

    close(fd);

    png.mapped = 1;
}

/*!
  \brief Start up graphics processing

   Anything that needs to be assigned, set up,
   started-up, or otherwise initialized happens here.  This is called only at
   the startup of the graphics driver.
 
   The external variables define the pixle limits of the graphics surface.  The
   coordinate system used by the applications programs has the (0,0) origin
   in the upper left-hand corner.  Hence,
     screen_left < screen_right
     screen_top  < screen_bottom 
*/

int PNG_Graph_set(void)
{
    unsigned int red, grn, blu;
    int do_read = 0;
    int do_map = 0;
    char *p;

    G_gisinit("PNG driver");

    p = getenv("GRASS_RENDER_FILE");
    if (!p || strlen(p) == 0)
	p = FILE_NAME;
    G_debug(1, "png: GRASS_RENDER_FILE: %s", p);

    png.file_name = p;

    p = getenv("GRASS_RENDER_TRUECOLOR");
    png.true_color = !p || strcmp(p, "FALSE") != 0;

    G_verbose_message(_("png: truecolor status %s"),
		      png.true_color ? _("enabled") : _("disabled"));

    p = getenv("GRASS_RENDER_FILE_MAPPED");
    do_map = p && strcmp(p, "TRUE") == 0;

    if (do_map) {
	char *ext = png.file_name + strlen(png.file_name) - 4;

	if (G_strcasecmp(ext, ".bmp") != 0)
	    do_map = 0;
    }

    p = getenv("GRASS_RENDER_FILE_READ");
    do_read = p && strcmp(p, "TRUE") == 0;

    if (do_read && access(png.file_name, 0) != 0)
	do_read = 0;

    png.width = screen_width;
    png.height = screen_height;

    png.clip_top = 0;
    png.clip_bot = png.height;
    png.clip_left = 0;
    png.clip_rite = png.width;

    p = getenv("GRASS_RENDER_TRANSPARENT");
    png.has_alpha = p && strcmp(p, "TRUE") == 0;

    png_init_color_table();

    p = getenv("GRASS_RENDER_BACKGROUNDCOLOR");
    if (p && *p && 
	(sscanf(p, "%02x%02x%02x", &red, &grn, &blu) == 3 ||
	 G_str_to_color(p, (int *)&red, (int *)&grn, (int *)&blu) == 1)) {
	png.background = png_get_color(red, grn, blu, png.has_alpha ? 255 : 0);
    }
    else {
	/* 0xffffff = white, 0x000000 = black */
	if (strcmp(DEFAULT_FG_COLOR, "white") == 0)
	    /* foreground: white, background: black */
	    png.background = png_get_color(0, 0, 0, png.has_alpha ? 255 : 0);
	else
	    /* foreground: black, background: white */
	    png.background = png_get_color(255, 255, 255, png.has_alpha ? 255 : 0);
    }
    
    G_verbose_message(_("png: collecting to file '%s'"), png.file_name);
    G_verbose_message(_("png: image size %dx%d"),
		      png.width, png.height);

    if (do_read && do_map)
	map_file();

    if (!png.mapped)
	png.grid = G_malloc(png.width * png.height * sizeof(unsigned int));

    if (!do_read) {
	PNG_Erase();
	png.modified = 1;
    }

    if (do_read && !png.mapped)
	read_image();

    if (do_map && !png.mapped) {
	write_image();
	map_file();
    }

    return 0;
}

/*!
  \brief Get render file

  \return file name
*/
const char *PNG_Graph_get_file(void)
{
    return png.file_name;
}
