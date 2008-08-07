#ifndef _GRASS_RASTER_H
#define _GRASS_RASTER_H

void R_flush(void);
int R_open_driver(void);
void R_close_driver(void);

int R_screen_left(void);
int R_screen_rite(void);
int R_screen_bot(void);
int R_screen_top(void);
int R_get_num_colors(void);

void R_standard_color(int);
void R_RGB_color(unsigned char, unsigned char, unsigned char);

void R_line_width(int);
void R_erase(void);

void R_move_abs(int, int);
void R_move_rel(int, int);
void R_cont_abs(int, int);
void R_cont_rel(int, int);
void R_polydots_abs(const int *, const int *, int);
void R_polydots_rel(const int *, const int *, int);
void R_polyline_abs(const int *, const int *, int);
void R_polyline_rel(const int *, const int *, int);
void R_polygon_abs(const int *, const int *, int);
void R_polygon_rel(const int *, const int *, int);
void R_box_abs(int, int, int, int);
void R_box_rel(int, int);

void R_text_size(int, int);
void R_text_rotation(float);
void R_set_window(int, int, int, int);
void R_text(const char *);
void R_get_text_box(const char *, int *, int *, int *, int *);

void R_font(const char *);
void R_charset(const char *);
void R_font_list(char ***, int *);
void R_font_info(char ***, int *);

void R_begin_scaled_raster(int, int[2][2], int[2][2]);
int R_scaled_raster(int, int,
		    const unsigned char *,
		    const unsigned char *,
		    const unsigned char *, const unsigned char *);
void R_end_scaled_raster(void);
void R_bitmap(int, int, int, const unsigned char *);

#endif
