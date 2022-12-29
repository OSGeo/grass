/*!
  \file cairodriver/cairodriver.h

  \brief GRASS cairo display driver - header file

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#ifndef __CAIRODRIVER_H__
#define __CAIRODRIVER_H__

#include <grass/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cairo.h>

#if !defined(USE_X11) || !CAIRO_HAS_XLIB_SURFACE || CAIRO_VERSION < CAIRO_VERSION_ENCODE(1,6,0)
#undef CAIRO_HAS_XLIB_XRENDER_SURFACE
#define CAIRO_HAS_XLIB_XRENDER_SURFACE 0
#endif

#if CAIRO_HAS_XLIB_XRENDER_SURFACE
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#endif

#include <grass/gis.h>

#include "driver.h"
#include "driverlib.h"

#define DEFAULT_FILE_NAME "map.png"

#define HEADER_SIZE 64

/* Scale for converting colors from [0..255] to cairo's [0.0..1.0] */
#define COLORSCALE (1.0/255.0)
#define CAIROCOLOR(a) (((double)(a))*COLORSCALE)

/* File types */
#define FTYPE_UNKNOWN 0
#define FTYPE_PPM 1
#define FTYPE_BMP 2
#define FTYPE_PNG 3
#define FTYPE_PDF 4
#define FTYPE_PS  5
#define FTYPE_SVG 6
#define FTYPE_X11 7

extern cairo_surface_t *surface;
extern cairo_t *cairo;

struct cairo_state {
    char *file_name;
    int file_type;
    int width, height, stride;
    unsigned char *grid;
    double bgcolor_r, bgcolor_g, bgcolor_b, bgcolor_a;
    int modified;
    int mapped;
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    Display *dpy;
    Drawable win;
    Screen *screen;
    XRenderPictFormat *format;
    int depth;
#endif
};

extern struct cairo_state ca;

extern const struct driver *Cairo_Driver(void);

extern void Cairo_Client_Close(void);
extern int Cairo_Graph_set(void);
extern const char *Cairo_Graph_get_file(void);
extern void Cairo_Graph_close(void);
extern void Cairo_Box(double, double, double, double);
extern void Cairo_Set_window(double, double, double, double);
extern void Cairo_Bitmap(int, int, int, const unsigned char *);
extern void Cairo_Color(int, int, int);
extern void Cairo_Erase(void);
extern void Cairo_begin_raster(int, int[2][2], double[2][2]);
extern int Cairo_raster(int, int,
			const unsigned char *, const unsigned char *,
			const unsigned char *, const unsigned char *);
extern void Cairo_end_raster(void);
extern void Cairo_Begin(void);
extern void Cairo_Move(double, double);
extern void Cairo_Cont(double, double);
extern void Cairo_Close(void);
extern void Cairo_Stroke(void);
extern void Cairo_Fill(void);
extern void Cairo_Point(double, double);
extern void Cairo_Line_width(double);
extern void Cairo_Text(const char *);
extern void Cairo_text_box(const char *, double *, double *, double *, double *);
extern void Cairo_set_font(const char *);
extern void Cairo_font_list(char ***, int *);
extern void Cairo_font_info(char ***, int *);

/* read.c */
extern void cairo_read_image(void);
extern void cairo_read_ppm(void);
extern void cairo_read_bmp(void);
extern void cairo_read_xid(void);

/* write.c */
extern void cairo_write_image(void);
extern void cairo_write_ppm(void);
extern void cairo_write_bmp(void);
extern void cairo_write_xid(void);

#endif /* __CAIRODRIVER_H__ */
