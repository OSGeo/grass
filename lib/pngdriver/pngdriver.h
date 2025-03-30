/*!
   \file lib/pngdriver/pngdriver.h

   \brief GRASS png display driver - header file

   (C) 2007-2014 by Glynn Clements and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Glynn Clements
 */

#ifndef __PNGDRIVER_H__
#define __PNGDRIVER_H__

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <grass/config.h>
#include "driver.h"
#include "path.h"

#define FILE_NAME   "map.png"

#define HEADER_SIZE 64

struct png_state {
    char *file_name;
    int current_color;
    int true_color;
    int has_alpha;
    int mapped;
#ifdef _WIN32
    HANDLE handle;
#endif

    double clip_top, clip_bot, clip_left, clip_rite;
    int width, height;
    unsigned int *grid;
    unsigned char palette[256][4];
    unsigned int background;
    int modified;

    int linewidth;
};

extern struct png_state png;

extern void read_image(void);
extern void read_ppm(void);
extern void read_pgm(void);
extern void read_bmp(void);
#ifdef HAVE_PNG_H
extern void read_png(void);
#endif

extern void write_image(void);
extern void write_ppm(void);
extern void write_pgm(void);
extern void write_bmp(void);
#ifdef HAVE_PNG_H
extern void write_png(void);
#endif

extern void png_init_color_table(void);
extern unsigned int png_get_color(int, int, int, int);
extern void png_get_pixel(unsigned int, int *, int *, int *, int *);
extern void png_draw_line(double, double, double, double);
extern void png_polygon(struct path *);

extern const struct driver *PNG_Driver(void);

extern void PNG_Box(double, double, double, double);
extern void PNG_Client_Close(void);
extern void PNG_Erase(void);
extern void PNG_Graph_close(void);
extern int PNG_Graph_set(void);
extern const char *PNG_Graph_get_file(void);
extern void PNG_Line_width(double);
extern void PNG_begin_raster(int, int[2][2], double[2][2]);
extern int PNG_raster(int, int, const unsigned char *, const unsigned char *,
                      const unsigned char *, const unsigned char *);
extern void PNG_Begin(void);
extern void PNG_Move(double, double);
extern void PNG_Cont(double, double);
extern void PNG_Close(void);
extern void PNG_Stroke(void);
extern void PNG_Fill(void);
extern void PNG_Point(double, double);
extern void PNG_Set_window(double, double, double, double);
extern void PNG_color_rgb(int, int, int);
extern void PNG_draw_bitmap(int, int, int, const unsigned char *);

#endif /* __PNGDRIVER_H__ */
