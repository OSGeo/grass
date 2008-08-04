#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/gis.h>


/* add_point.c */
int add_point(int, int);

/* analyze_sig.c */
int analyze_sig(void);

/* band_files.c */
int open_band_files(void);
int close_band_files(void);

/* cellhd.c */
int Outline_cellhd(View *, struct Cell_head *);

/* complete_reg.c */
int complete_region(void);

/* conv.c */
int view_to_col(View *, int);
int view_to_row(View *, int);
int col_to_view(View *, int);
int row_to_view(View *, int);
double row_to_northing(struct Cell_head *, int, double);
double col_to_easting(struct Cell_head *, int, double);

/* curses.c */
int Begin_curses(void);
int End_curses(void);
int Suspend_curses(void);
int Resume_curses(void);
int Curses_allow_interrupts(int);
int Curses_clear_window(Window *);
int Curses_outline_window(Window *);
int Curses_write_window(Window *, int, int, char *);
int Curses_replot_screen(void);
int Curses_prompt_gets(char *, char *);
int Curses_getch(int);

/* define_reg.c */
int define_region(void);

/* del_point.c */
int del_point(void);

/* draw_cell.c */
int draw_cell(View *, int);

/* draw_match.c */
int draw_cell(View *, int);

/* draw_reg.c */
int draw_region(void);
int line_in_map1(int, int, int, int, int);

/* driver.c */
int driver(void);

/* edge.c */
int edge(register int, register int, int, int);

/* edge_point.c */
int edge_point(register int, register int);

/* erase_reg.c */
int erase_region(void);

/* g_init.c */
int g_init(void);

/* graphics.c */
int Init_graphics(void);
int Outline_box(int, int, int, int);
int Text_width(char *);
int Text(char *, int, int, int, int, int);

/* histograms.c */
int histograms(int, float *, float **, int **, int, int *, int *, double,
	       int);
/* init_reg.c */
int init_region(struct region Region);

/* input.c */
int Input_pointer(Objects *);
int Input_box(Objects *, int, int);
int Input_other(int (*)(), char *);
int use_mouse_msg(void);
int Menu_msg(char *);
int Start_mouse_in_menu(void);

/* input_color.c */
int input_color(void);
int green(void);
int red(void);
int blue(void);
int yellow(void);
int orange(void);
int brown(void);
int purple(void);
int white(void);
int grey(void);
int black(void);

/* input_std.c */
int input_std(void);
int other(void);
int nstd050(void);
int nstd075(void);
int nstd100(void);
int nstd125(void);
int nstd150(void);
int nstd175(void);
int nstd200(void);
int nstd225(void);
int nstd250(void);

/* main.c */
#ifdef __GNUC_MINOR__
void quit(void) __attribute__ ((__noreturn__));
#else
void quit(void);
#endif
int error(const char *, int);

/* mouse.c */
int Mouse_pointer(int *, int *, int *);
int Mouse_line_anchored(int, int, int *, int *, int *);
int Mouse_box_anchored(int, int, int *, int *, int *);
int Get_mouse_xy(int *, int *);
int Set_mouse_xy(int, int);

/* outline.c */
int outline(void);

/* readbands.c */
int readbands(int, int);

/* redisplay.c */
int redisplay(void);
int redisplay_both(void);
int redisplay_map(void);
int redisplay_zoom(void);
int cancel_redisplay(void);

/* remove_mask.c */
int remove_mask(void);

/* restore_reg.c */
int restore_region(void);

/* save_reg.c */
int save_region(void);

/* set_signals.c */
int set_signals(void);

/* sigalarm.c */
void sigalarm(int);

/* signature.c */
int init_sig_routines(size_t);
int prepare_signature(int);
int show_signature(int, double);
int display_signature(void);
int have_signature(void);
int save_signature(void);
int write_signatures(void);

/* title.c */
int display_title(View *);

/* view.c */
int Configure_view(View *, char *, char *, double, double);
int In_view(View *, int, int);
int Erase_view(View *);
double magnification(View *);

/* zoom_box.c */
int zoom_box(void);

#endif /* __LOCAL_PROTO_H__ */
