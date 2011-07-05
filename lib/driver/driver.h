
#ifndef _DRIVER_H
#define _DRIVER_H

#include <grass/fontcap.h>

extern int screen_width;
extern int screen_height;

extern double cur_x;
extern double cur_y;

extern double text_size_x;
extern double text_size_y;
extern double text_rotation;
extern double text_sinrot;
extern double text_cosrot;
extern int matrix_valid;

extern struct GFONT_CAP *ftcap;

struct driver
{
    char *name;
    
    void (*Box)(double, double, double, double);
    void (*Erase)(void);
    int (*Graph_set)(void);
    void (*Graph_close)(void);
    void (*Line_width)(double);
    void (*Set_window)(double, double, double, double);
    void (*Begin_raster)(int, int[2][2], double[2][2]);
    int (*Raster)(int, int,
		  const unsigned char *,
		  const unsigned char *,
		  const unsigned char *,
		  const unsigned char *);
    void (*End_raster)(void);
    void (*Begin)(void);
    void (*Move)(double, double);
    void (*Cont)(double, double);
    void (*Close)(void);
    void (*Stroke)(void);
    void (*Fill)(void);
    void (*Point)(double, double);

    void (*Color)(int, int, int);
    void (*Bitmap)(int, int, int, const unsigned char *);
    void (*Text)(const char *);
    void (*Text_box)(const char *, double *, double *, double *, double *);
    void (*Set_font)(const char *);
    void (*Font_list)(char ***, int *);
    void (*Font_info)(char ***, int *);
};

/* Library Functions */

/* init.c */
extern void LIB_init(const struct driver *drv);

/* Commands */

/* Box.c */
extern void COM_Box_abs(double, double, double, double);

/* Color.c */
extern void COM_Color_RGB(unsigned char, unsigned char, unsigned char);
extern void COM_Standard_color(int);

/* Erase.c */
extern void COM_Erase(void);

/* Font.c */
extern void COM_Set_font(const char *);
extern void COM_Set_encoding(const char *);
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
extern void COM_Pos_abs(double, double);

/* Raster.c */
extern void COM_begin_raster(int, int[2][2], double[2][2]);
extern int COM_raster(int, int, const unsigned char *,
		      const unsigned char *, const unsigned char *,
		      const unsigned char *);
extern void COM_end_raster(void);

/* Set_window.c */
extern void COM_Set_window(double, double, double, double);
extern void COM_Get_window(double *, double *, double *, double *);

/* Text.c */
extern void COM_Text(const char *);

/* Text_size.c */
extern void COM_Text_size(double, double);
extern void COM_Text_rotation(double);

/* Driver Operations */

/* Draw.c */
extern void COM_Bitmap(int, int, int, const unsigned char *);
extern void COM_Begin(void);
extern void COM_Move(double, double);
extern void COM_Cont(double, double);
extern void COM_Close(void);
extern void COM_Stroke(void);
extern void COM_Fill(void);
extern void COM_Point(double, double);

#endif /* _DRIVER_H */
