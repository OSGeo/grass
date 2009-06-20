#ifndef _GRASS_DISPLAY_RASTER_H
#define _GRASS_DISPLAY_RASTER_H

void R__line_width(double);
void R__get_text_box(const char *, double *, double *, double *, double *);
void R__pos_abs(double, double);
void R__begin(void);
void R__move(double, double);
void R__cont(double, double);
void R__close(void);
void R__stroke(void);
void R__fill(void);
void R__point(double, double);
void R__RGB_color(int, int, int);
int R__scaled_raster(int, int,
		     const unsigned char *,
		     const unsigned char *,
		     const unsigned char *, const unsigned char *);
void R__begin_scaled_raster(int, int[2][2], double[2][2]);
void R__end_scaled_raster(void);
void R__standard_color(int);

int R_open_driver(void);
void R_close_driver(void);

void R_get_window(double *, double *, double *, double *);

void R_erase(void);

void R_text_size(double, double);
void R_text_rotation(double);
void R_text(const char *);

void R_font(const char *);
void R_encoding(const char *);
void R_font_list(char ***, int *);
void R_font_info(char ***, int *);

#endif
