/*
 *****************************************************************************
 *
 * MODULE:      Grass Include Files
 * AUTHOR(S):   Original author unknown - probably CERL
 *              Justin Hickey - Thailand - jhickey@hpcc.nectec.or.th
 * PURPOSE:     This file contains the prototypes for all the functions in the
 *              gis library (src/libes/gis).
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

#ifndef GRASS_GISDEFS_H
#define GRASS_GISDEFS_H

/*============================= Include Files ==============================*/

/* none */

/*=========================== Constants/Defines ============================*/

/* none (look in gis.h) */

/*=========================== Typedefs/Structures ==========================*/

/* none (look in gis.h) */

/*============================== Prototypes ================================*/

#ifdef __GNUC__
# ifdef __MINGW32__
#  include <malloc.h>
# else
#  include <alloca.h>
# endif
# define G__alloca(n) alloca(n)
# define G__freea(p)
#else
# define G__alloca(n) G_malloc(n)
# define G__freea(p) G_free(p)
#endif

#include <sys/types.h>

/* adj_cellhd.c */
const char *G_adjust_Cell_head(struct Cell_head *, int, int);
const char *G_adjust_Cell_head3(struct Cell_head *, int, int, int);

/* align_window.c */
const char *G_align_window(struct Cell_head *, const struct Cell_head *);

/* alloc.c */
void *G__malloc(const char *, int, size_t);
void *G__calloc(const char *, int, size_t, size_t);
void *G__realloc(const char *, int, void *, size_t);
void G_free(void *);

#define G_malloc(n)     G__malloc(__FILE__, __LINE__, (n))
#define G_calloc(m, n)  G__calloc(__FILE__, __LINE__, (m), (n))
#define G_realloc(p, n) G__realloc(__FILE__, __LINE__, (p), (n))

/* alloc_cell.c */
size_t G_raster_size(RASTER_MAP_TYPE);
CELL *G_allocate_cell_buf(void);
void *G_allocate_raster_buf(RASTER_MAP_TYPE);
CELL *G_allocate_c_raster_buf(void);
FCELL *G_allocate_f_raster_buf(void);
DCELL *G_allocate_d_raster_buf(void);
char *G_allocate_null_buf(void);
unsigned char *G__allocate_null_bits(int);
int G__null_bitstream_size(int);

/* area.c */
int G_begin_cell_area_calculations(void);
double G_area_of_cell_at_row(int);
int G_begin_polygon_area_calculations(void);
double G_area_of_polygon(const double *, const double *, int);

/* area_ellipse.c */
void G_begin_zone_area_on_ellipsoid(double, double, double);
double G_darea0_on_ellipsoid(double);
double G_area_for_zone_on_ellipsoid(double, double);

/* area_poly1.c */
void G_begin_ellipsoid_polygon_area(double, double);
double G_ellipsoid_polygon_area(const double *, const double *, int);

/* area_poly2.c */
double G_planimetric_polygon_area(const double *, const double *, int);

/* area_sphere.c */
void G_begin_zone_area_on_sphere(double, double);
double G_darea0_on_sphere(double);
double G_area_for_zone_on_sphere(double, double);

/* ascii_chk.c */
void G_ascii_check(char *);


/* asprintf.c */
/* Do it better if you know how */
/* asprintf is not found on MINGW but exists */

/* 
 *  Because configure script in GDAL test is G_asprintf exists in gis lib
 *  the G_asprintf macro is disabled until a stable version of GDAL
 *  with a different function becomes widely used 
 */
#ifndef SWIG
int G_vasprintf(char **, const char *, va_list);
#endif
int G_asprintf(char **, const char *, ...)
    __attribute__ ((format(printf, 2, 3)));

/* auto_mask.c */
int G__check_for_auto_masking(void);
void G_suppress_masking(void);
void G_unsuppress_masking(void);

/* basename.c */
char *G_basename(char *, const char *);

/* bres_line.c */
void G_bresenham_line(int, int, int, int, int (*)(int, int));

/* cats.c */
int G_read_cats(const char *, const char *, struct Categories *);
int G_read_raster_cats(const char *, const char *, struct Categories *);
int G_read_vector_cats(const char *, const char *, struct Categories *);
CELL G_number_of_cats(const char *, const char *);
char *G_get_cats_title(const struct Categories *);
char *G_get_raster_cats_title(const struct Categories *);
char *G_get_cat(CELL, struct Categories *);
char *G_get_c_raster_cat(CELL *, struct Categories *);
char *G_get_f_raster_cat(FCELL *, struct Categories *);
char *G_get_d_raster_cat(DCELL *, struct Categories *);
char *G_get_raster_cat(void *, struct Categories *, RASTER_MAP_TYPE);
void G_unmark_raster_cats(struct Categories *);
void G_mark_c_raster_cats(const CELL *, int, struct Categories *);
void G_mark_f_raster_cats(const FCELL *, int, struct Categories *);
void G_mark_d_raster_cats(const DCELL *, int, struct Categories *);
int G_mark_raster_cats(const void *, int, struct Categories *, RASTER_MAP_TYPE);
void G_rewind_raster_cats(struct Categories *);
char *G_get_next_marked_d_raster_cat(struct Categories *, DCELL *, DCELL *,
				     long *);
char *G_get_next_marked_c_raster_cat(struct Categories *, CELL *, CELL *,
				     long *);
char *G_get_next_marked_f_raster_cat(struct Categories *, FCELL *, FCELL *,
				     long *);
char *G_get_next_marked_raster_cat(struct Categories *, void *, void *,
				   long *, RASTER_MAP_TYPE);
int G_set_cat(CELL, const char *, struct Categories *);
int G_set_c_raster_cat(const CELL *, const CELL *, const char *, struct Categories *);
int G_set_f_raster_cat(const FCELL *, const FCELL *, const char *, struct Categories *);
int G_set_d_raster_cat(const DCELL *, const DCELL *, const char *, struct Categories *);
int G_set_raster_cat(const void *, const void *, const char *, struct Categories *,
		     RASTER_MAP_TYPE);
int G_write_cats(const char *, struct Categories *);
int G_write_raster_cats(const char *, struct Categories *);
int G_write_vector_cats(const char *, struct Categories *);
char *G_get_ith_d_raster_cat(const struct Categories *, int, DCELL *,
			     DCELL *);
char *G_get_ith_f_raster_cat(const struct Categories *, int, void *, void *);
char *G_get_ith_c_raster_cat(const struct Categories *, int, void *, void *);
char *G_get_ith_raster_cat(const struct Categories *, int, void *, void *,
			   RASTER_MAP_TYPE);
void G_init_cats(CELL, const char *, struct Categories *);
void G_init_raster_cats(const char *, struct Categories *);
void G_set_cats_title(const char *, struct Categories *);
void G_set_raster_cats_title(const char *, struct Categories *);
void G_set_cats_fmt(const char *, double, double, double, double,
		   struct Categories *);
void G_set_raster_cats_fmt(const char *, double, double, double, double,
			   struct Categories *);
void G_free_cats(struct Categories *);
void G_free_raster_cats(struct Categories *);
void G_copy_raster_cats(struct Categories *, const struct Categories *);
int G_number_of_raster_cats(struct Categories *);
int G_sort_cats(struct Categories *);

/* cell_stats.c */
void G_init_cell_stats(struct Cell_stats *);
int G_update_cell_stats(const CELL *, int, struct Cell_stats *);
int G_find_cell_stat(CELL, long *, const struct Cell_stats *);
int G_rewind_cell_stats(struct Cell_stats *);
int G_next_cell_stat(CELL *, long *, struct Cell_stats *);
void G_get_stats_for_null_value(long *, const struct Cell_stats *);
void G_free_cell_stats(struct Cell_stats *);

/* cell_title.c */
char *G_get_cell_title(const char *, const char *);

/* cellstats_eq.c */
int G_cell_stats_histo_eq(struct Cell_stats *, CELL, CELL, CELL, CELL, int,
			  void (*)(CELL, CELL, CELL));

/* clear_scrn.c */
void G_clear_screen(void);

/* clicker.c */
void G_clicker(void);

/* closecell.c */
int G_close_cell(int);
int G_unopen_cell(int);

/* color_compat.c */
void G_make_ryg_colors(struct Colors *, CELL, CELL);
void G_make_ryg_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_aspect_colors(struct Colors *, CELL, CELL);
void G_make_aspect_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_byr_colors(struct Colors *, CELL, CELL);
void G_make_byr_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_bgyr_colors(struct Colors *, CELL, CELL);
void G_make_bgyr_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_byg_colors(struct Colors *, CELL, CELL);
void G_make_byg_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_grey_scale_colors(struct Colors *, CELL, CELL);
void G_make_grey_scale_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_gyr_colors(struct Colors *, CELL, CELL);
void G_make_gyr_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_rainbow_colors(struct Colors *, CELL, CELL);
void G_make_rainbow_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_ramp_colors(struct Colors *, CELL, CELL);
void G_make_ramp_fp_colors(struct Colors *, DCELL, DCELL);
void G_make_wave_colors(struct Colors *, CELL, CELL);
void G_make_wave_fp_colors(struct Colors *, DCELL, DCELL);

/* color_free.c */
void G_free_colors(struct Colors *);
void G__color_free_rules(struct _Color_Info_ *);
void G__color_free_lookup(struct _Color_Info_ *);
void G__color_free_fp_lookup(struct _Color_Info_ *);
void G__color_reset(struct Colors *);

/* color_get.c */
int G_get_color(CELL, int *, int *, int *, struct Colors *);
int G_get_raster_color(const void *, int *, int *, int *, struct Colors *,
		       RASTER_MAP_TYPE);
int G_get_c_raster_color(const CELL *, int *, int *, int *, struct Colors *);
int G_get_f_raster_color(const FCELL *, int *, int *, int *, struct Colors *);
int G_get_d_raster_color(const DCELL *, int *, int *, int *, struct Colors *);
void G_get_null_value_color(int *, int *, int *, const struct Colors *);
void G_get_default_color(int *, int *, int *, const struct Colors *);

/* color_hist.c */
void G_make_histogram_eq_colors(struct Colors *, struct Cell_stats *);
void G_make_histogram_log_colors(struct Colors *, struct Cell_stats *, int, int);

/* color_init.c */
void G_init_colors(struct Colors *);

/* color_insrt.c */
int G__insert_color_into_lookup(CELL, int, int, int, struct _Color_Info_ *);

/* color_invrt.c */
void G_invert_colors(struct Colors *);

/* color_look.c */
void G_lookup_colors(const CELL *, unsigned char *, unsigned char *,
		     unsigned char *, unsigned char *, int, struct Colors *);
void G_lookup_c_raster_colors(const CELL *, unsigned char *, unsigned char *,
			      unsigned char *, unsigned char *, int,
			      struct Colors *);
void G_lookup_raster_colors(const void *, unsigned char *, unsigned char *,
			    unsigned char *, unsigned char *, int,
			    struct Colors *, RASTER_MAP_TYPE);
void G_lookup_f_raster_colors(const FCELL *, unsigned char *, unsigned char *,
			     unsigned char *, unsigned char *, int,
			     struct Colors *);
void G_lookup_d_raster_colors(const DCELL *, unsigned char *, unsigned char *,
			     unsigned char *, unsigned char *, int,
			     struct Colors *);
void G__lookup_colors(const void *, unsigned char *, unsigned char *,
		      unsigned char *, unsigned char *, int, struct Colors *,
		      int, int, RASTER_MAP_TYPE);
void G__interpolate_color_rule(DCELL, unsigned char *, unsigned char *,
			       unsigned char *, const struct _Color_Rule_ *);

/* color_org.c */
void G__organize_colors(struct Colors *);

/* color_rand.c */
void G_make_random_colors(struct Colors *, CELL, CELL);

/* color_range.c */
void G_set_color_range(CELL, CELL, struct Colors *);
void G_set_d_color_range(DCELL, DCELL, struct Colors *);
void G_get_color_range(CELL *, CELL *, const struct Colors *);
void G_get_d_color_range(DCELL *, DCELL *, const struct Colors *);

/* color_read.c */
int G_read_colors(const char *, const char *, struct Colors *);
void G_mark_colors_as_fp(struct Colors *);

/* color_remove.c */
int G_remove_colors(const char *, const char *);

/* color_rule.c */
void G_add_d_raster_color_rule(const DCELL *, int, int, int,
			       const DCELL *, int, int, int,
			       struct Colors *);
void G_add_f_raster_color_rule(const FCELL *, int, int, int,
			       const FCELL *, int, int, int,
			       struct Colors *);
void G_add_c_raster_color_rule(const CELL *, int, int, int,
			       const CELL *, int, int, int,
			       struct Colors *);
void G_add_raster_color_rule(const void *, int, int, int,
			     const void *, int, int, int,
			     struct Colors *, RASTER_MAP_TYPE);
void G_add_color_rule(const CELL, int, int, int,
		      const CELL, int, int, int,
		      struct Colors *);
int G_add_modular_d_raster_color_rule(const DCELL *, int, int, int,
				      const DCELL *, int, int, int,
				      struct Colors *);
int G_add_modular_f_raster_color_rule(const FCELL *, int, int, int,
				      const FCELL *, int, int, int,
				      struct Colors *);
int G_add_modular_c_raster_color_rule(const CELL *, int, int, int,
				      const CELL *, int, int, int,
				      struct Colors *);
int G_add_modular_raster_color_rule(const void *, int, int, int,
				    const void *, int, int, int,
				    struct Colors *, RASTER_MAP_TYPE);
int G_add_modular_color_rule(CELL, int, int, int,
			     CELL, int, int, int,
			     struct Colors *);

/* color_rule_get.c */
int G_colors_count(const struct Colors *);
int G_get_f_color_rule(DCELL *, unsigned char *, unsigned char *,
		       unsigned char *, DCELL *, unsigned char *,
		       unsigned char *, unsigned char *,
		       const struct Colors *, int);

/* color_rules.c */
typedef int read_rule_fn(void *, DCELL, DCELL,
			 DCELL *, int *, int *, int *, int *, int *, int *);
int G_parse_color_rule(DCELL, DCELL, const char *, DCELL *, int *, int *,
		       int *, int *, int *, int *);
const char *G_parse_color_rule_error(int);
int G_read_color_rule(void *, DCELL, DCELL, DCELL *, int *, int *, int *,
		      int *, int *, int *);
int G_read_color_rules(struct Colors *, DCELL, DCELL, read_rule_fn *, void *);
int G_load_colors(struct Colors *, const char *, CELL, CELL);
int G_load_fp_colors(struct Colors *, const char *, DCELL, DCELL);
void G_make_colors(struct Colors *, const char *, CELL, CELL);
void G_make_fp_colors(struct Colors *, const char *, DCELL, DCELL);

/* color_set.c */
void G_set_color(CELL, int, int, int, struct Colors *);
void G_set_d_color(DCELL, int, int, int, struct Colors *);
void G_set_null_value_color(int, int, int, struct Colors *);
void G_set_default_color(int, int, int, struct Colors *);

/* color_shift.c */
void G_shift_colors(int, struct Colors *);
void G_shift_d_colors(DCELL, struct Colors *);

/* color_str.c */
int G_str_to_color(const char *, int *, int *, int *);

/* color_write.c */
int G_write_colors(const char *, const char *, struct Colors *);
void G__write_colors(FILE *, struct Colors *);

/* color_xform.c */
void G_histogram_eq_colors(struct Colors *, struct Colors *,
			   struct Cell_stats *);
void G_histogram_eq_colors_fp(struct Colors *,
			      struct Colors *, struct FP_stats *);
void G_log_colors(struct Colors *, struct Colors *, int);
void G_abs_log_colors(struct Colors *, struct Colors *, int);

/* commas.c */
int G_insert_commas(char *);
void G_remove_commas(char *);

/* copy.c */
void G_copy(void *, const void *, int);

/* copy_dir.c */
int G_recursive_copy(const char *, const char *);

/* copy_file.c */
int G_copy_file(const char *, const char *);

/* counter.c */
int G_is_initialized(int *);
void G_initialize_done(int *);
void G_init_counter(struct Counter *, int);
int G_counter_next(struct Counter *);

/* date.c */
const char *G_date(void);

/* datum.c */
int G_get_datum_by_name(const char *);
const char *G_datum_name(int);
const char *G_datum_description(int);
const char *G_datum_ellipsoid(int);
int G_get_datumparams_from_projinfo(const struct Key_Value *, char *, char *);
void G_read_datum_table(void);


/* debug.c */
void G_init_debug(void);
int G_debug(int, const char *, ...) __attribute__ ((format(printf, 2, 3)));

/* distance.c */
int G_begin_distance_calculations(void);
double G_distance(double, double, double, double);
double G_distance_between_line_segments(double, double, double, double,
					double, double, double, double);
double G_distance_point_to_line_segment(double, double, double, double,
					double, double);

/* done_msg.c */
void G_done_msg(const char *, ...) __attribute__ ((format(printf, 1, 2)));

/* endian.c */
int G_is_little_endian(void);

/* env.c */
void G_init_env(void);
const char *G_getenv(const char *);
const char *G_getenv2(const char *, int);
const char *G__getenv(const char *);
const char *G__getenv2(const char *, int);
void G_setenv(const char *, const char *);
void G_setenv2(const char *, const char *, int);
void G__setenv(const char *, const char *);
void G__setenv2(const char *, const char *, int);
void G_unsetenv(const char *);
void G_unsetenv2(const char *, int);
void G__write_env(void);
const char *G__env_name(int);
void G__read_env(void);
void G_set_gisrc_mode(int);
int G_get_gisrc_mode(void);
void G__create_alt_env(void);
void G__switch_env(void);

/* error.c */
int G_info_format(void);
void G_message(const char *, ...) __attribute__ ((format(printf, 1, 2)));
void G_verbose_message(const char *, ...)
    __attribute__ ((format(printf, 1, 2)));
void G_important_message(const char *, ...)
    __attribute__ ((format(printf, 1, 2)));
void G_fatal_error(const char *, ...) __attribute__ ((format(printf, 1, 2)))
    __attribute__ ((noreturn));
void G_warning(const char *, ...) __attribute__ ((format(printf, 1, 2)));
int G_suppress_warnings(int);
int G_sleep_on_error(int);
void G_set_error_routine(int (*)(const char *, int));
void G_unset_error_routine(void);
void G_init_logging(void);

/* file_name.c */
char *G__file_name(char *, const char *, const char *, const char *);
char *G__file_name_misc(char *, const char *, const char *, const char *,
			const char *);

/* find_cell.c */
const char *G_find_cell(char *, const char *);
const char *G_find_cell2(const char *, const char *);

/* find_file.c */
const char *G_find_file(const char *, char *, const char *);
const char *G_find_file2(const char *, const char *, const char *);
const char *G_find_file_misc(const char *, const char *, char *, const char *);
const char *G_find_file2_misc(const char *, const char *, const char *,
			      const char *);

/* find_etc.c */
char *G_find_etc(const char *);

/* find_vect.c */
const char *G_find_vector(char *, const char *);
const char *G_find_vector2(const char *, const char *);

/* flate.c */
int G_zlib_compress(const unsigned char *, int, unsigned char *, int);
int G_zlib_expand(const unsigned char *, int, unsigned char *, int);
int G_zlib_write(int, const unsigned char *, int);
int G_zlib_read(int, int, unsigned char *, int);
int G_zlib_write_noCompress(int, const unsigned char *, int);

/* format.c */
int G__check_format(int);
int G__read_row_ptrs(int);
int G__write_row_ptrs(int);

/* fpreclass.c */
void G_fpreclass_clear(struct FPReclass *);
void G_fpreclass_reset(struct FPReclass *);
void G_fpreclass_init(struct FPReclass *);
void G_fpreclass_set_domain(struct FPReclass *, DCELL, DCELL);
void G_fpreclass_set_range(struct FPReclass *, DCELL, DCELL);
int G_fpreclass_get_limits(const struct FPReclass *, DCELL *, DCELL *,
			   DCELL *, DCELL *);
int G_fpreclass_nof_rules(const struct FPReclass *);
void G_fpreclass_get_ith_rule(const struct FPReclass *, int, DCELL *, DCELL *,
			      DCELL *, DCELL *);
void G_fpreclass_set_neg_infinite_rule(struct FPReclass *, DCELL, DCELL);
int G_fpreclass_get_neg_infinite_rule(const struct FPReclass *, DCELL *,
				      DCELL *);
void G_fpreclass_set_pos_infinite_rule(struct FPReclass *, DCELL, DCELL);
int G_fpreclass_get_pos_infinite_rule(const struct FPReclass *, DCELL *,
				      DCELL *);
void G_fpreclass_add_rule(struct FPReclass *, DCELL, DCELL, DCELL, DCELL);
void G_fpreclass_reverse_rule_order(struct FPReclass *);
DCELL G_fpreclass_get_cell_value(const struct FPReclass *, DCELL);
void G_fpreclass_perform_di(const struct FPReclass *, const DCELL *, CELL *,
			    int);
void G_fpreclass_perform_df(const struct FPReclass *, const DCELL *, FCELL *,
			    int);
void G_fpreclass_perform_dd(const struct FPReclass *, const DCELL *, DCELL *,
			    int);
void G_fpreclass_perform_fi(const struct FPReclass *, const FCELL *, CELL *,
			    int);
void G_fpreclass_perform_ff(const struct FPReclass *, const FCELL *, FCELL *,
			    int);
void G_fpreclass_perform_fd(const struct FPReclass *, const FCELL *, DCELL *,
			    int);
void G_fpreclass_perform_ii(const struct FPReclass *, const CELL *, CELL *,
			    int);
void G_fpreclass_perform_if(const struct FPReclass *, const CELL *, FCELL *,
			    int);
void G_fpreclass_perform_id(const struct FPReclass *, const CELL *, DCELL *,
			    int);
/* gdal.c */
void G_init_gdal(void);
struct GDAL_link *G_get_gdal_link(const char *, const char *);
struct GDAL_link *G_create_gdal_link(const char *, RASTER_MAP_TYPE);
void G_close_gdal_link(struct GDAL_link *);
int G_close_gdal_write_link(struct GDAL_link *);

/* geodesic.c */
int G_begin_geodesic_equation(double, double, double, double);
double G_geodesic_lat_from_lon(double);

/* geodist.c */
void G_begin_geodesic_distance(double, double);
void G_set_geodesic_distance_lat1(double);
void G_set_geodesic_distance_lat2(double);
double G_geodesic_distance_lon_to_lon(double, double);
double G_geodesic_distance(double, double, double, double);

/* get_cellhd.c */
int G_get_cellhd(const char *, const char *, struct Cell_head *);

/* get_ellipse.c */
int G_get_ellipsoid_parameters(double *, double *);
int G_get_spheroid_by_name(const char *, double *, double *, double *);
int G_get_ellipsoid_by_name(const char *, double *, double *);
const char *G_ellipsoid_name(int);
const char *G_ellipsoid_description(int);
int G_read_ellipsoid_table(int);

/* get_projinfo.c */
struct Key_Value *G_get_projunits(void);
struct Key_Value *G_get_projinfo(void);

/* get_row.c */
int G_get_map_row_nomask(int, CELL *, int);
int G_get_raster_row_nomask(int, void *, int, RASTER_MAP_TYPE);
int G_get_c_raster_row_nomask(int, CELL *, int);
int G_get_f_raster_row_nomask(int, FCELL *, int);
int G_get_d_raster_row_nomask(int, DCELL *, int);
int G_get_map_row(int, CELL *, int);
int G_get_raster_row(int, void *, int, RASTER_MAP_TYPE);
int G_get_c_raster_row(int, CELL *, int);
int G_get_f_raster_row(int, FCELL *, int);
int G_get_d_raster_row(int, DCELL *, int);
int G_get_null_value_row(int, char *, int);

/* get_row_colr.c */
int G_get_raster_row_colors(int, int, struct Colors *,
			    unsigned char *, unsigned char *, unsigned char *,
			    unsigned char *);

/* get_window.c */
void G_get_window(struct Cell_head *);
void G_get_default_window(struct Cell_head *);
char *G__get_window(struct Cell_head *, const char *, const char *,
		    const char *);

/* getl.c */
int G_getl(char *, int, FILE *);
int G_getl2(char *, int, FILE *);

/* gisbase.c */
const char *G_gisbase(void);

/* gisdbase.c */
const char *G_gisdbase(void);

/* gisinit.c */
void G__gisinit(const char *, const char *);
void G__no_gisinit(const char *);
void G__check_gisinit(void);
void G_init_all(void);

/* histo_eq.c */
void G_histogram_eq(const struct Histogram *, unsigned char **,
		    CELL *, CELL *);

/* histogram.c */
void G_init_histogram(struct Histogram *);
int G_read_histogram(const char *, const char *, struct Histogram *);
int G_write_histogram(const char *, const struct Histogram *);
int G_write_histogram_cs(const char *, struct Cell_stats *);
void G_make_histogram_cs(struct Cell_stats *, struct Histogram *);
int G_get_histogram_num(const struct Histogram *);
CELL G_get_histogram_cat(int, const struct Histogram *);
long G_get_histogram_count(int, const struct Histogram *);
void G_free_histogram(struct Histogram *);
int G_sort_histogram(struct Histogram *);
int G_sort_histogram_by_count(struct Histogram *);
void G_remove_histogram(const char *);
int G_add_histogram(CELL, long, struct Histogram *);
int G_set_histogram(CELL, long, struct Histogram *);
void G_extend_histogram(CELL, long, struct Histogram *);
void G_zero_histogram(struct Histogram *);

/* history.c */
int G_read_history(const char *, const char *, struct History *);
int G_write_history(const char *, struct History *);
void G_short_history(const char *, const char *, struct History *);
int G_command_history(struct History *);

/* home.c */
const char *G_home(void);
const char *G__home(void);

/* index.c */
char *G_index(const char *, int);
char *G_rindex(const char *, int);

/* interp.c */
DCELL G_interp_linear(double, DCELL, DCELL);
DCELL G_interp_bilinear(double, double, DCELL, DCELL, DCELL, DCELL);
DCELL G_interp_cubic(double, DCELL, DCELL, DCELL, DCELL);
DCELL G_interp_bicubic(double, double,
		       DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL,
		       DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL,
		       DCELL);

/* intersect.c */
int G_intersect_line_segments(double, double, double, double, double, double,
			      double, double, double *, double *, double *,
			      double *);

/* is.c */
int G_is_gisbase(const char *);
int G_is_location(const char *);
int G_is_mapset(const char *);

/* key_value1.c */
struct Key_Value *G_create_key_value(void);
int G_set_key_value(const char *, const char *, struct Key_Value *);
const char *G_find_key_value(const char *, const struct Key_Value *);
void G_free_key_value(struct Key_Value *);

/* key_value2.c */
int G_fwrite_key_value(FILE *, const struct Key_Value *);
struct Key_Value *G_fread_key_value(FILE *);

/* key_value3.c */
int G_write_key_value_file(const char *, const struct Key_Value *, int *);
struct Key_Value *G_read_key_value_file(const char *, int *);

/* key_value4.c */
int G_update_key_value_file(const char *, const char *, const char *);
int G_lookup_key_value_from_file(const char *, const char *, char[], int);

/* legal_name.c */
int G_legal_filename(const char *);
int G_check_input_output_name(const char *, const char *, int);

/* line_dist.c */
void G_set_distance_to_line_tolerance(double);
double G_distance2_point_to_line(double, double, double, double, double,
				 double);

/* list.c */
void G_list_element(const char *, const char *, const char *,
		    int (*)(const char *, const char *, const char *));
char **G_list(int, const char *, const char *, const char *);
void G_free_list(char **);

/* ll_format.c */
void G_lat_format(double, char *);
const char *G_lat_format_string(void);
void G_lon_format(double, char *);
const char *G_lon_format_string(void);
void G_llres_format(double, char *);
const char *G_llres_format_string(void);
void G_lat_parts(double, int *, int *, double *, char *);
void G_lon_parts(double, int *, int *, double *, char *);

/* ll_scan.c */
int G_lat_scan(const char *, double *);
int G_lon_scan(const char *, double *);
int G_llres_scan(const char *, double *);

/* location.c */
const char *G_location(void);
char *G_location_path(void);
char *G__location_path(void);

/* ls.c */
void G_set_ls_filter(int (*)(const char *, void *), void *);
void G_set_ls_exclude_filter(int (*)(const char *, void *), void *);
char **G__ls(const char *, int *);
void G_ls(const char *, FILE *);
void G_ls_format(char **, int, int, FILE *);

/* mach_name.c */
const char *G__machine_name(void);

/* make_loc.c */
int G__make_location(const char *, struct Cell_head *, struct Key_Value *,
		     struct Key_Value *, FILE *);
int G_make_location(const char *, struct Cell_head *, struct Key_Value *,
		    struct Key_Value *, FILE *);
int G_compare_projections(const struct Key_Value *, const struct Key_Value *,
			  const struct Key_Value *, const struct Key_Value *);

/* make_mapset.c */
int G__make_mapset(const char *gisdbase_name, const char *location_name,
		   const char *mapset_name);
int G_make_mapset(const char *gisdbase_name, const char *location_name,
		  const char *mapset_name);

/* mapcase.c */
char *G_tolcase(char *);
char *G_toucase(char *);

/* mapset.c */
const char *G_mapset(void);
const char *G__mapset(void);

/* mapset_msc.c */
int G__make_mapset_element(const char *);
int G__make_mapset_element_misc(const char *, const char *);
int G__mapset_permissions(const char *);
int G__mapset_permissions2(const char *, const char *, const char *);

/* mapset_nme.c */
const char *G__mapset_name(int);
void G_get_list_of_mapsets(void);
void G__create_alt_search_path(void);
void G__switch_search_path(void);
void G_reset_mapsets(void);
char **G_available_mapsets(void);
void G_add_mapset_to_search_path(const char *);
int G_is_mapset_in_search_path(const char *);

/* mask_info.c */
char *G_mask_info(void);
int G__mask_info(char *, char *);

/* maskfd.c */
int G_maskfd(void);

/* myname.c */
char *G_myname(void);

/* named_colr.c */
int G_color_values(const char *, float *, float *, float *);
const char *G_color_name(int);

/* nl_to_spaces.c */
void G_newlines_to_spaces(char *);

/* nme_in_mps.c */
int G__name_is_fully_qualified(const char *, char *, char *);
char *G_fully_qualified_name(const char *, const char *);
int G__unqualified_name(const char *, const char *, char *, char *);

/* null_val.c */
void G__set_null_value(void *, int, int, RASTER_MAP_TYPE);
void G_set_null_value(void *, int, RASTER_MAP_TYPE);
void G_set_c_null_value(CELL *, int);
void G_set_f_null_value(FCELL *, int);
void G_set_d_null_value(DCELL *, int);
int G_is_null_value(const void *, RASTER_MAP_TYPE);
int G_is_c_null_value(const CELL *);
int G_is_f_null_value(const FCELL *);
int G_is_d_null_value(const DCELL *);
void G_insert_null_values(void *, char *, int, RASTER_MAP_TYPE);
void G_insert_c_null_values(CELL *, char *, int);
void G_insert_f_null_values(FCELL *, char *, int);
void G_insert_d_null_values(DCELL *, char *, int);
int G__check_null_bit(const unsigned char *, int, int);
void G__convert_01_flags(const char *, unsigned char *, int);
void G__convert_flags_01(char *, const unsigned char *, int);
void G__init_null_bits(unsigned char *, int);

/* open.c */
int G_open_new(const char *, const char *);
int G_open_old(const char *, const char *, const char *);
int G_open_update(const char *, const char *);
FILE *G_fopen_new(const char *, const char *);
FILE *G_fopen_old(const char *, const char *, const char *);
FILE *G_fopen_append(const char *, const char *);
FILE *G_fopen_modify(const char *, const char *);

/* open_misc.c */
int G_open_new_misc(const char *, const char *, const char *);
int G_open_old_misc(const char *, const char *, const char *, const char *);
int G_open_update_misc(const char *, const char *, const char *);
FILE *G_fopen_new_misc(const char *, const char *, const char *);
FILE *G_fopen_old_misc(const char *, const char *, const char *,
		       const char *);
FILE *G_fopen_append_misc(const char *, const char *, const char *);
FILE *G_fopen_modify_misc(const char *, const char *, const char *);

/* opencell.c */
int G_open_cell_old(const char *, const char *);
int G__open_cell_old(const char *, const char *);
int G_open_cell_new(const char *);
int G_open_cell_new_uncompressed(const char *);
void G_want_histogram(int);
void G_set_cell_format(int);
int G_cellvalue_format(CELL);
int G_open_fp_cell_new(const char *);
int G_open_fp_cell_new_uncompressed(const char *);
int G_set_fp_type(RASTER_MAP_TYPE);
int G_raster_map_is_fp(const char *, const char *);
RASTER_MAP_TYPE G_raster_map_type(const char *, const char *);
RASTER_MAP_TYPE G__check_fp_type(const char *, const char *);
RASTER_MAP_TYPE G_get_raster_map_type(int);
int G_open_raster_new(const char *, RASTER_MAP_TYPE);
int G_open_raster_new_uncompressed(const char *, RASTER_MAP_TYPE);
int G_set_quant_rules(int, struct Quant *);

/* overwrite.c */
int G_check_overwrite(int argc, char **argv);

/* parser.c */
void G_disable_interactive(void);
struct GModule *G_define_module(void);
struct Flag *G_define_flag(void);
struct Option *G_define_option(void);
struct Option *G_define_standard_option(int);
int G_parser(int, char **);
void G_usage(void);
char *G_recreate_command(void);

/* paths.c */
int G_mkdir(const char *);
int G_is_dirsep(char);
int G_is_absolute_path(const char *);
char *G_convert_dirseps_to_host(char *);
char *G_convert_dirseps_from_host(char *);
struct stat;
int G_lstat(const char *, struct stat *);
int G_stat(const char *, struct stat *);

/* percent.c */
void G_percent(long, long, int);
void G_percent_reset(void);
void G_set_percent_routine(int (*) (int));
void G_unset_percent_routine(void);

/* plot.c */
void G_setup_plot(double, double, double, double, int (*)(int, int),
		  int (*)(int, int));
void G_setup_fill(int);
void G_plot_where_xy(double, double, int *, int *);
void G_plot_where_en(int, int, double *, double *);
void G_plot_point(double, double);
void G_plot_line(double, double, double, double);
void G_plot_line2(double, double, double, double);
int G_plot_polygon(const double *, const double *, int);
int G_plot_area(double *const *, double *const *, int *, int);
void G_plot_fx(double (*)(double), double, double);

/* pole_in_poly.c */
int G_pole_in_polygon(const double *, const double *, int);

/* popen.c */
FILE *G_popen(const char *, const char *);
int G_pclose(FILE *);

/* progrm_nme.c */
const char *G_program_name(void);
void G_set_program_name(const char *);

/* proj1.c */
int G_projection(void);

/* proj2.c */
int G__projection_units(int);
const char *G__unit_name(int, int);
const char *G__projection_name(int);

/* proj3.c */
const char *G_database_unit_name(int);
const char *G_database_projection_name(void);
const char *G_database_datum_name(void);
const char *G_database_ellipse_name(void);
double G_database_units_to_meters_factor(void);

/* put_cellhd.c */
int G_put_cellhd(const char *, struct Cell_head *);

/* put_row.c */
int G_put_map_row(int, const CELL *);
int G_put_raster_row(int, const void *, RASTER_MAP_TYPE);
int G_put_c_raster_row(int, const CELL *);
int G_put_f_raster_row(int, const FCELL *);
int G_put_d_raster_row(int, const DCELL *);
int G__open_null_write(int);
int G__write_null_bits(int, const unsigned char *, int, int, int);

/* put_title.c */
int G_put_cell_title(const char *, const char *);

/* put_window.c */
int G_put_window(const struct Cell_head *);
int G__put_window(const struct Cell_head *, const char *, const char *);

/* putenv.c */
void G_putenv(const char *, const char *);

/* quant.c */
void G_quant_clear(struct Quant *);
void G_quant_free(struct Quant *);
int G__quant_organize_fp_lookup(struct Quant *);
void G_quant_init(struct Quant *);
int G_quant_is_truncate(const struct Quant *);
int G_quant_is_round(const struct Quant *);
void G_quant_truncate(struct Quant *);
void G_quant_round(struct Quant *);
int G_quant_get_limits(const struct Quant *, DCELL *, DCELL *, CELL *,
		       CELL *);
int G_quant_nof_rules(const struct Quant *);
void G_quant_get_ith_rule(const struct Quant *, int, DCELL *, DCELL *, CELL *,
			  CELL *);
void G_quant_set_neg_infinite_rule(struct Quant *, DCELL, CELL);
int G_quant_get_neg_infinite_rule(const struct Quant *, DCELL *, CELL *);
void G_quant_set_pos_infinite_rule(struct Quant *, DCELL, CELL);
int G_quant_get_pos_infinite_rule(const struct Quant *, DCELL *, CELL *);
void G_quant_add_rule(struct Quant *, DCELL, DCELL, CELL, CELL);
void G_quant_reverse_rule_order(struct Quant *);
CELL G_quant_get_cell_value(struct Quant *, DCELL);
void G_quant_perform_d(struct Quant *, const DCELL *, CELL *, int);
void G_quant_perform_f(struct Quant *, const FCELL *, CELL *, int);
struct Quant_table *G__quant_get_rule_for_d_raster_val(const struct Quant *,
						       DCELL);

/* quant_io.c */
int G__quant_import(const char *, const char *, struct Quant *);
int G__quant_export(const char *, const char *, const struct Quant *);

/* quant_rw.c */
int G_truncate_fp_map(const char *, const char *);
int G_round_fp_map(const char *, const char *);
int G_quantize_fp_map(const char *, const char *, CELL, CELL);
int G_quantize_fp_map_range(const char *, const char *, DCELL, DCELL, CELL,
			    CELL);
int G_write_quant(const char *, const char *, const struct Quant *);
int G_read_quant(const char *, const char *, struct Quant *);

/* radii.c */
double G_meridional_radius_of_curvature(double, double, double);
double G_transverse_radius_of_curvature(double, double, double);
double G_radius_of_conformal_tangent_sphere(double, double, double);

/* range.c */
void G__remove_fp_range(const char *);
void G_construct_default_range(struct Range *);
int G_read_fp_range(const char *, const char *, struct FPRange *);
int G_read_range(const char *, const char *, struct Range *);
int G_write_range(const char *, const struct Range *);
int G_write_fp_range(const char *, const struct FPRange *);
void G_update_range(CELL, struct Range *);
void G_update_fp_range(DCELL, struct FPRange *);
void G_row_update_range(const CELL *, int, struct Range *);
void G__row_update_range(const CELL *, int, struct Range *, int);
void G_row_update_fp_range(const void *, int, struct FPRange *,
			  RASTER_MAP_TYPE);
void G_init_range(struct Range *);
void G_get_range_min_max(const struct Range *, CELL *, CELL *);
void G_init_fp_range(struct FPRange *);
void G_get_fp_range_min_max(const struct FPRange *, DCELL *, DCELL *);

/* raster.c */
void *G_incr_void_ptr(const void *, const size_t);
int G_raster_cmp(const void *, const void *, RASTER_MAP_TYPE);
void G_raster_cpy(void *, const void *, int, RASTER_MAP_TYPE);
void G_set_raster_value_c(void *, CELL, RASTER_MAP_TYPE);
void G_set_raster_value_f(void *, FCELL, RASTER_MAP_TYPE);
void G_set_raster_value_d(void *, DCELL, RASTER_MAP_TYPE);
CELL G_get_raster_value_c(const void *, RASTER_MAP_TYPE);
FCELL G_get_raster_value_f(const void *, RASTER_MAP_TYPE);
DCELL G_get_raster_value_d(const void *, RASTER_MAP_TYPE);

/* raster_metadata.c */
int G_read_raster_units(const char *, const char *, char *);
int G_read_raster_vdatum(const char *, const char *, char *);
int G_write_raster_units(const char *, const char *);
int G_write_raster_vdatum(const char *, const char *);
int G__raster_misc_read_line(const char *, const char *, const char *,
			     char *);
int G__raster_misc_write_line(const char *, const char *, const char *);

/* rd_cellhd.c */
char *G__read_Cell_head(FILE *, struct Cell_head *, int);
char *G__read_Cell_head_array(char **, struct Cell_head *, int);

/* reclass.c */
int G_is_reclass(const char *, const char *, char *, char *);
int G_is_reclassed_to(const char *, const char *, int *, char ***);
int G_get_reclass(const char *, const char *, struct Reclass *);
void G_free_reclass(struct Reclass *);
int G_put_reclass(const char *, const struct Reclass *);

/* remove.c */
int G_remove(const char *, const char *);
int G_remove_misc(const char *, const char *, const char *);

/* rename.c */
int G_rename_file(const char *, const char *);
int G_rename(const char *, const char *, const char *);

/* rhumbline.c */
int G_begin_rhumbline_equation(double, double, double, double);
double G_rhumbline_lat_from_lon(double);

/* rotate.c */
void G_rotate_around_point(double, double, double *, double *, double);
void G_rotate_around_point_int(int, int, int *, int *, double);

/* sample.c */
DCELL G_get_raster_sample_nearest(
    int, const struct Cell_head *, struct Categories *, double, double, int);
DCELL G_get_raster_sample_bilinear(
    int, const struct Cell_head *, struct Categories *, double, double, int);
DCELL G_get_raster_sample_cubic(
    int, const struct Cell_head *, struct Categories *, double, double, int);
DCELL G_get_raster_sample(
    int, const struct Cell_head *, struct Categories *, double, double, int,
    INTERP_TYPE);

/* seek.c */
off_t G_ftell(FILE *);
void G_fseek(FILE *, off_t, int);

/* set_window.c */
void G_get_set_window(struct Cell_head *);
int G_set_window(struct Cell_head *);

/* short_way.c */
void G_shortest_way(double *, double *);

/* sleep.c */
void G_sleep(unsigned int);

/* snprintf.c */
int G_snprintf(char *, size_t, const char *, ...)
    __attribute__ ((format(printf, 3, 4)));

/* strings.c */
int G_strcasecmp(const char *, const char *);
char *G_store(const char *);
char *G_strchg(char *, char, char);
char *G_str_replace(char *, const char *, const char *);
void G_strip(char *);
char *G_chop(char *);
void G_str_to_upper(char *);
void G_str_to_lower(char *);
int G_str_to_sql(char *);
char *G_squeeze(char *);

/* system.c */
int G_system(const char *);

/* tempfile.c */
void G_init_tempfile(void);
char *G_tempfile(void);
char *G__tempfile(int);
void G__temp_element(char *);

/* timestamp.c */
void G_init_timestamp(struct TimeStamp *);
void G_set_timestamp(struct TimeStamp *, const DateTime *);
void G_set_timestamp_range(struct TimeStamp *, const DateTime *,
			   const DateTime *);
int G__read_timestamp(FILE *, struct TimeStamp *);
int G__write_timestamp(FILE *, const struct TimeStamp *);
void G_get_timestamps(const struct TimeStamp *, DateTime *, DateTime *, int *);
int G_read_raster_timestamp(const char *, const char *, struct TimeStamp *);
int G_read_vector_timestamp(const char *, const char *, struct TimeStamp *);
int G_write_raster_timestamp(const char *, const struct TimeStamp *);
int G_write_vector_timestamp(const char *, const struct TimeStamp *);
int G_format_timestamp(const struct TimeStamp *, char *);
int G_scan_timestamp(struct TimeStamp *, const char *);
int G_remove_raster_timestamp(const char *);
int G_remove_vector_timestamp(const char *);
int G_read_grid3_timestamp(const char *, const char *, struct TimeStamp *);
int G_remove_grid3_timestamp(const char *);
int G_write_grid3_timestamp(const char *, const struct TimeStamp *);

/* token.c */
char **G_tokenize(const char *, const char *);
int G_number_of_tokens(char **);
void G_free_tokens(char **);

/* trim_dec.c */
void G_trim_decimal(char *);

/* user_config.c */
char *G_rc_path(const char *, const char *);

/* verbose.c */
int G_verbose(void);
int G_verbose_min(void);
int G_verbose_std(void);
int G_verbose_max(void);
int G_set_verbose(int);

/* view.c */
void G_3dview_warning(int);
int G_get_3dview_defaults(struct G_3dview *, struct Cell_head *);
int G_put_3dview(const char *, const char *, const struct G_3dview *,
		 const struct Cell_head *);
int G_get_3dview(const char *, const char *, struct G_3dview *);

/* whoami.c */
const char *G_whoami(void);

/* wind_2_box.c */
void G_adjust_window_to_box(const struct Cell_head *, struct Cell_head *, int,
			    int);

/* wind_format.c */
void G_format_northing(double, char *, int);
void G_format_easting(double, char *, int);
void G_format_resolution(double, char *, int);

/* wind_in.c */
int G_point_in_region(double, double);
int G_point_in_window(double, double, const struct Cell_head *);

/* wind_limits.c */
int G_limit_east(double *, int);
int G_limit_west(double *, int);
int G_limit_north(double *, int);
int G_limit_south(double *, int);

/* wind_overlap.c */
int G_window_overlap(const struct Cell_head *, double, double, double,
		     double);
double G_window_percentage_overlap(const struct Cell_head *, double, double,
				   double, double);

/* wind_scan.c */
int G_scan_northing(const char *, double *, int);
int G_scan_easting(const char *, double *, int);
int G_scan_resolution(const char *, double *, int);

/* window_map.c */
void G__create_window_mapping(int);
double G_northing_to_row(double, const struct Cell_head *);
double G_adjust_east_longitude(double, double);
double G_adjust_easting(double, const struct Cell_head *);
double G_easting_to_col(double, const struct Cell_head *);
double G_row_to_northing(double, const struct Cell_head *);
double G_col_to_easting(double, const struct Cell_head *);
int G_window_rows(void);
int G_window_cols(void);
void G__init_window(void);
int G_row_repeat_nomask(int, int);

/* worker.c */
void G_begin_execute(void (*func)(void *), void *, void **, int);
void G_end_execute(void **);
void G_init_workers(void);
void G_finish_workers(void);

/* wr_cellhd.c */
void G__write_Cell_head(FILE *, const struct Cell_head *, int);
void G__write_Cell_head3(FILE *, const struct Cell_head *, int);

/* writ_zeros.c */
void G_write_zeros(int, size_t);

/* zero.c */
void G_zero(void *, int);

/* zero_cell.c */
void G_zero_cell_buf(CELL *);
void G_zero_raster_buf(void *, RASTER_MAP_TYPE);

/* zone.c */
int G_zone(void);

#endif /* GRASS_GISDEFS_H */
