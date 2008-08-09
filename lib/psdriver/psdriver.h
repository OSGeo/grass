#ifndef __PSDRIVER_H__
#define __PSDRIVER_H__

#include <stdio.h>

#include <grass/config.h>
#include "driver.h"

#define FILE_NAME  "map.ps"

extern const char *file_name;
extern FILE *outfp;
extern int true_color;
extern int width, height;
extern int encapsulated;
extern int no_header, no_trailer;

extern void output(const char *, ...);

extern void init_color_table(void);

extern const struct driver *PS_Driver(void);

extern void PS_Box(double, double, double, double);
extern void PS_Client_Close(void);
extern void PS_Erase(void);
extern void PS_Graph_close(void);
extern int PS_Graph_set(void);
extern void PS_Line_width(double);
extern void PS_Respond(void);
extern void PS_Set_window(double, double, double, double);
extern void PS_color(int);
extern void PS_draw_bitmap(int, int, int, const unsigned char *);
extern void PS_draw_line(double, double, double, double);
extern void PS_draw_point(double, double);
extern int PS_lookup_color(int, int, int);
extern void PS_begin_scaled_raster(int, int[2][2], double[2][2]);
extern int PS_scaled_raster(int, int, const unsigned char *,
			    const unsigned char *, const unsigned char *,
			    const unsigned char *);
extern void PS_end_scaled_raster(void);
extern void PS_Polygon(const double *, const double *, int);
extern void PS_Polyline(const double *, const double *, int);

#endif /* __PSDRIVER_H__ */
