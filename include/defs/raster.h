#ifndef GRASS_RASTERDEFS_H
#define GRASS_RASTERDEFS_H

#include <grass/gis.h>

/* --- ANSI prototypes for the lib/raster functions --- */

/* align_window.c */
void Rast_align_window(struct Cell_head *, const struct Cell_head *);

/* alloc_cell.c */
size_t Rast_cell_size(RASTER_MAP_TYPE);
void *Rast_allocate_buf(RASTER_MAP_TYPE);
CELL *Rast_allocate_c_buf(void);
FCELL *Rast_allocate_f_buf(void);
DCELL *Rast_allocate_d_buf(void);
char *Rast_allocate_null_buf(void);
unsigned char *Rast__allocate_null_bits(int);
int Rast__null_bitstream_size(int);

void *Rast_allocate_input_buf(RASTER_MAP_TYPE);
CELL *Rast_allocate_c_input_buf(void);
FCELL *Rast_allocate_f_input_buf(void);
DCELL *Rast_allocate_d_input_buf(void);
char *Rast_allocate_null_input_buf(void);

void *Rast_allocate_output_buf(RASTER_MAP_TYPE);
CELL *Rast_allocate_c_output_buf(void);
FCELL *Rast_allocate_f_output_buf(void);
DCELL *Rast_allocate_d_output_buf(void);
char *Rast_allocate_null_output_buf(void);

/* auto_mask.c */
int Rast__check_for_auto_masking(void);
void Rast_suppress_masking(void);
void Rast_unsuppress_masking(void);

/* cats.c */
int Rast_read_cats(const char *, const char *, struct Categories *);
int Rast_read_vector_cats(const char *, const char *, struct Categories *);
CELL Rast_get_max_c_cat(const char *, const char *);
char *Rast_get_cats_title(const struct Categories *);
char *Rast_get_c_cat(CELL *, struct Categories *);
char *Rast_get_f_cat(FCELL *, struct Categories *);
char *Rast_get_d_cat(DCELL *, struct Categories *);
char *Rast_get_cat(void *, struct Categories *, RASTER_MAP_TYPE);
void Rast_unmark_cats(struct Categories *);
void Rast_mark_c_cats(const CELL *, int, struct Categories *);
void Rast_mark_f_cats(const FCELL *, int, struct Categories *);
void Rast_mark_d_cats(const DCELL *, int, struct Categories *);
int Rast_mark_cats(const void *, int, struct Categories *, RASTER_MAP_TYPE);
void Rast_rewind_cats(struct Categories *);
char *Rast_get_next_marked_d_cat(struct Categories *, DCELL *, DCELL *,
				 long *);
char *Rast_get_next_marked_c_cat(struct Categories *, CELL *, CELL *,
				 long *);
char *Rast_get_next_marked_f_cat(struct Categories *, FCELL *, FCELL *,
				     long *);
char *Rast_get_next_marked_cat(struct Categories *, void *, void *,
			       long *, RASTER_MAP_TYPE);
int Rast_set_c_cat(const CELL *, const CELL *, const char *, struct Categories *);
int Rast_set_f_cat(const FCELL *, const FCELL *, const char *, struct Categories *);
int Rast_set_d_cat(const DCELL *, const DCELL *, const char *, struct Categories *);
int Rast_set_cat(const void *, const void *, const char *, struct Categories *,
		 RASTER_MAP_TYPE);
void Rast_write_cats(const char *, struct Categories *);
void Rast_write_vector_cats(const char *, struct Categories *);
char *Rast_get_ith_d_cat(const struct Categories *, int, DCELL *,
			 DCELL *);
char *Rast_get_ith_f_cat(const struct Categories *, int, void *, void *);
char *Rast_get_ith_c_cat(const struct Categories *, int, void *, void *);
char *Rast_get_ith_cat(const struct Categories *, int, void *, void *,
		       RASTER_MAP_TYPE);
void Rast_init_cats(const char *, struct Categories *);
void Rast_set_cats_title(const char *, struct Categories *);
void Rast_set_cats_fmt(const char *, double, double, double, double,
			   struct Categories *);
void Rast_free_cats(struct Categories *);
void Rast_copy_cats(struct Categories *, const struct Categories *);
int Rast_number_of_cats(struct Categories *);
int Rast_sort_cats(struct Categories *);

/* cell_stats.c */
void Rast_init_cell_stats(struct Cell_stats *);
int Rast_update_cell_stats(const CELL *, int, struct Cell_stats *);
int Rast_find_cell_stat(CELL, long *, const struct Cell_stats *);
int Rast_rewind_cell_stats(struct Cell_stats *);
int Rast_next_cell_stat(CELL *, long *, struct Cell_stats *);
void Rast_get_stats_for_null_value(long *, const struct Cell_stats *);
void Rast_free_cell_stats(struct Cell_stats *);

/* cell_title.c */
char *Rast_get_cell_title(const char *, const char *);

/* cellstats_eq.c */
int Rast_cell_stats_histo_eq(struct Cell_stats *, CELL, CELL, CELL, CELL, int,
			     void (*)(CELL, CELL, CELL));

/* close.c */
void Rast_close(int);
void Rast_unopen(int);
void Rast__unopen_all(void);

/* color_compat.c */
void Rast_make_ryg_colors(struct Colors *, CELL, CELL);
void Rast_make_ryg_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_aspect_colors(struct Colors *, CELL, CELL);
void Rast_make_aspect_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_byr_colors(struct Colors *, CELL, CELL);
void Rast_make_byr_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_bgyr_colors(struct Colors *, CELL, CELL);
void Rast_make_bgyr_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_byg_colors(struct Colors *, CELL, CELL);
void Rast_make_byg_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_grey_scale_colors(struct Colors *, CELL, CELL);
void Rast_make_grey_scale_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_gyr_colors(struct Colors *, CELL, CELL);
void Rast_make_gyr_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_rainbow_colors(struct Colors *, CELL, CELL);
void Rast_make_rainbow_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_ramp_colors(struct Colors *, CELL, CELL);
void Rast_make_ramp_fp_colors(struct Colors *, DCELL, DCELL);
void Rast_make_wave_colors(struct Colors *, CELL, CELL);
void Rast_make_wave_fp_colors(struct Colors *, DCELL, DCELL);

/* color_free.c */
void Rast_free_colors(struct Colors *);
void Rast__color_free_rules(struct _Color_Info_ *);
void Rast__color_free_lookup(struct _Color_Info_ *);
void Rast__color_free_fp_lookup(struct _Color_Info_ *);
void Rast__color_reset(struct Colors *);

/* color_get.c */
int Rast_get_color(const void *, int *, int *, int *, struct Colors *,
		   RASTER_MAP_TYPE);
int Rast_get_c_color(const CELL *, int *, int *, int *, struct Colors *);
int Rast_get_f_color(const FCELL *, int *, int *, int *, struct Colors *);
int Rast_get_d_color(const DCELL *, int *, int *, int *, struct Colors *);
void Rast_get_null_value_color(int *, int *, int *, const struct Colors *);
void Rast_get_default_color(int *, int *, int *, const struct Colors *);

/* color_hist.c */
void Rast_make_histogram_eq_colors(struct Colors *, struct Cell_stats *);
void Rast_make_histogram_log_colors(struct Colors *, struct Cell_stats *, int, int);

/* color_init.c */
void Rast_init_colors(struct Colors *);

/* color_insrt.c */
int Rast__insert_color_into_lookup(CELL, int, int, int, struct _Color_Info_ *);

/* color_invrt.c */
void Rast_invert_colors(struct Colors *);

/* color_look.c */
void Rast_lookup_c_colors(const CELL *, unsigned char *, unsigned char *,
			  unsigned char *, unsigned char *, int,
			  struct Colors *);
void Rast_lookup_colors(const void *, unsigned char *, unsigned char *,
			unsigned char *, unsigned char *, int,
			struct Colors *, RASTER_MAP_TYPE);
void Rast_lookup_f_colors(const FCELL *, unsigned char *, unsigned char *,
			  unsigned char *, unsigned char *, int,
			  struct Colors *);
void Rast_lookup_d_colors(const DCELL *, unsigned char *, unsigned char *,
			  unsigned char *, unsigned char *, int,
			  struct Colors *);
void Rast__lookup_colors(const void *, unsigned char *, unsigned char *,
			 unsigned char *, unsigned char *, int, struct Colors *,
			 int, int, RASTER_MAP_TYPE);
void Rast__interpolate_color_rule(DCELL, unsigned char *, unsigned char *,
				  unsigned char *, const struct _Color_Rule_ *);

/* color_org.c */
void Rast__organize_colors(struct Colors *);

/* color_out.c */
void Rast_print_colors(struct Colors *, DCELL, DCELL, FILE *, int);

/* color_rand.c */
void Rast_make_random_colors(struct Colors *, CELL, CELL);

/* color_range.c */
void Rast_set_c_color_range(CELL, CELL, struct Colors *);
void Rast_set_d_color_range(DCELL, DCELL, struct Colors *);
void Rast_get_c_color_range(CELL *, CELL *, const struct Colors *);
void Rast_get_d_color_range(DCELL *, DCELL *, const struct Colors *);

/* color_read.c */
int Rast_read_colors(const char *, const char *, struct Colors *);
int Rast__read_colors(const char *, const char *, const char *, struct Colors *);
void Rast_mark_colors_as_fp(struct Colors *);

/* color_remove.c */
int Rast_remove_colors(const char *, const char *);

/* color_rule.c */
void Rast_add_d_color_rule(const DCELL *, int, int, int,
			   const DCELL *, int, int, int,
			   struct Colors *);
void Rast_add_f_color_rule(const FCELL *, int, int, int,
			   const FCELL *, int, int, int,
			   struct Colors *);
void Rast_add_c_color_rule(const CELL *, int, int, int,
			   const CELL *, int, int, int,
			   struct Colors *);
void Rast_add_color_rule(const void *, int, int, int,
			 const void *, int, int, int,
			 struct Colors *, RASTER_MAP_TYPE);
int Rast_add_modular_d_color_rule(const DCELL *, int, int, int,
				  const DCELL *, int, int, int,
				  struct Colors *);
int Rast_add_modular_f_color_rule(const FCELL *, int, int, int,
				  const FCELL *, int, int, int,
				  struct Colors *);
int Rast_add_modular_c_color_rule(const CELL *, int, int, int,
				  const CELL *, int, int, int,
				  struct Colors *);
int Rast_add_modular_color_rule(const void *, int, int, int,
				const void *, int, int, int,
				struct Colors *, RASTER_MAP_TYPE);

/* color_rule_get.c */
int Rast_colors_count(const struct Colors *);
int Rast_get_fp_color_rule(DCELL *, unsigned char *, unsigned char *,
		       unsigned char *, DCELL *, unsigned char *,
		       unsigned char *, unsigned char *,
		       const struct Colors *, int);

/* color_rules.c */
typedef int read_rule_fn(void *, DCELL, DCELL,
			 DCELL *, int *, int *, int *, int *, int *, int *);
int Rast_parse_color_rule(DCELL, DCELL, const char *, DCELL *, int *, int *,
		       int *, int *, int *, int *);
const char *Rast_parse_color_rule_error(int);
int Rast_read_color_rule(void *, DCELL, DCELL, DCELL *, int *, int *, int *,
		      int *, int *, int *);
int Rast_read_color_rules(struct Colors *, DCELL, DCELL, read_rule_fn *, void *);
int Rast_load_colors(struct Colors *, const char *, CELL, CELL);
int Rast_load_fp_colors(struct Colors *, const char *, DCELL, DCELL);
void Rast_make_colors(struct Colors *, const char *, CELL, CELL);
void Rast_make_fp_colors(struct Colors *, const char *, DCELL, DCELL);

/* color_set.c */
void Rast_set_c_color(CELL, int, int, int, struct Colors *);
void Rast_set_d_color(DCELL, int, int, int, struct Colors *);
void Rast_set_null_value_color(int, int, int, struct Colors *);
void Rast_set_default_color(int, int, int, struct Colors *);

/* color_shift.c */
void Rast_shift_c_colors(CELL, struct Colors *);
void Rast_shift_d_colors(DCELL, struct Colors *);

/* color_write.c */
void Rast_write_colors(const char *, const char *, struct Colors *);
void Rast__write_colors(FILE *, struct Colors *);

/* color_xform.c */
void Rast_histogram_eq_colors(struct Colors *, struct Colors *,
			   struct Cell_stats *);
void Rast_histogram_eq_fp_colors(struct Colors *,
			      struct Colors *, struct FP_stats *);
void Rast_log_colors(struct Colors *, struct Colors *, int);
void Rast_abs_log_colors(struct Colors *, struct Colors *, int);

/* format.c */
int Rast__check_format(int);
int Rast__read_row_ptrs(int);
int Rast__write_row_ptrs(int);

/* fpreclass.c */
void Rast_fpreclass_clear(struct FPReclass *);
void Rast_fpreclass_reset(struct FPReclass *);
void Rast_fpreclass_init(struct FPReclass *);
void Rast_fpreclass_set_domain(struct FPReclass *, DCELL, DCELL);
void Rast_fpreclass_set_range(struct FPReclass *, DCELL, DCELL);
int Rast_fpreclass_get_limits(const struct FPReclass *, DCELL *, DCELL *,
			   DCELL *, DCELL *);
int Rast_fpreclass_nof_rules(const struct FPReclass *);
void Rast_fpreclass_get_ith_rule(const struct FPReclass *, int, DCELL *, DCELL *,
			      DCELL *, DCELL *);
void Rast_fpreclass_set_neg_infinite_rule(struct FPReclass *, DCELL, DCELL);
int Rast_fpreclass_get_neg_infinite_rule(const struct FPReclass *, DCELL *,
				      DCELL *);
void Rast_fpreclass_set_pos_infinite_rule(struct FPReclass *, DCELL, DCELL);
int Rast_fpreclass_get_pos_infinite_rule(const struct FPReclass *, DCELL *,
				      DCELL *);
void Rast_fpreclass_add_rule(struct FPReclass *, DCELL, DCELL, DCELL, DCELL);
void Rast_fpreclass_reverse_rule_order(struct FPReclass *);
DCELL Rast_fpreclass_get_cell_value(const struct FPReclass *, DCELL);
void Rast_fpreclass_perform_di(const struct FPReclass *, const DCELL *, CELL *,
			    int);
void Rast_fpreclass_perform_df(const struct FPReclass *, const DCELL *, FCELL *,
			    int);
void Rast_fpreclass_perform_dd(const struct FPReclass *, const DCELL *, DCELL *,
			    int);
void Rast_fpreclass_perform_fi(const struct FPReclass *, const FCELL *, CELL *,
			    int);
void Rast_fpreclass_perform_ff(const struct FPReclass *, const FCELL *, FCELL *,
			    int);
void Rast_fpreclass_perform_fd(const struct FPReclass *, const FCELL *, DCELL *,
			    int);
void Rast_fpreclass_perform_ii(const struct FPReclass *, const CELL *, CELL *,
			    int);
void Rast_fpreclass_perform_if(const struct FPReclass *, const CELL *, FCELL *,
			    int);
void Rast_fpreclass_perform_id(const struct FPReclass *, const CELL *, DCELL *,
			    int);
/* gdal.c */
void Rast_init_gdal(void);
struct GDAL_link *Rast_get_gdal_link(const char *, const char *);
struct GDAL_link *Rast_create_gdal_link(const char *, RASTER_MAP_TYPE);
void Rast_close_gdal_link(struct GDAL_link *);
int Rast_close_gdal_write_link(struct GDAL_link *);

/* get_cellhd.c */
void Rast_get_cellhd(const char *, const char *, struct Cell_head *);

/* get_row.c */
void Rast_get_row_nomask(int, void *, int, RASTER_MAP_TYPE);
void Rast_get_c_row_nomask(int, CELL *, int);
void Rast_get_f_row_nomask(int, FCELL *, int);
void Rast_get_d_row_nomask(int, DCELL *, int);
void Rast_get_row(int, void *, int, RASTER_MAP_TYPE);
void Rast_get_c_row(int, CELL *, int);
void Rast_get_f_row(int, FCELL *, int);
void Rast_get_d_row(int, DCELL *, int);
void Rast_get_null_value_row(int, char *, int);

/* get_row_colr.c */
void Rast_get_row_colors(int, int, struct Colors *,
			 unsigned char *, unsigned char *, unsigned char *,
			 unsigned char *);
/* histo_eq.c */
void Rast_histogram_eq(const struct Histogram *, unsigned char **,
		       CELL *, CELL *);

/* histogram.c */
void Rast_init_histogram(struct Histogram *);
int Rast_read_histogram(const char *, const char *, struct Histogram *);
void Rast_write_histogram(const char *, const struct Histogram *);
void Rast_write_histogram_cs(const char *, struct Cell_stats *);
void Rast_make_histogram_cs(struct Cell_stats *, struct Histogram *);
int Rast_get_histogram_num(const struct Histogram *);
CELL Rast_get_histogram_cat(int, const struct Histogram *);
long Rast_get_histogram_count(int, const struct Histogram *);
void Rast_free_histogram(struct Histogram *);
int Rast_sort_histogram(struct Histogram *);
int Rast_sort_histogram_by_count(struct Histogram *);
void Rast_remove_histogram(const char *);
int Rast_add_histogram(CELL, long, struct Histogram *);
int Rast_set_histogram(CELL, long, struct Histogram *);
void Rast_extend_histogram(CELL, long, struct Histogram *);
void Rast_zero_histogram(struct Histogram *);

/* history.c */
int Rast__read_history(struct History *, FILE *);
int Rast_read_history(const char *, const char *, struct History *);
void Rast__write_history(struct History *, FILE *);
void Rast_write_history(const char *, struct History *);
void Rast_short_history(const char *, const char *, struct History *);
int Rast_command_history(struct History *);
void Rast_append_history(struct History *, const char *);
void Rast_append_format_history(struct History *, const char *, ...)
    __attribute__ ((format(printf, 2, 3)));
const char *Rast_get_history(struct History *, int);
void Rast_set_history(struct History *, int, const char *);
void Rast_format_history(struct History *, int, const char *, ...)
    __attribute__ ((format(printf, 3, 4)));
void Rast_clear_history(struct History *);
void Rast_free_history(struct History *);
int Rast_history_length(struct History *);
const char *Rast_history_line(struct History *, int);

/* init.c */
void Rast_init(void);
void Rast__check_init(void);
void Rast_init_all(void);
void Rast__init(void);
void Rast__error_handler(void *);

/* interp.c */
DCELL Rast_interp_linear(double, DCELL, DCELL);
DCELL Rast_interp_bilinear(double, double, DCELL, DCELL, DCELL, DCELL);
DCELL Rast_interp_cubic(double, DCELL, DCELL, DCELL, DCELL);
DCELL Rast_interp_bicubic(double, double,
		       DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL,
		       DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL,
		       DCELL);
DCELL Rast_interp_lanczos(double, double, DCELL *);
DCELL Rast_interp_cubic_bspline(double, DCELL, DCELL, DCELL, DCELL);
DCELL Rast_interp_bicubic_bspline(double, double,
		       DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL,
		       DCELL, DCELL, DCELL, DCELL, DCELL, DCELL, DCELL,
		       DCELL);
int Rast_option_to_interp_type(const struct Option *);

/* mask_info.c */
char *Rast_mask_info(void);
int Rast__mask_info(char *, char *);

/* maskfd.c */
int Rast_maskfd(void);

/* null_val.c */
#define Rast_is_c_null_value(cellVal)	\
    (*(const CELL *)(cellVal) == (CELL) 0x80000000)
#define Rast_is_f_null_value(fcellVal)	\
    (*(const FCELL *)(fcellVal) != *(const FCELL *)(fcellVal))
#define Rast_is_d_null_value(dcellVal)	\
    (*(const DCELL *)(dcellVal) != *(const DCELL *)(dcellVal))

void Rast__set_null_value(void *, int, int, RASTER_MAP_TYPE);
void Rast_set_null_value(void *, int, RASTER_MAP_TYPE);
void Rast_set_c_null_value(CELL *, int);
void Rast_set_f_null_value(FCELL *, int);
void Rast_set_d_null_value(DCELL *, int);
int Rast_is_null_value(const void *, RASTER_MAP_TYPE);
#ifndef Rast_is_c_null_value
int Rast_is_c_null_value(const CELL *);
#endif
#ifndef Rast_is_f_null_value
int Rast_is_f_null_value(const FCELL *);
#endif
#ifndef Rast_is_d_null_value
int Rast_is_d_null_value(const DCELL *);
#endif
void Rast_insert_null_values(void *, char *, int, RASTER_MAP_TYPE);
void Rast_insert_c_null_values(CELL *, char *, int);
void Rast_insert_f_null_values(FCELL *, char *, int);
void Rast_insert_d_null_values(DCELL *, char *, int);
int Rast__check_null_bit(const unsigned char *, int, int);
void Rast__convert_01_flags(const char *, unsigned char *, int);
void Rast__convert_flags_01(char *, const unsigned char *, int);
void Rast__init_null_bits(unsigned char *, int);

/* open.c */
int Rast_open_old(const char *, const char *);
int Rast__open_old(const char *, const char *);
int Rast_open_c_new(const char *);
int Rast_open_c_new_uncompressed(const char *);
void Rast_want_histogram(int);
void Rast_set_cell_format(int);
int Rast_get_cell_format(CELL);
int Rast_open_fp_new(const char *);
int Rast_open_fp_new_uncompressed(const char *);
void Rast_set_fp_type(RASTER_MAP_TYPE);
int Rast_map_is_fp(const char *, const char *);
RASTER_MAP_TYPE Rast_map_type(const char *, const char *);
RASTER_MAP_TYPE Rast__check_fp_type(const char *, const char *);
RASTER_MAP_TYPE Rast_get_map_type(int);
int Rast_open_new(const char *, RASTER_MAP_TYPE);
int Rast_open_new_uncompressed(const char *, RASTER_MAP_TYPE);
void Rast_set_quant_rules(int, struct Quant *);

/* put_cellhd.c */
void Rast_put_cellhd(const char *, struct Cell_head *);

/* put_row.c */
void Rast_put_row(int, const void *, RASTER_MAP_TYPE);
void Rast_put_c_row(int, const CELL *);
void Rast_put_f_row(int, const FCELL *);
void Rast_put_d_row(int, const DCELL *);
int Rast__open_null_write(int);
void Rast__write_null_bits(int, const unsigned char *, int, int, int);

/* put_title.c */
int Rast_put_cell_title(const char *, const char *);

/* quant.c */
void Rast_quant_clear(struct Quant *);
void Rast_quant_free(struct Quant *);
int Rast__quant_organize_fp_lookup(struct Quant *);
void Rast_quant_init(struct Quant *);
int Rast_quant_is_truncate(const struct Quant *);
int Rast_quant_is_round(const struct Quant *);
void Rast_quant_truncate(struct Quant *);
void Rast_quant_round(struct Quant *);
int Rast_quant_get_limits(const struct Quant *, DCELL *, DCELL *, CELL *,
		       CELL *);
int Rast_quant_nof_rules(const struct Quant *);
void Rast_quant_get_ith_rule(const struct Quant *, int, DCELL *, DCELL *, CELL *,
			  CELL *);
void Rast_quant_set_neg_infinite_rule(struct Quant *, DCELL, CELL);
int Rast_quant_get_neg_infinite_rule(const struct Quant *, DCELL *, CELL *);
void Rast_quant_set_pos_infinite_rule(struct Quant *, DCELL, CELL);
int Rast_quant_get_pos_infinite_rule(const struct Quant *, DCELL *, CELL *);
void Rast_quant_add_rule(struct Quant *, DCELL, DCELL, CELL, CELL);
void Rast_quant_reverse_rule_order(struct Quant *);
CELL Rast_quant_get_cell_value(struct Quant *, DCELL);
void Rast_quant_perform_d(struct Quant *, const DCELL *, CELL *, int);
void Rast_quant_perform_f(struct Quant *, const FCELL *, CELL *, int);
struct Quant_table *Rast__quant_get_rule_for_d_raster_val(const struct Quant *,
						       DCELL);

/* quant_io.c */
int Rast__quant_import(const char *, const char *, struct Quant *);
int Rast__quant_export(const char *, const char *, const struct Quant *);

/* quant_rw.c */
void Rast_truncate_fp_map(const char *, const char *);
void Rast_round_fp_map(const char *, const char *);
void Rast_quantize_fp_map(const char *, const char *, CELL, CELL);
void Rast_quantize_fp_map_range(const char *, const char *, DCELL, DCELL, CELL,
				CELL);
void Rast_write_quant(const char *, const char *, const struct Quant *);
int Rast_read_quant(const char *, const char *, struct Quant *);

/* range.c */
void Rast__remove_fp_range(const char *);
void Rast_construct_default_range(struct Range *);
int Rast_read_fp_range(const char *, const char *, struct FPRange *);
int Rast_read_range(const char *, const char *, struct Range *);
void Rast_write_range(const char *, const struct Range *);
void Rast_write_fp_range(const char *, const struct FPRange *);
void Rast_update_range(CELL, struct Range *);
void Rast_update_fp_range(DCELL, struct FPRange *);
void Rast_row_update_range(const CELL *, int, struct Range *);
void Rast__row_update_range(const CELL *, int, struct Range *, int);
void Rast_row_update_fp_range(const void *, int, struct FPRange *,
			  RASTER_MAP_TYPE);
void Rast_init_range(struct Range *);
void Rast_get_range_min_max(const struct Range *, CELL *, CELL *);
void Rast_init_fp_range(struct FPRange *);
void Rast_get_fp_range_min_max(const struct FPRange *, DCELL *, DCELL *);

/* raster.c */
int Rast_raster_cmp(const void *, const void *, RASTER_MAP_TYPE);
void Rast_raster_cpy(void *, const void *, int, RASTER_MAP_TYPE);
void Rast_set_c_value(void *, CELL, RASTER_MAP_TYPE);
void Rast_set_f_value(void *, FCELL, RASTER_MAP_TYPE);
void Rast_set_d_value(void *, DCELL, RASTER_MAP_TYPE);
CELL Rast_get_c_value(const void *, RASTER_MAP_TYPE);
FCELL Rast_get_f_value(const void *, RASTER_MAP_TYPE);
DCELL Rast_get_d_value(const void *, RASTER_MAP_TYPE);

/* raster_metadata.c */
char *Rast_read_units(const char *, const char *);
char *Rast_read_vdatum(const char *, const char *);
void Rast_write_units(const char *, const char *);
void Rast_write_vdatum(const char *, const char *);

/* reclass.c */
int Rast_is_reclass(const char *, const char *, char *, char *);
int Rast_is_reclassed_to(const char *, const char *, int *, char ***);
int Rast_get_reclass(const char *, const char *, struct Reclass *);
void Rast_free_reclass(struct Reclass *);
int Rast_put_reclass(const char *, const struct Reclass *);

/* sample.c */
DCELL Rast_get_sample_nearest(int, const struct Cell_head *, struct Categories *, double, double, int);
DCELL Rast_get_sample_bilinear(int, const struct Cell_head *, struct Categories *, double, double, int);
DCELL Rast_get_sample_cubic(int, const struct Cell_head *, struct Categories *, double, double, int);
DCELL Rast_get_sample(int, const struct Cell_head *, struct Categories *, double, double, int, INTERP_TYPE);

/* set_window.c */
void Rast__init_window(void);
void Rast_set_window(struct Cell_head *);
void Rast_unset_window(void);
void Rast_set_output_window(struct Cell_head *);
void Rast_set_input_window(struct Cell_head *);

/* window.c */
void Rast_get_window(struct Cell_head *);
void Rast_get_input_window(struct Cell_head *);
void Rast_get_output_window(struct Cell_head *);
int Rast_window_rows(void);
int Rast_window_cols(void);
int Rast_input_window_rows(void);
int Rast_input_window_cols(void);
int Rast_output_window_rows(void);
int Rast_output_window_cols(void);
double Rast_northing_to_row(double, const struct Cell_head *);
double Rast_easting_to_col(double, const struct Cell_head *);
double Rast_row_to_northing(double, const struct Cell_head *);
double Rast_col_to_easting(double, const struct Cell_head *);

/* window_map.c */
void Rast__create_window_mapping(int);
int Rast_row_repeat_nomask(int, int);

/* zero_cell.c */
void Rast_zero_buf(void *, RASTER_MAP_TYPE);
void Rast_zero_input_buf(void *, RASTER_MAP_TYPE);
void Rast_zero_output_buf(void *, RASTER_MAP_TYPE);

#endif /* GRASS_RASTERDEFS_H */
