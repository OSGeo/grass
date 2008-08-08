
#ifndef _DRIVER_H
#define _DRIVER_H

#include <grass/freetypecap.h>

extern int NCOLORS;

extern int screen_left;
extern int screen_right;
extern int screen_bottom;
extern int screen_top;

extern int cur_x;
extern int cur_y;

extern double text_size_x;
extern double text_size_y;
extern double text_rotation;

extern int mouse_button[];

extern struct GFONT_CAP *ftcap;

struct driver
{
    void (*Box) (int, int, int, int);
    void (*Erase) (void);
    int (*Graph_set) (int, char **);
    void (*Graph_close) (void);
    void (*Line_width) (int);
    void (*Polydots) (const int *, const int *, int);
    void (*Polyline) (const int *, const int *, int);
    void (*Polygon) (const int *, const int *, int);
    void (*Set_window) (int, int, int, int);
    void (*Begin_scaled_raster) (int, int[2][2], int[2][2]);
    int (*Scaled_raster) (int, int,
			  const unsigned char *,
			  const unsigned char *,
			  const unsigned char *, const unsigned char *);
    void (*End_scaled_raster) (void);
    void (*Respond) (void);

    int (*lookup_color) (int, int, int);
    void (*color) (int);
    void (*draw_line) (int, int, int, int);
    void (*draw_point) (int, int);
    void (*draw_bitmap) (int, int, int, const unsigned char *);
    void (*draw_text) (const char *);
};

/* Library Functions */

/* init.c */
extern int LIB_init(const struct driver *drv, int argc, char **argv);

/* Commands */

/* Bitmap.c */
extern void COM_Bitmap(int, int, int, const unsigned char *);

/* Box.c */
extern void COM_Box_abs(int, int, int, int);
extern void COM_Box_rel(int, int);

/* Color.c */
extern void COM_Color_RGB(unsigned char, unsigned char, unsigned char);
extern void COM_Standard_color(int);

/* Cont.c */
extern void COM_Cont_abs(int, int);
extern void COM_Cont_rel(int, int);

/* Erase.c */
extern void COM_Erase(void);

/* Font.c */
extern void COM_Font_get(const char *);
extern void COM_Font_init_charset(const char *);
extern void COM_Font_list(char ***, int *);
extern void COM_Font_info(char ***, int *);

/* Get_t_box.c */
extern void COM_Get_text_box(const char *, int *, int *, int *, int *);

/* Graph.c */
extern int COM_Graph_set(int, char **);
extern void COM_Graph_close(void);

/* Line_width.c */
extern void COM_Line_width(int);

/* Move.c */
extern void COM_Move_abs(int, int);
extern void COM_Move_rel(int, int);

/* Polydots.c */
extern void COM_Polydots_abs(const int *, const int *, int);
extern void COM_Polydots_rel(const int *, const int *, int);

/* Polygon.c */
extern void COM_Polygon_abs(const int *, const int *, int);
extern void COM_Polygon_rel(const int *, const int *, int);

/* Polyline.c */
extern void COM_Polyline_abs(const int *, const int *, int);
extern void COM_Polyline_rel(const int *, const int *, int);

/* Raster.c */
extern void COM_begin_scaled_raster(int, int[2][2], int[2][2]);
extern int COM_scaled_raster(int, int, const unsigned char *,
			     const unsigned char *, const unsigned char *,
			     const unsigned char *);
extern void COM_end_scaled_raster(void);

/* Respond.c */
extern void COM_Respond(void);

/* Returns.c */
extern void COM_Screen_left(int *);
extern void COM_Screen_rite(int *);
extern void COM_Screen_bot(int *);
extern void COM_Screen_top(int *);
extern void COM_Number_of_colors(int *);

/* Set_window.c */
extern void COM_Set_window(int, int, int, int);

/* Text.c */
extern void COM_Text(const char *);

/* Text_size.c */
extern void COM_Text_size(int, int);
extern void COM_Text_rotation(double);

/* Driver Operations */

/* Color.c */
extern int DRV_lookup_color(int, int, int);
extern void DRV_color(int);

/* Draw.c */
extern void DRV_draw_bitmap(int, int, int, const unsigned char *);
extern void DRV_draw_line(int x0, int y0, int x1, int y1);
extern void DRV_draw_point(int x, int y);

#endif /* _DRIVER_H */
