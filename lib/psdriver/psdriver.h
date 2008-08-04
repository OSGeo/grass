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

extern void PS_Box_abs(int, int, int, int);
extern void PS_Client_Close(void);
extern void PS_Erase(void);
extern void PS_Graph_close(void);
extern int PS_Graph_set(int, char **);
extern void PS_Line_width(int);
extern void PS_Respond(void);
extern void PS_Set_window(int, int, int, int);
extern void PS_color(int);
extern void PS_draw_bitmap(int, int, int, const unsigned char *);
extern void PS_draw_line(int, int, int, int);
extern void PS_draw_point(int, int);
extern int PS_lookup_color(int, int, int);
extern void PS_begin_scaled_raster(int, int[2][2], int[2][2]);
extern int PS_scaled_raster(int, int, const unsigned char *,
			    const unsigned char *, const unsigned char *,
			    const unsigned char *);
extern void PS_end_scaled_raster(void);
extern void PS_Polygon_abs(const int *, const int *, int);
extern void PS_Polyline_abs(const int *, const int *, int);

#endif /* __PSDRIVER_H__ */
