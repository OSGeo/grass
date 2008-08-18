#ifndef GRASS_DISPLAY_H
#define GRASS_DISPLAY_H

#include <grass/gis.h>
#include <grass/symbol.h>

/* cnversions.c */
int D_do_conversions(const struct Cell_head *, double, double, double, double);
int D_is_lat_lon(void);
double D_get_u_to_d_xconv(void);
double D_get_u_to_d_yconv(void);
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
double D_u_to_a_row(double);
double D_u_to_a_col(double);
double D_a_to_d_row(double);
double D_a_to_d_col(double);
double D_u_to_d_row(double);
double D_u_to_d_col(double);
double D_d_to_u_row(double);
double D_d_to_u_col(double);
double D_d_to_a_row(double);
double D_d_to_a_col(double);
double D_get_ns_resolution(void);
double D_get_ew_resolution(void);
void D_get_u(double[2][2]);
void D_get_a(int[2][2]);
void D_get_d(double[2][2]);

/* color_list.c */
char *D_color_list(void);

/* draw2.c */
void D_set_clip(double, double, double, double);
void D_clip_to_map(void);
void D_clip_to_display(void);
void D_clip_to_screen(void);
void D_move_clip(double, double);
int D_cont_clip(double, double);
int D_line_clip(double, double, double, double);
void D_move_abs_clip(double, double);
int D_cont_abs_clip(double, double);
int D_line_abs_clip(double, double, double, double);
void D_polydots_clip(const double *, const double *, int);
void D_polyline_cull(const double *, const double *, int);
void D_polyline_clip(const double *, const double *, int);
void D_polygon_cull(const double *, const double *, int);
void D_polygon_clip(const double *, const double *, int);
void D_box_clip(double, double, double, double);
void D_move(double, double);
void D_cont(double, double);
void D_line(double, double, double, double);
void D_polydots(const double *, const double *, int);
void D_polyline(const double *, const double *, int);
void D_polygon(const double *, const double *, int);
void D_box(double, double, double, double);
void D_line_width(double);
void D_move_abs(double, double);
void D_cont_abs(double, double);
void D_line_abs(double, double, double, double);

/* icon.c */
void D_plot_icon(double, double, int, double, double);

/* raster.c */
int D_draw_raster(int, const void *, struct Colors *, RASTER_MAP_TYPE);
int D_draw_d_raster(int, const DCELL *, struct Colors *);
int D_draw_f_raster(int, const FCELL *, struct Colors *);
int D_draw_c_raster(int, const CELL *, struct Colors *);
int D_draw_cell(int, const CELL *, struct Colors *);
int D_cell_draw_setup(double, double, double, double);
int D_draw_raster_RGB(int, const void *, const void *, const void *,
		      struct Colors *, struct Colors *, struct Colors *,
		      RASTER_MAP_TYPE, RASTER_MAP_TYPE, RASTER_MAP_TYPE);
void D_cell_draw_end(void);

/* raster2.c */
int D_set_overlay_mode(int);
int D_color(CELL, struct Colors *);
int D_c_color(CELL, struct Colors *);
int D_d_color(DCELL, struct Colors *);
int D_f_color(FCELL, struct Colors *);
int D_color_of_type(const void *, struct Colors *, RASTER_MAP_TYPE);

/* setup.c */
void D_setup(int);

/* symbol.c */
void D_symbol(const SYMBOL *, double, double, const RGBA_Color *,
	      const RGBA_Color *);
void D_symbol2(const SYMBOL *, double, double, const RGBA_Color *,
	       const RGBA_Color *);

/* tran_colr.c */
int D_translate_color(const char *);
int D_translate_or_add_color(const char *, int);
int D_allocate_color(void);
int D_parse_color(const char *, int);
int D_raster_use_color(int);
int D_color_number_to_RGB(int, int *, int *, int *);

/* window.c */
void D_set_window(double, double, double, double);
void D_get_screen_window(double *, double *, double *, double *);
void D_check_map_window(struct Cell_head *);
void D_erase(const char *);

#endif /* GRASS_DISPLAY_H */
