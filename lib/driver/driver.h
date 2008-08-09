
#ifndef _DRIVER_H
#define _DRIVER_H

#include <grass/freetypecap.h>

extern int NCOLORS;

extern int screen_left;
extern int screen_right;
extern int screen_bottom;
extern int screen_top;

extern double cur_x;
extern double cur_y;

extern double text_size_x;
extern double text_size_y;
extern double text_rotation;

extern struct GFONT_CAP *ftcap;

struct driver
{
    void (*Box) (double, double, double, double);
    void (*Erase) (void);
    int (*Graph_set) (void);
    void (*Graph_close) (void);
    void (*Line_width) (double);
    void (*Polydots) (const double *, const double *, int);
    void (*Polyline) (const double *, const double *, int);
    void (*Polygon) (const double *, const double *, int);
    void (*Set_window) (double, double, double, double);
    void (*Begin_scaled_raster) (int, int[2][2], double[2][2]);
    int (*Scaled_raster) (int, int,
			  const unsigned char *,
			  const unsigned char *,
			  const unsigned char *, const unsigned char *);
    void (*End_scaled_raster) (void);
    void (*Respond) (void);

    int (*lookup_color) (int, int, int);
    void (*color) (int);
    void (*draw_line) (double, double, double, double);
    void (*draw_point) (double, double);
    void (*draw_bitmap) (int, int, int, const unsigned char *);
    void (*draw_text) (const char *);
};

/* Library Functions */

/* init.c */
extern int LIB_init(const struct driver *drv);

/* Commands */

/* Bitmap.c */
extern void COM_Bitmap(int, int, int, const unsigned char *);

/* Box.c */
extern void COM_Box_abs(double, double, double, double);
extern void COM_Box_rel(double, double);

/* Color.c */
extern void COM_Color_RGB(unsigned char, unsigned char, unsigned char);
extern void COM_Standard_color(int);

/* Cont.c */
extern void COM_Cont_abs(double, double);
extern void COM_Cont_rel(double, double);

/* Erase.c */
extern void COM_Erase(void);

/* Font.c */
extern void COM_Font_get(const char *);
extern void COM_Font_init_charset(const char *);
extern void COM_Font_list(char ***, int *);
extern void COM_Font_info(char ***, int *);

/* Get_t_box.c */
extern void COM_Get_text_box(const char *, double *, double *, double *, double *);

/* Graph.c */
extern int COM_Graph_set(void);
extern void COM_Graph_close(void);

/* Line_width.c */
extern void COM_Line_width(double);

/* Move.c */
extern void COM_Move_abs(double, double);
extern void COM_Move_rel(double, double);

/* Polydots.c */
extern void COM_Polydots_abs(const double *, const double *, int);
extern void COM_Polydots_rel(const double *, const double *, int);

/* Polygon.c */
extern void COM_Polygon_abs(const double *, const double *, int);
extern void COM_Polygon_rel(const double *, const double *, int);

/* Polyline.c */
extern void COM_Polyline_abs(const double *, const double *, int);
extern void COM_Polyline_rel(const double *, const double *, int);

/* Raster.c */
extern void COM_begin_scaled_raster(int, int[2][2], double[2][2]);
extern int COM_scaled_raster(int, int, const unsigned char *,
			     const unsigned char *, const unsigned char *,
			     const unsigned char *);
extern void COM_end_scaled_raster(void);

/* Respond.c */
extern void COM_Respond(void);

/* Returns.c */
extern double COM_Screen_left(void);
extern double COM_Screen_rite(void);
extern double COM_Screen_bot(void);
extern double COM_Screen_top(void);
extern int COM_Number_of_colors(void);

/* Set_window.c */
extern void COM_Set_window(double, double, double, double);

/* Text.c */
extern void COM_Text(const char *);

/* Text_size.c */
extern void COM_Text_size(double, double);
extern void COM_Text_rotation(double);

/* Driver Operations */

/* Color.c */
extern int DRV_lookup_color(int, int, int);
extern void DRV_color(int);

/* Draw.c */
extern void DRV_draw_bitmap(int, int, int, const unsigned char *);
extern void DRV_draw_line(double, double, double, double);
extern void DRV_draw_point(double, double);

#endif /* _DRIVER_H */
