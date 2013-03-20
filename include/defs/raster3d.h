#ifndef GRASS_RASTER3DDEFS_H
#define GRASS_RASTER3DDEFS_H

/* cache.c */
void Rast3d_cache_reset(RASTER3D_cache *);
void Rast3d_cache_dispose(RASTER3D_cache *);
void *Rast3d_cache_new(int, int, int, write_fn *, void *, read_fn *, void *);
void Rast3d_cache_set_remove_fun(RASTER3D_cache *, write_fn *, void *);
void Rast3d_cache_set_load_fun(RASTER3D_cache *, read_fn *, void *);
void *Rast3d_cache_new_read(int, int, int, read_fn *, void *);
int Rast3d_cache_lock(RASTER3D_cache *, int);
void Rast3d_cache_lock_intern(RASTER3D_cache *, int);
int Rast3d_cache_unlock(RASTER3D_cache *, int);
int Rast3d_cache_unlock_all(RASTER3D_cache *);
int Rast3d_cache_lock_all(RASTER3D_cache *);
void Rast3d_cache_autolock_on(RASTER3D_cache *);
void Rast3d_cache_autolock_off(RASTER3D_cache *);
void Rast3d_cache_set_min_unlock(RASTER3D_cache *, int);
int Rast3d_cache_remove_elt(RASTER3D_cache *, int);
int Rast3d_cache_flush(RASTER3D_cache *, int);
int Rast3d_cache_remove_all(RASTER3D_cache *);
int Rast3d_cache_flush_all(RASTER3D_cache *);
void *Rast3d_cache_elt_ptr(RASTER3D_cache *, int);
int Rast3d_cache_load(RASTER3D_cache *, int);
int Rast3d_cache_get_elt(RASTER3D_cache *, int, void *);
int Rast3d_cache_put_elt(RASTER3D_cache *, int, const void *);

/* cachehash.c */
void Rast3d_cache_hash_reset(Rast3d_cache_hash *);
void Rast3d_cache_hash_dispose(Rast3d_cache_hash *);
void *Rast3d_cache_hash_new(int);
void Rast3d_cache_hash_remove_name(Rast3d_cache_hash *, int);
void Rast3d_cache_hash_load_name(Rast3d_cache_hash *, int, int);
int Rast3d_cache_hash_name2index(Rast3d_cache_hash *, int);

/* changeprecision.c */
void Rast3d_change_precision(void *, int, const char *);

/* changetype.c */
void Rast3d_change_type(void *, const char *);

/* filecompare.c */
void Rast3d_compare_files(const char *, const char *, const char *, const char *);

/* filename.c */
void Rast3d_filename(char *, const char *, const char *, const char *);

/* fpcompress.c */
void Rast3d_fpcompress_print_binary(char *, int);
void Rast3d_fpcompress_dissect_xdr_double(unsigned char *);
int Rast3d_fpcompress_write_xdr_nums(int, char *, int, int, char *, int);
int Rast3d_fpcompress_read_xdr_nums(int, char *, int, int, int, char *, int);

/* alloc.c */
void *Rast3d_malloc(int);
void *Rast3d_realloc(void *, int);
void Rast3d_free(void *);

/* cache.c */
int Rast3d_init_cache(RASTER3D_Map *, int);
int Rast3d_dispose_cache(RASTER3D_Map *);
int Rast3d_flush_all_tiles(RASTER3D_Map *);

/* cats.c */
int Rast3d_write_cats(const char *, struct Categories *);
int Rast3d_read_cats(const char *, const char *, struct Categories *);

/* close.c */
int Rast3d_close(RASTER3D_Map *);

/* color.c */
int Rast3d_remove_color(const char *);
int Rast3d_read_colors(const char *, const char *, struct Colors *);
int Rast3d_write_colors(const char *, const char *, struct Colors *);

/* defaults.c */
void Rast3d_set_compression_mode(int, int);
void Rast3d_get_compression_mode(int *, int *);
void Rast3d_set_cache_size(int);
int Rast3d_get_cache_size(void);
void Rast3d_set_cache_limit(int);
int Rast3d_get_cache_limit(void);
void Rast3d_set_file_type(int);
int Rast3d_get_file_type(void);
void Rast3d_set_tile_dimension(int, int, int);
void Rast3d_get_tile_dimension(int *, int *, int *);
void Rast3d_set_error_fun(void (*)(const char *));
void Rast3d_init_defaults(void);

/* doubleio.c */
int Rast3d_write_doubles(int, int, const double *, int);
int Rast3d_read_doubles(int, int, double *, int);

/* error.c */
void Rast3d_skip_error(const char *);
void Rast3d_print_error(const char *);
void Rast3d_fatal_error(const char *, ...) __attribute__ ((format(printf, 1, 2)))
    __attribute__ ((noreturn));
void Rast3d_fatal_error_noargs(const char *) __attribute__ ((noreturn));
void Rast3d_error(const char *, ...) __attribute__ ((format(printf, 1, 2)));

/* fpxdr.c */
int Rast3d_is_xdr_null_num(const void *, int);
int Rast3d_is_xdr_null_float(const float *);
int Rast3d_is_xdr_null_double(const double *);
void Rast3d_set_xdr_null_num(void *, int);
void Rast3d_set_xdr_null_double(double *);
void Rast3d_set_xdr_null_float(float *);
int Rast3d_init_fp_xdr(RASTER3D_Map *, int);
int Rast3d_init_copy_to_xdr(RASTER3D_Map *, int);
int Rast3d_copy_to_xdr(const void *, int);
int Rast3d_init_copy_from_xdr(RASTER3D_Map *, int);
int Rast3d_copy_from_xdr(int, void *);

/* history.c */
int Rast3d_write_history(const char *, struct History *);
int Rast3d_read_history(const char *, const char *, struct History *);

/* intio.c */
int Rast3d_write_ints(int, int, const int *, int);
int Rast3d_read_ints(int, int, int *, int);

/* keys.c */
int Rast3d_key_get_int(struct Key_Value *, const char *, int *);
int Rast3d_key_get_double(struct Key_Value *, const char *, double *);
int Rast3d_key_get_string(struct Key_Value *, const char *, char **);
int Rast3d_key_get_value(struct Key_Value *, const char *, char *, char *, int,
		    int, int *);
int Rast3d_key_set_int(struct Key_Value *, const char *, const int *);
int Rast3d_key_set_double(struct Key_Value *, const char *, const double *);
int Rast3d_key_set_string(struct Key_Value *, const char *, char *const *);
int Rast3d_key_set_value(struct Key_Value *, const char *, const char *,
		    const char *, int, int, const int *);
/* long.c */
int Rast3d_long_encode(long *, unsigned char *, int);
void Rast3d_long_decode(unsigned char *, long *, int, int);

/* mapset.c */
void Rast3d_make_mapset_map_directory(const char *);

/* mask.c */
int Rast3d_mask_close(void);
int Rast3d_mask_file_exists(void);
int Rast3d_mask_open_old(void);
int Rast3d_mask_reopen(int);
int Rast3d_is_masked(RASTER3D_Map *, int, int, int);
void Rast3d_mask_num(RASTER3D_Map *, int, int, int, void *, int);
void Rast3d_mask_float(RASTER3D_Map *, int, int, int, float *);
void Rast3d_mask_double(RASTER3D_Map *, int, int, int, double *);
void Rast3d_mask_tile(RASTER3D_Map *, int, void *, int);
void Rast3d_mask_on(RASTER3D_Map *);
void Rast3d_mask_off(RASTER3D_Map *);
int Rast3d_mask_is_on(RASTER3D_Map *);
int Rast3d_mask_is_off(RASTER3D_Map *);
const char *Rast3d_mask_file(void);
int Rast3d_mask_map_exists(void);

/* maskfn.c */
int Rast3d_mask_d_select(DCELL *, d_Mask *);
DCELL Rast3d_mask_match_d_interval(DCELL, d_Interval *);
void Rast3d_parse_vallist(char **, d_Mask **);

/* misc.c */
int Rast3d_g3d_type2cell_type(int);
void Rast3d_copy_float2Double(const float *, int, double *, int, int);
void Rast3d_copy_double2Float(const double *, int, float *, int, int);
void Rast3d_copy_values(const void *, int, int, void *, int, int, int);
int Rast3d_length(int);
int Rast3d_extern_length(int);

/* null.c */
int Rast3d_is_null_value_num(const void *, int);
void Rast3d_set_null_value(void *, int, int);

/* open2.c */
void *Rast3d_open_new_param(const char *, int , int, RASTER3D_Region *, int, int, int, int, int, int);
/* open.c */
void *Rast3d_open_cell_old_no_header(const char *, const char *);
void *Rast3d_open_cell_old(const char *, const char *, RASTER3D_Region *, int, int);
void *Rast3d_open_cell_new(const char *, int, int, RASTER3D_Region *);
void *Rast3d_open_new_opt_tile_size(const char *, int , RASTER3D_Region * , int , int );

/* param.c */
void Rast3d_set_standard3d_input_params(void);
int Rast3d_get_standard3d_params(int *, int *, int *, int *, int *,
			    int *, int *, int *, int *, int *);
void Rast3d_set_window_params(void);
char *Rast3d_get_window_params(void);

/* range.c */
void Rast3d_range_update_from_tile(RASTER3D_Map *, const void *, int, int, int, int,
			      int, int, int, int);
int Rast3d_read_range(const char *, const char *, struct FPRange *);
int Rast3d_range_load(RASTER3D_Map *);
void Rast3d_range_min_max(RASTER3D_Map *, double *, double *);
int Rast3d_range_write(RASTER3D_Map *);
int Rast3d_range_init(RASTER3D_Map *);

/* region.c */
void Rast3d_get_region_value(RASTER3D_Map *, double, double, double, void *, int);
void Rast3d_adjust_region(RASTER3D_Region *);
void Rast3d_region_copy(RASTER3D_Region *, RASTER3D_Region *);
void Rast3d_incorporate2d_region(struct Cell_head *, RASTER3D_Region *);
void Rast3d_region_from_to_cell_head(struct Cell_head *, RASTER3D_Region *);
void Rast3d_adjust_region_res(RASTER3D_Region *);
void Rast3d_extract2d_region(RASTER3D_Region *, struct Cell_head *);
void Rast3d_region_to_cell_head(RASTER3D_Region *, struct Cell_head *);
int Rast3d_read_region_map(const char *, const char *, RASTER3D_Region *);
int Rast3d_is_valid_location(RASTER3D_Region *, double, double, double);
void Rast3d_location2coord(RASTER3D_Region *, double, double, double, int *, int *, int *);
void Rast3d_location2coord2(RASTER3D_Region *, double, double, double, int *, int *, int *);
void Rast3d_coord2location(RASTER3D_Region *, double, double, double, double *, double *, double *);
/* resample.c */
void Rast3d_nearest_neighbor(RASTER3D_Map *, int, int, int, void *, int);
void Rast3d_set_resampling_fun(RASTER3D_Map *, void (*)());
void Rast3d_get_resampling_fun(RASTER3D_Map *, void (**)());
void Rast3d_get_nearest_neighbor_fun_ptr(void (**)());

/* volume.c */
void Rast3d_get_volume_a(void *, double[2][2][2][3], int, int, int, void *, int);
void Rast3d_get_volume(void *, double, double, double, double, double, double,
		   double, double, double, double, double, double, int, int,
		   int, void *, int);
void Rast3d_get_aligned_volume(void *, double, double, double, double, double,
			  double, int, int, int, void *, int);
void Rast3d_make_aligned_volume_file(void *, const char *, double, double, double,
			       double, double, double, int, int, int);
/* window.c */
void Rast3d_get_value(RASTER3D_Map *, int, int, int, void *, int);
float Rast3d_get_float(RASTER3D_Map *, int, int, int);
double Rast3d_get_double(RASTER3D_Map *, int, int, int);
void Rast3d_get_window_value(RASTER3D_Map *, double, double, double, void *, int);


RASTER3D_Region *Rast3d_window_ptr(void);
void Rast3d_set_window(RASTER3D_Region *);
void Rast3d_set_window_map(RASTER3D_Map *, RASTER3D_Region *);
void Rast3d_get_window(RASTER3D_Region *);

/* windowio.c */
void Rast3d_use_window_params(void);
int Rast3d_read_window(RASTER3D_Region *, const char *);

/* int Rast3d_writeWindow (RASTER3D_Region *, char *); */
/* getblock.c */
void Rast3d_get_block_nocache(RASTER3D_Map *, int, int, int, int, int, int, void *,
			 int);
void Rast3d_get_block(RASTER3D_Map *, int, int, int, int, int, int, void *, int);

/* header.c */
int Rast3d_read_header(RASTER3D_Map *, int *, int *, double *, double *, double *,
		   double *, double *, double *, int *, int *, int *,
		   double *, double *, double *, int *, int *, int *, int *,
		   int *, int *, int *, int *, int *, int *, int *, char **, int *, int *);
int Rast3d_write_header(RASTER3D_Map *, int, int, double, double, double, double,
		    double, double, int, int, int, double, double, double,
		    int, int, int, int, int, int, int, int, int, int, int,
		    char *, int, int);
int Rast3d_rewrite_header(RASTER3D_Map * map);
int Rast3d_cache_size_encode(int, int);
int Rast3d__compute_cache_size(RASTER3D_Map *, int);
int Rast3d_fill_header(RASTER3D_Map *, int, int, int, int, int, int, int, int, int,
		   int, int, int, int, int, int, int, double, double, double,
		   double, double, double, int, int, int, double, double,
		   double, char *, int, int);
/* headerinfo.c */
void Rast3d_get_coords_map(RASTER3D_Map *, int *, int *, int *);
void Rast3d_get_coords_map_window(RASTER3D_Map *, int *, int *, int *);
void Rast3d_get_nof_tiles_map(RASTER3D_Map *, int *, int *, int *);
void Rast3d_get_region_map(RASTER3D_Map *, double *, double *, double *, double *,
		      double *, double *);
void Rast3d_get_window_map(RASTER3D_Map *, double *, double *, double *, double *,
		      double *, double *);
void Rast3d_get_tile_dimensions_map(RASTER3D_Map *, int *, int *, int *);
int Rast3d_tile_type_map(RASTER3D_Map *);
int Rast3d_file_type_map(RASTER3D_Map *);
int Rast3d_tile_precision_map(RASTER3D_Map *);
int Rast3d_tile_use_cache_map(RASTER3D_Map *);
void Rast3d_print_header(RASTER3D_Map *);
void Rast3d_get_region_struct_map(RASTER3D_Map *, RASTER3D_Region *);
const char* Rast3d_get_unit(RASTER3D_Map * map);
int Rast3d_get_vertical_unit2(RASTER3D_Map * map);
const char* Rast3d_get_vertical_unit(RASTER3D_Map * map);
void Rast3d_set_unit(RASTER3D_Map * map, const char *);
void Rast3d_set_vertical_unit(RASTER3D_Map * map, const char *);
void Rast3d_set_vertical_unit2(RASTER3D_Map * map, int);

/* index.c */
int Rast3d_flush_index(RASTER3D_Map *);
int Rast3d_init_index(RASTER3D_Map *, int);

/* retile.c */
void Rast3d_retile(void *, const char *, int, int, int);

/* rle.c */
int Rast3d_rle_count_only(char *, int, int);
void Rast3d_rle_encode(char *, char *, int, int);
void Rast3d_rle_decode(char *, char *, int, int, int *, int *);

/* tilealloc.c */
void *Rast3d_alloc_tiles_type(RASTER3D_Map *, int, int);
void *Rast3d_alloc_tiles(RASTER3D_Map *, int);
void Rast3d_free_tiles(void *);

/* tileio.c */
void *Rast3d_get_tile_ptr(RASTER3D_Map *, int);
int Rast3d_tile_load(RASTER3D_Map *, int);
int Rast3d__remove_tile(RASTER3D_Map *, int);
float Rast3d_get_float_region(RASTER3D_Map *, int, int, int);
double Rast3d_get_double_region(RASTER3D_Map *, int, int, int);
void Rast3d_get_value_region(RASTER3D_Map *, int, int, int, void *, int);

/* tilemath.c */
void Rast3d_compute_optimal_tile_dimension(RASTER3D_Region *, int, int *, int *, int *, int);
void Rast3d_tile_index2tile(RASTER3D_Map *, int, int *, int *, int *);
int Rast3d_tile2tile_index(RASTER3D_Map *, int, int, int);
void Rast3d_tile_coord_origin(RASTER3D_Map *, int, int, int, int *, int *, int *);
void Rast3d_tile_index_origin(RASTER3D_Map *, int, int *, int *, int *);
void Rast3d_coord2tile_coord(RASTER3D_Map *, int, int, int, int *, int *, int *, int *,
			 int *, int *);
void Rast3d_coord2tile_index(RASTER3D_Map *, int, int, int, int *, int *);
int Rast3d_coord_in_range(RASTER3D_Map *, int, int, int);
int Rast3d_tile_index_in_range(RASTER3D_Map *, int);
int Rast3d_tile_in_range(RASTER3D_Map *, int, int, int);
int Rast3d_compute_clipped_tile_dimensions(RASTER3D_Map *, int, int *, int *, int *,
				     int *, int *, int *);

/* tilenull.c */
void Rast3d_set_null_tile_type(RASTER3D_Map *, void *, int);
void Rast3d_set_null_tile(RASTER3D_Map *, void *);

/* tileread.c */
int Rast3d_read_tile(RASTER3D_Map *, int, void *, int);
int Rast3d_read_tile_float(RASTER3D_Map *, int, void *);
int Rast3d_read_tile_double(RASTER3D_Map *, int, void *);
int Rast3d_lock_tile(RASTER3D_Map *, int);
int Rast3d_unlock_tile(RASTER3D_Map *, int);
int Rast3d_unlock_all(RASTER3D_Map *);
void Rast3d_autolock_on(RASTER3D_Map *);
void Rast3d_autolock_off(RASTER3D_Map *);
void Rast3d_min_unlocked(RASTER3D_Map *, int);
int Rast3d_begin_cycle(RASTER3D_Map *);
int Rast3d_end_cycle(RASTER3D_Map *);

/* tilewrite.c */
int Rast3d_write_tile(RASTER3D_Map *, int, const void *, int);
int Rast3d_write_tile_float(RASTER3D_Map *, int, const void *);
int Rast3d_write_tile_double(RASTER3D_Map *, int, const void *);
int Rast3d_flush_tile(RASTER3D_Map *, int);
int Rast3d_flush_tile_cube(RASTER3D_Map *, int, int, int, int, int, int);
int Rast3d_flush_tiles_in_cube(RASTER3D_Map *, int, int, int, int, int, int);
int Rast3d_put_float(RASTER3D_Map *, int, int, int, float);
int Rast3d_put_double(RASTER3D_Map *, int, int, int, double);
int Rast3d_put_value(RASTER3D_Map *, int, int, int, const void *, int);

/* writeascii.c */
void Rast3d_write_ascii(void *, const char *);

#endif /* RASTER3DDEFS */
