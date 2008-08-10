#ifndef _GRASS_RASTER_H
#define _GRASS_RASTER_H

void R_flush(void);
int R_open_driver(void);
void R_close_driver(void);

double R_screen_left(void);
double R_screen_rite(void);
double R_screen_bot(void);
double R_screen_top(void);

int R_get_num_colors(void);

void R_standard_color(int);
void R_RGB_color(int, int, int);

void R_line_width(double);
void R_erase(void);

void R_move_abs(double, double);
void R_move_rel(double, double);
void R_cont_abs(double, double);
void R_cont_rel(double, double);
void R_polydots_abs(const double *, const double *, int);
void R_polydots_rel(const double *, const double *, int);
void R_polyline_abs(const double *, const double *, int);
void R_polyline_rel(const double *, const double *, int);
void R_polygon_abs(const double *, const double *, int);
void R_polygon_rel(const double *, const double *, int);
void R_box_abs(double, double, double, double);
void R_box_rel(double, double);

void R_text_size(double, double);
void R_text_rotation(double);
void R_set_window(double, double, double, double);
void R_text(const char *);
void R_get_text_box(const char *, double *, double *, double *, double *);

void R_font(const char *);
void R_encoding(const char *);
void R_font_list(char ***, int *);
void R_font_info(char ***, int *);

void R_begin_scaled_raster(int, int[2][2], double[2][2]);
int R_scaled_raster(int, int,
		    const unsigned char *,
		    const unsigned char *,
		    const unsigned char *, const unsigned char *);
void R_end_scaled_raster(void);
void R_bitmap(int, int, int, const unsigned char *);

#endif
