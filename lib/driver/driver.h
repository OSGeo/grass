
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
    void (*Box_abs) (int, int, int, int);
    void (*Box_rel) (int, int);
    void (*Client_Open) (void);
    void (*Client_Close) (void);
    void (*Erase) (void);
    int (*Get_with_box) (int, int, int *, int *, int *);
    int (*Get_with_line) (int, int, int *, int *, int *);
    int (*Get_with_pointer) (int *, int *, int *);
    int (*Graph_set) (int, char **);
    void (*Graph_close) (void);
    void (*Line_width) (int);
    void (*Panel_save) (const char *, int, int, int, int);
    void (*Panel_restore) (const char *);
    void (*Panel_delete) (const char *);
    void (*Polydots_abs) (const int *, const int *, int);
    void (*Polydots_rel) (const int *, const int *, int);
    void (*Polyline_abs) (const int *, const int *, int);
    void (*Polyline_rel) (const int *, const int *, int);
    void (*Polygon_abs) (const int *, const int *, int);
    void (*Polygon_rel) (const int *, const int *, int);
    void (*Set_window) (int, int, int, int);
    void (*Begin_scaled_raster) (int, int[2][2], int[2][2]);
    int (*Scaled_raster) (int, int,
			  const unsigned char *,
			  const unsigned char *,
			  const unsigned char *, const unsigned char *);
    void (*End_scaled_raster) (void);
    void (*Respond) (void);
    int (*Work_stream) (void);
    void (*Do_work) (int);

    int (*lookup_color) (int, int, int);
    void (*color) (int);
    void (*draw_line) (int, int, int, int);
    void (*draw_point) (int, int);
    void (*draw_bitmap) (int, int, int, const unsigned char *);
    void (*draw_text) (const char *);
};

/* Library Functions */

/* command.c */
extern int LIB_command_get_input(void);

/* init.c */
extern int LIB_init(const struct driver *drv, int argc, char **argv);

/* main.c */
extern int LIB_main(int argc, char **argv);

/* Commands */

/* Bitmap.c */
extern void COM_Bitmap(int, int, int, const unsigned char *);

/* Box.c */
extern void COM_Box_abs(int, int, int, int);
extern void COM_Box_rel(int, int);

/* Client.c */
extern void COM_Client_Open(void);
extern void COM_Client_Close(void);

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

/* Get_location.c */
extern int COM_Get_location_with_box(int, int, int *, int *, int *);
extern int COM_Get_location_with_line(int, int, int *, int *, int *);
extern int COM_Get_location_with_pointer(int *, int *, int *);

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

/* Panel.c */
extern void COM_Panel_save(const char *, int, int, int, int);
extern void COM_Panel_restore(const char *);
extern void COM_Panel_delete(const char *);

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

/* Work.c */
extern int COM_Has_work(void);
extern int COM_Work_stream(void);
extern void COM_Do_work(int);

/* Driver Operations */

/* Color.c */
extern int DRV_lookup_color(int, int, int);
extern void DRV_color(int);

/* Draw.c */
extern void DRV_draw_bitmap(int, int, int, const unsigned char *);
extern void DRV_draw_line(int x0, int y0, int x1, int y1);
extern void DRV_draw_point(int x, int y);

#endif /* _DRIVER_H */
