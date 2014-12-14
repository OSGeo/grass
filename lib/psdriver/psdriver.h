#ifndef __PSDRIVER_H__
#define __PSDRIVER_H__

#include <stdio.h>

#include <grass/config.h>
#include "driver.h"

#define FILE_NAME  "map.ps"

struct ps_state
{
    FILE *outfp;
    int true_color;
    int encapsulated;
    int no_header, no_trailer;
    double left, right, bot, top;
};

extern struct ps_state ps;

extern void output(const char *, ...);

extern void init_color_table(void);
extern int lookup_color(int, int, int);

extern const struct driver *PS_Driver(void);

extern void PS_Box(double, double, double, double);
extern void PS_Client_Close(void);
extern void PS_Erase(void);
extern void PS_Graph_close(void);
extern int PS_Graph_set(void);
extern const char *PS_Graph_get_file(void);
extern void PS_Line_width(double);
extern void PS_Set_window(double, double, double, double);
extern void PS_Color(int, int, int);
extern void PS_Bitmap(int, int, int, const unsigned char *);
extern void PS_begin_raster(int, int[2][2], double[2][2]);
extern int PS_raster(int, int, const unsigned char *,
		     const unsigned char *, const unsigned char *,
		     const unsigned char *);
extern void PS_end_raster(void);
extern void PS_Begin(void);
extern void PS_Move(double, double);
extern void PS_Cont(double, double);
extern void PS_Close(void);
extern void PS_Stroke(void);
extern void PS_Fill(void);
extern void PS_Point(double, double);

#endif /* __PSDRIVER_H__ */
