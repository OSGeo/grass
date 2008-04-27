#ifndef _GRASS_RASTER_H
#define _GRASS_RASTER_H

#include <grass/monitors.h>

/* common.c */
void R_flush(void);
void R_pad_perror(const char *,int);
void R_pad_freelist(char **,int);

/* get.c */
void R_get_location_with_box(int,int,int *,int *,int *);
void R_get_location_with_line(int,int,int *,int *,int *);
void R_get_location_with_pointer(int *,int *,int *);

/* io.c */
int _send_ident(int);
int _send_char(const unsigned char *);
int _send_char_array(int,const unsigned char *);
int _send_int_array(int,const int *);
int _send_float_array(int,const float *);
int _send_int(const int *);
int _send_float(const float *);
int _send_text(const char *);

int _get_char(char *);
int _get_int(int *);
int _get_float(float *);
int _get_text(char *);
char *_get_text_2(void);

void R__open_quiet(void);
int sync_driver(char *name);
void _hold_signals(int);
void R_stabilize(void);
void R_kill_driver(void);
void R_close_driver(void);
void R_release_driver(void);

/* io_fifo.c / io_sock.c */
int unlock_driver(int);
int R_open_driver(void);

/* pad.c */
int R_pad_create(const char *);
int R_pad_current(char *);
int R_pad_delete(void);
int R_pad_invent(char *);
int R_pad_list(char ***,int *);
int R_pad_select(const char *);
int R_pad_append_item(const char *,const char *,int);
int R_pad_delete_item(const char *);
int R_pad_get_item(const char *,char ***,int *);
int R_pad_list_items(char ***,int *);
int R_pad_set_item(const char *,const char *);

/* parse_mon.c */
struct MON_CAP *R_parse_monitorcap(int,char *);

/* protocol.c */
int R_screen_left(void);
int R_screen_rite(void);
int R_screen_bot(void);
int R_screen_top(void);
void R_get_num_colors(int *);

void R_standard_color(int);
void R_RGB_color(unsigned char,unsigned char,unsigned char);

void R_line_width(int);
void R_erase(void);

void R_move_abs(int,int);
void R_move_rel(int,int);
void R_cont_abs(int,int);
void R_cont_rel(int,int);
void R_polydots_abs(const int *,const int *,int);
void R_polydots_rel(const int *,const int *,int);
void R_polyline_abs(const int *,const int *,int);
void R_polyline_rel(const int *,const int *,int);
void R_polygon_abs(const int *,const int *,int);
void R_polygon_rel(const int *,const int *,int);
void R_box_abs(int,int,int,int);
void R_box_rel(int,int);

void R_text_size(int,int);
void R_text_rotation(float);
void R_set_window(int,int,int,int);
void R_text(const char *);
void R_get_text_box(const char *,int *,int *,int *,int *);

void R_font(const char *);
void R_charset(const char *);
void R_font_list(char ***, int *);
void R_font_info(char ***, int *);

void R_panel_save(const char *,int,int,int,int);
void R_panel_restore(const char *);
void R_panel_delete(const char *);

void R_begin_scaled_raster(int, int [2][2], int [2][2]);
int R_scaled_raster(int, int,
		    const unsigned char *,
		    const unsigned char *,
		    const unsigned char *,
		    const unsigned char *);
void R_end_scaled_raster(void);
void R_bitmap(int,int,int,const unsigned char *);

#endif
