#ifndef GRASS_DISPLAYDEFS_H
#define GRASS_DISPLAYDEFS_H

/* cnversions.c */
void D_update_conversions(void);
void D_fit_d_to_u(void);
void D_fit_u_to_d(void);
void D_show_conversions(void);

void D_do_conversions(const struct Cell_head *, double, double, double, double);

int D_is_lat_lon(void);

double D_get_d_to_a_xconv(void);
double D_get_d_to_a_yconv(void);
double D_get_d_to_u_xconv(void);
double D_get_d_to_u_yconv(void);
double D_get_a_to_u_xconv(void);
double D_get_a_to_u_yconv(void);
double D_get_a_to_d_xconv(void);
double D_get_a_to_d_yconv(void);
double D_get_u_to_d_xconv(void);
double D_get_u_to_d_yconv(void);
double D_get_u_to_a_xconv(void);
double D_get_u_to_a_yconv(void);

double D_get_ns_resolution(void);
double D_get_ew_resolution(void);

double D_get_u_west(void);
double D_get_u_east(void);
double D_get_u_north(void);
double D_get_u_south(void);
double D_get_a_west(void);
double D_get_a_east(void);
double D_get_a_north(void);
double D_get_a_south(void);
double D_get_d_west(void);
double D_get_d_east(void);
double D_get_d_north(void);
double D_get_d_south(void);

void D_set_region(const struct Cell_head *);
void D_set_src(double, double, double, double);
void D_get_src(double *, double *, double *, double *);
void D_set_grid(int, int, int, int);
void D_get_grid(int *, int *, int *, int *);
void D_set_dst(double, double, double, double);
void D_get_dst(double *, double *, double *, double *);

void D_get_u(double [2][2]);
void D_get_a(int [2][2]);
void D_get_d(double [2][2]);

double D_d_to_a_row(double);
double D_d_to_a_col(double);
double D_d_to_u_row(double);
double D_d_to_u_col(double);
double D_a_to_u_row(double);
double D_a_to_u_col(double);
double D_a_to_d_row(double);
double D_a_to_d_col(double);
double D_u_to_d_row(double);
double D_u_to_d_col(double);
double D_u_to_a_row(double);
double D_u_to_a_col(double);

/* draw2.c */

void D_set_clip(double, double, double, double);
void D_clip_to_map(void);
void D_set_clip_mode(int);
void D_set_reduction(double);

void D_line_width(double);
void D_get_text_box(const char *, double *, double *, double *, double *);

void D_pos_abs(double, double);
void D_pos_rel(double, double);
void D_move_abs(double, double);
void D_move_rel(double, double);
void D_cont_abs(double, double);
void D_cont_rel(double, double);
void D_line_abs(double, double, double, double);
void D_line_rel(double, double, double, double);
void D_polydots_abs(const double *, const double *, int);
void D_polydots_rel(const double *, const double *, int);
void D_polyline_abs(const double *, const double *, int);
void D_polyline_rel(const double *, const double *, int);
void D_polygon_abs(const double *, const double *, int);
void D_polygon_rel(const double *, const double *, int);
void D_box_abs(double, double, double, double);
void D_box_rel(double, double);

void D_begin(void);
void D_end(void);
void D_close(void);
void D_stroke(void);
void D_fill(void);
void D_dots(void);

/* icon.c */
void D_plot_icon(double, double, int, double, double);

/* raster.c */
int D_draw_raster(int, const void *, struct Colors *, RASTER_MAP_TYPE);
int D_draw_d_raster(int, const DCELL *, struct Colors *);
int D_draw_f_raster(int, const FCELL *, struct Colors *);
int D_draw_c_raster(int, const CELL *, struct Colors *);
void D_raster_draw_begin(void);
int D_draw_raster_RGB(int, const void *, const void *, const void *,
		      struct Colors *, struct Colors *, struct Colors *,
		      RASTER_MAP_TYPE, RASTER_MAP_TYPE, RASTER_MAP_TYPE);
void D_raster_draw_end(void);

/* raster2.c */
int D_set_overlay_mode(int);
int D_color(CELL, struct Colors *);
int D_c_color(CELL, struct Colors *);
int D_d_color(DCELL, struct Colors *);
int D_f_color(FCELL, struct Colors *);
int D_color_of_type(const void *, struct Colors *, RASTER_MAP_TYPE);

/* setup.c */
void D_setup(int);
void D_setup_unity(int);
void D_setup2(int, int, double, double, double, double);

/* symbol.c */
void D_symbol(const SYMBOL *, double, double, const RGBA_Color *,
	      const RGBA_Color *);
void D_symbol2(const SYMBOL *, double, double, const RGBA_Color *,
	       const RGBA_Color *);

/* tran_colr.c */
int D_translate_color(const char *);
int D_parse_color(const char *, int);
int D_use_color(int);
int D_color_number_to_RGB(int, int *, int *, int *);
void D_RGB_color(int, int, int);

/* window.c */
void D_erase(const char *);

/* r_raster.c */

int D_open_driver(void);
void D_close_driver(void);
int D_save_command(const char *);

void D__erase(void);

void D_text_size(double, double);
void D_text_rotation(double);
void D_text(const char *);

void D_font(const char *);
void D_encoding(const char *);
void D_font_list(char ***, int *);
void D_font_info(char ***, int *);

void D_get_clip_window(double *, double *, double *, double *);
void D_set_clip_window(double, double, double, double);
void D_get_frame(double *, double *, double *, double *);
void D_get_screen(double *, double *, double *, double *);
void D_set_clip_window_to_map_window(void);
void D_set_clip_window_to_screen_window(void);

const char *D_get_file(void);

#endif /* GRASS_DISPLAYDEFS_H */
