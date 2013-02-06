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

#include <grass/config.h>

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

#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>

/* adj_cellhd.c */
void G_adjust_Cell_head(struct Cell_head *, int, int);
void G_adjust_Cell_head3(struct Cell_head *, int, int, int);

/* alloc.c */
#define G_incr_void_ptr(ptr, size) \
    ((void *)((const unsigned char *)(ptr) + (size)))

void *G__malloc(const char *, int, size_t);
void *G__calloc(const char *, int, size_t, size_t);
void *G__realloc(const char *, int, void *, size_t);
void G_free(void *);
#ifndef G_incr_void_ptr
void *G_incr_void_ptr(const void *, size_t);
#endif

#ifndef CTYPESGEN
#define G_malloc(n)     G__malloc(__FILE__, __LINE__, (n))
#define G_calloc(m, n)  G__calloc(__FILE__, __LINE__, (m), (n))
#define G_realloc(p, n) G__realloc(__FILE__, __LINE__, (p), (n))
#else
#define G_malloc(n)     G__malloc("<ctypesgen>", 0, (n))
#define G_calloc(m, n)  G__calloc("<ctypesgen>", 0, (m), (n))
#define G_realloc(p, n) G__realloc("<ctypesgen>", 0, (p), (n))
#endif

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
int G_vasprintf(char **, const char *, va_list);
int G_asprintf(char **, const char *, ...)
    __attribute__ ((format(printf, 2, 3)));

int G_rasprintf(char **, size_t *,const char *, ...)
    __attribute__ ((format(printf, 3, 4)));

/* basename.c */
char *G_basename(char *, const char *);

/* bres_line.c */
void G_bresenham_line(int, int, int, int, int (*)(int, int));

/* clicker.c */
void G_clicker(void);

/* color_rules.c */
char *G_color_rules_options(void);
char *G_color_rules_descriptions(void);
void G_list_color_rules(FILE *);
int G_find_color_rule(const char *);

/* color_str.c */
int G_num_standard_colors(void);

/* commas.c */
int G_insert_commas(char *);
void G_remove_commas(char *);

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
char *G_file_name(char *, const char *, const char *, const char *);
char *G_file_name_misc(char *, const char *, const char *, const char *,
		       const char *);

/* find_file.c */
const char *G_find_file(const char *, char *, const char *);
const char *G_find_file2(const char *, const char *, const char *);
const char *G_find_file_misc(const char *, const char *, char *, const char *);
const char *G_find_file2_misc(const char *, const char *, const char *,
			      const char *);

/* find_etc.c */
char *G_find_etc(const char *);

/* find_rast.c */
const char *G_find_raster(char *, const char *);
const char *G_find_raster2(const char *, const char *);

/* find_rast3d.c */
const char *G_find_raster3d(const char *, const char *);

/* find_vect.c */
const char *G_find_vector(char *, const char *);
const char *G_find_vector2(const char *, const char *);

/* flate.c */
int G_zlib_compress(const unsigned char *, int, unsigned char *, int);
int G_zlib_expand(const unsigned char *, int, unsigned char *, int);
int G_zlib_write(int, const unsigned char *, int);
int G_zlib_read(int, int, unsigned char *, int);
int G_zlib_write_noCompress(int, const unsigned char *, int);

/* geodesic.c */
int G_begin_geodesic_equation(double, double, double, double);
double G_geodesic_lat_from_lon(double);

/* geodist.c */
void G_begin_geodesic_distance(double, double);
void G_set_geodesic_distance_lat1(double);
void G_set_geodesic_distance_lat2(double);
double G_geodesic_distance_lon_to_lon(double, double);
double G_geodesic_distance(double, double, double, double);

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

/* get_window.c */
void G_get_window(struct Cell_head *);
void G_get_default_window(struct Cell_head *);
void G__get_window(struct Cell_head *, const char *, const char *,
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

/* handler.c */
void G_add_error_handler(void (*)(void *), void *);
void G_remove_error_handler(void (*)(void *), void *);
void G__call_error_handlers(void);

/* home.c */
const char *G_home(void);
const char *G__home(void);

/* ilist.c */
void G_init_ilist(struct ilist *);
void G_ilist_add(struct ilist *, int);

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
void G_set_key_value(const char *, const char *, struct Key_Value *);
const char *G_find_key_value(const char *, const struct Key_Value *);
void G_free_key_value(struct Key_Value *);

/* key_value2.c */
int G_fwrite_key_value(FILE *, const struct Key_Value *);
struct Key_Value *G_fread_key_value(FILE *);

/* key_value3.c */
void G_write_key_value_file(const char *, const struct Key_Value *);
struct Key_Value *G_read_key_value_file(const char *);

/* key_value4.c */
void G_update_key_value_file(const char *, const char *, const char *);
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

/* ls.c */
void G_set_ls_filter(int (*)(const char *, void *), void *);
void G_set_ls_exclude_filter(int (*)(const char *, void *), void *);
char **G__ls(const char *, int *);
void G_ls(const char *, FILE *);
void G_ls_format(char **, int, int, FILE *);

/* ls_filter.c */
#ifdef HAVE_REGEX_H
void *G_ls_regex_filter(const char *, int, int);
void *G_ls_glob_filter(const char *, int);
void G_free_ls_filter(void *);
#endif

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
char *G_mapset_path(void);

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

/* myname.c */
char *G_myname(void);

/* named_colr.c */
int G_color_values(const char *, float *, float *, float *);
const char *G_color_name(int);

/* nl_to_spaces.c */
void G_newlines_to_spaces(char *);

/* nme_in_mps.c */
int G_name_is_fully_qualified(const char *, char *, char *);
char *G_fully_qualified_name(const char *, const char *);
int G_unqualified_name(const char *, const char *, char *, char *);

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

/* overwrite.c */
int G_check_overwrite(int argc, char **argv);

/* pager.c */
FILE *G_open_pager(struct Popen *);
void G_close_pager(struct Popen *);
FILE *G_open_mail(struct Popen *);
void G_close_mail(struct Popen *);

/* parser.c */
void G_disable_interactive(void);
struct GModule *G_define_module(void);
struct Flag *G_define_flag(void);
struct Option *G_define_option(void);
struct Option *G_define_standard_option(int);
struct Flag *G_define_standard_flag(int);
int G_parser(int, char **);
void G_usage(void);
char *G_recreate_command(void);
void G_add_keyword(const char *);
void G_set_keywords(const char *);
int G_get_overwrite();
char* G_option_to_separator(const struct Option *);

/* paths.c */
int G_mkdir(const char *);
int G_is_dirsep(char);
int G_is_absolute_path(const char *);
char *G_convert_dirseps_to_host(char *);
char *G_convert_dirseps_from_host(char *);
int G_lstat(const char *, STRUCT_STAT *);
int G_stat(const char *, STRUCT_STAT *);
int G_owner(const char *);

/* percent.c */
void G_percent(long, long, int);
void G_percent_reset(void);
void G_progress(long, int);
void G_set_percent_routine(int (*) (int));
void G_unset_percent_routine(void);

/* popen.c */
void G_popen_clear(struct Popen *);
FILE *G_popen_write(struct Popen *, const char *, const char **);
FILE *G_popen_read(struct Popen *, const char *, const char **);
void G_popen_close(struct Popen *);

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

/* progrm_nme.c */
const char *G_program_name(void);
void G_set_program_name(const char *);

/* proj1.c */
int G_projection(void);

/* proj2.c */
int G__projection_units(int);
const char *G__projection_name(int);

/* proj3.c */
const char *G_database_unit_name(int);
const char *G_database_projection_name(void);
const char *G_database_datum_name(void);
const char *G_database_ellipse_name(void);
double G_database_units_to_meters_factor(void);

/* put_window.c */
int G_put_window(const struct Cell_head *);
int G__put_window(const struct Cell_head *, const char *, const char *);

/* putenv.c */
void G_putenv(const char *, const char *);

/* radii.c */
double G_meridional_radius_of_curvature(double, double, double);
double G_transverse_radius_of_curvature(double, double, double);
double G_radius_of_conformal_tangent_sphere(double, double, double);

/* rd_cellhd.c */
void G__read_Cell_head(FILE *, struct Cell_head *, int);
void G__read_Cell_head_array(char **, struct Cell_head *, int);

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

/* seek.c */
off_t G_ftell(FILE *);
void G_fseek(FILE *, off_t, int);

/* set_window.c */
void G_get_set_window(struct Cell_head *);
void G_set_window(struct Cell_head *);
void G_unset_window();

/* short_way.c */
void G_shortest_way(double *, double *);

/* sleep.c */
void G_sleep(unsigned int);

/* snprintf.c */
int G_snprintf(char *, size_t, const char *, ...)
    __attribute__ ((format(printf, 3, 4)));

/* strings.c */
int G_strcasecmp(const char *, const char *);
int G_strncasecmp(const char *, const char *, int);
char *G_store(const char *);
char *G_strchg(char *, char, char);
char *G_str_replace(const char *, const char *, const char *);
void G_strip(char *);
char *G_chop(char *);
void G_str_to_upper(char *);
void G_str_to_lower(char *);
int G_str_to_sql(char *);
void G_squeeze(char *);
char *G_strcasestr(const char *, const char *);

/* tempfile.c */
void G_init_tempfile(void);
char *G_tempfile(void);
char *G__tempfile(int);
void G__temp_element(char *);

/* timestamp.c */
void G_init_timestamp(struct TimeStamp *);
void G_set_timestamp(struct TimeStamp *, const struct DateTime *);
void G_set_timestamp_range(struct TimeStamp *, const struct DateTime *,
			   const struct DateTime *);
int G__read_timestamp(FILE *, struct TimeStamp *);
int G__write_timestamp(FILE *, const struct TimeStamp *);
void G_get_timestamps(const struct TimeStamp *, struct DateTime *, struct DateTime *, int *);
int G_format_timestamp(const struct TimeStamp *, char *);
int G_scan_timestamp(struct TimeStamp *, const char *);
int G_has_raster_timestamp(const char *, const char *);
int G_read_raster_timestamp(const char *, const char *, struct TimeStamp *);
int G_write_raster_timestamp(const char *, const struct TimeStamp *);
int G_remove_raster_timestamp(const char *);
int G_has_vector_timestamp(const char *, const char *, const char *);
int G_read_vector_timestamp(const char *, const char *, const char *, struct TimeStamp *);
int G_write_vector_timestamp(const char *, const char *, const struct TimeStamp *);
int G_remove_vector_timestamp(const char *, const char *);
int G_has_raster3d_timestamp(const char *, const char *);
int G_read_raster3d_timestamp(const char *, const char *, struct TimeStamp *);
int G_remove_raster3d_timestamp(const char *);
int G_write_raster3d_timestamp(const char *, const struct TimeStamp *);

/* token.c */
char **G_tokenize(const char *, const char *);
char **G_tokenize2(const char *, const char *, const char *);
int G_number_of_tokens(char **);
void G_free_tokens(char **);

/* trim_dec.c */
void G_trim_decimal(char *);

/* units.c */
double G_units_to_meters_factor(int);
double G_units_to_meters_factor_sq(int);
const char *G_get_units_name(int, int, int);
int G_units(const char *);
int G_is_units_type_spatial(int);
int G_is_units_type_temporal(int);

/* user_config.c */
#ifndef __MINGW32__
char *G_rc_path(const char *, const char *);
#endif

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
double G_adjust_east_longitude(double, double);
double G_adjust_easting(double, const struct Cell_head *);
void G__init_window(void);

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

/* xdr.c */
void G_xdr_get_int(int *, const void *);
void G_xdr_put_int(void *, const int *);
void G_xdr_get_float(float *, const void *);
void G_xdr_put_float(void *, const float *);
void G_xdr_get_double(double *, const void *);
void G_xdr_put_double(void *, const double *);

/* zero.c */
void G_zero(void *, int);

/* zone.c */
int G_zone(void);

#endif /* GRASS_GISDEFS_H */
