void *dig_alloc_space (int, int *, int, void *, int);	/* exits on error, calls _alloc_space () */
void *dig__alloc_space (int, int *, int, void *, int);	/* returns NULL on error, calls calloc(), _frealloc() */
void *dig_falloc (int, int);	/* exits on error, calls _falloc () */
void *dig_frealloc (void *, int, int, int);	/* exits on error, calls _frealloc () */
void *dig__falloc (int, int);	/* returns NULL on error, calls calloc () */
void *dig__frealloc (void *, int, int, int);	/* returns NULL on error, calls calloc () */

int dig_init_list ( struct ilist * );
int dig_list_add ( struct ilist *, int );

char *color_name (int);		/* pass it an int, returns the name of the color */

float dig_calc_begin_angle (struct line_pnts *, double);
float dig_calc_end_angle (struct line_pnts *, double);
int dig_line_degenerate ( struct line_pnts *);
char *dig_float_point (char *, int, double);
/* double dig_point_in_area (struct Map_info *, double, double, P_AREA *); */
double dig_x_intersect (double, double, double, double, double);
double dig_distance2_point_to_line (double, double, double, double, double, double, double, double, double, 
	                            int, double *, double *, double *, double *, int *);

double dig_unit_conversion (void);

/* portable data routines  -  only to be called by library routines! */
double *dig__double_convert (double *, double *, int, struct dig_head *);
float *dig__float_convert (float *, float *, int, struct dig_head *);
short *dig__short_convert (short *in, short *out, int, struct dig_head *);
long *dig__long_convert (long *, long *, int, struct dig_head *);
long *dig__int_convert (int *, long *, int, struct dig_head *);
long *dig__plus_t_convert (plus_t *, long *, int, struct dig_head *);
int *dig__long_convert_to_int (long *, int *, int, struct dig_head *);
plus_t *dig__long_convert_to_plus_t (long *, plus_t *, int, struct dig_head *);
char *dig__convert_buffer (int);

plus_t **dig_get_cont_lines (struct Map_info *, plus_t, double, int);
plus_t dig_get_next_cont_line (struct Map_info *, plus_t, double, int);

struct dig_head *dig_get_head (void);
struct dig_head *dig__get_head (void);
void dig_init_portable (struct Port_info *, int);
int dig__byte_order_out ();

/* int dig__set_cur_head (struct dig_head *); */
int dig_set_cur_port (struct Port_info *);
int dig__write_head (struct Map_info *);
int dig__read_head (struct Map_info *);

int dig__fread_port_D (double *, int, GVFILE *);
int dig__fread_port_F (float *, int, GVFILE *);
int dig__fread_port_L (long *, int, GVFILE *);
int dig__fread_port_S (short *, int, GVFILE *);
int dig__fread_port_I (int *, int, GVFILE *);
int dig__fread_port_P (plus_t *, int, GVFILE *);
int dig__fread_port_C (char *, int, GVFILE *);
int dig__fwrite_port_D (double *, int, GVFILE *);
int dig__fwrite_port_F (float *, int, GVFILE *);
int dig__fwrite_port_L (long *, int, GVFILE *);
int dig__fwrite_port_S (short *, int, GVFILE *);
int dig__fwrite_port_I (int *, int, GVFILE *);
int dig__fwrite_port_P (plus_t *, int, GVFILE *);
int dig__fwrite_port_C (char *, int, GVFILE *);


/******************************************************************************/
int dig_build_area_with_line (struct Plus_head *, plus_t, int, plus_t ** );
plus_t dig_line_get_area (struct Plus_head *, plus_t, int);
int dig_line_set_area (struct Plus_head *, plus_t, int, plus_t);
int dig_add_area (struct Plus_head *, int, plus_t *);
int dig_area_add_isle (struct Plus_head *, int, int);
int dig_area_del_isle (struct Plus_head *, int, int);
int dig_del_area (struct Plus_head *, int );
int dig_angle_next_line (struct Plus_head *, plus_t, int, int);
/* int dig_area_bound_box (struct Map_info *, P_AREA *); */
int dig_bound_box2 (struct line_pnts *, double *, double *, double *, double *, long);
int dig_box_copy (BOUND_BOX *, BOUND_BOX *);
int dig_box_extend (BOUND_BOX *, BOUND_BOX *);
int dig_line_box (struct line_pnts *, BOUND_BOX *);
int dig_line_set_box (struct Plus_head *, plus_t, BOUND_BOX *);
int dig_line_get_box (struct Plus_head *, plus_t, BOUND_BOX *);
int dig_area_set_box (struct Plus_head *, plus_t, BOUND_BOX *);
int dig_area_get_box (struct Plus_head *, plus_t, BOUND_BOX *);
int dig_isle_set_box (struct Plus_head *, plus_t, BOUND_BOX *);
int dig_isle_get_box (struct Plus_head *, plus_t, BOUND_BOX *);

int dig_is_line_degenerate (struct line_pnts *, double);
/* int dig_check_nodes (struct Map_info *, struct new_node *, struct line_pnts *);
int dig_in_area_bbox (P_AREA *, double, double); */
int dig_start_clock (long *);
int dig_stop_clock (long *);
char *dig_stop_clock_str (long *);
int dig_write_file_checks (GVFILE *, struct Plus_head *);
int dig_do_file_checks (struct Map_info *, char *, char *);
/* int dig_find_area (struct Map_info *, P_AREA *, double *, double *, double *, double);
int dig_find_area2 (struct Map_info *, P_AREA *, double *); */
int dig_find_area_poly (struct line_pnts *, double *);
int dig_get_poly_points ( int, struct line_pnts **, int *, struct line_pnts *);
int dig_add_isle (struct Plus_head *, int, plus_t *);
int dig_del_isle (struct Plus_head *, int);
int dig_set_distance_to_line_tolerance (double);
int dig_test_for_intersection (double, double, double, double, double, double, double, double);
int dig_find_intersection (double, double, double, double, double, double, double, double, double *, double *);

int dig_init_plus (struct Plus_head *);
void dig_free_plus_nodes (struct Plus_head *);
void dig_free_plus_lines (struct Plus_head *);
void dig_free_plus_areas (struct Plus_head *);
void dig_free_plus_isles (struct Plus_head *);
void dig_free_plus (struct Plus_head *);
int dig_load_plus (struct Plus_head *, GVFILE *, int);
int dig_map_to_head (struct Map_info *, struct Plus_head *);
int dig_head_to_map (struct Plus_head *, struct Map_info *);
int dig_spindex_init ( struct Plus_head *);

/* int dig_snap_line_to_node (struct Map_info *, int, int, struct line_pnts *); */

int dig_add_node (struct Plus_head *, double, double, double );
int dig_which_node (struct Plus_head *, double, double, double);
int dig_add_line (struct Plus_head *plus, int type, struct line_pnts *points, long offset);
int dig_del_line (struct Plus_head *, int );
int dig_node_add_line (struct Plus_head *, int, int, struct line_pnts *, int);
float dig_node_line_angle ( struct Plus_head *, int , int );
int dig_node_angle_check (struct Plus_head *, int, int );
/* int dig_node_del_line (struct Plus_head *plus, int node, int line);
int dig_add_line_to_node (int, int, char, struct Map_info *, struct line_pnts *); */
int dig_point_to_area (struct Map_info *, double, double);
int dig_point_to_next_area (struct Map_info *, double, double, double *);
int dig_point_to_line (struct Map_info *, double, double, char);

/* list of updated */
void dig_line_reset_updated ( struct Plus_head *Plus );
void dig_line_add_updated ( struct Plus_head *Plus, int line );
void dig_node_reset_updated ( struct Plus_head *Plus );
void dig_node_add_updated ( struct Plus_head *Plus, int node );

/* conversion of types */
int dig_type_to_store (int);
int dig_type_from_store (int);

/* spatial index */

int dig_spidx_add_node ( struct Plus_head *, int, double, double, double);
int dig_spidx_add_line ( struct Plus_head *, int, BOUND_BOX *);
int dig_spidx_add_area ( struct Plus_head *, int, BOUND_BOX *);
int dig_spidx_add_isle ( struct Plus_head *, int, BOUND_BOX *);

int dig_spidx_del_node ( struct Plus_head *, int);
int dig_spidx_del_line ( struct Plus_head *, int);
int dig_spidx_del_area ( struct Plus_head *, int);
int dig_spidx_del_isle ( struct Plus_head *, int);

int dig_select_nodes ( struct Plus_head *, BOUND_BOX *, struct ilist *);
int dig_select_lines ( struct Plus_head *, BOUND_BOX *, struct ilist *);
int dig_select_areas ( struct Plus_head *, BOUND_BOX *, struct ilist *);
int dig_select_isles ( struct Plus_head *, BOUND_BOX *, struct ilist *);
int dig_find_node ( struct Plus_head *, double, double, double);

int dig_spidx_init (struct Plus_head *);
void dig_spidx_free_nodes (struct Plus_head *);
void dig_spidx_free_lines (struct Plus_head *);
void dig_spidx_free_areas (struct Plus_head *);
void dig_spidx_free_isles (struct Plus_head *);
void dig_spidx_free (struct Plus_head *);
int dig_write_spidx (GVFILE *, struct Plus_head *);
int dig_dump_spidx (FILE *, struct Plus_head *);
int dig_read_spidx (GVFILE *, struct Plus_head *);

/* category index */
int dig_cidx_init ( struct Plus_head *);
void dig_cidx_free ( struct Plus_head *);
int dig_cidx_add_cat ( struct Plus_head *, int, int, int, int);
int dig_cidx_add_cat_sorted ( struct Plus_head *, int, int, int, int);
int dig_cidx_del_cat ( struct Plus_head *, int, int, int, int);
void dig_cidx_sort ( struct Plus_head *);

int dig_write_cidx_head ( GVFILE *, struct Plus_head *);
int dig_read_cidx_head ( GVFILE *, struct Plus_head *);
int dig_write_cidx ( GVFILE *, struct Plus_head *);
int dig_read_cidx ( GVFILE *, struct Plus_head *, int);

/* int dig_in_line_bbox (P_LINE *, double, double); */
int dig_check_dist (struct Map_info *, int, double, double, double *);
int dig__check_dist (struct Map_info *, struct line_pnts *, double, double, double *);
/* int dig_center_check (P_LINE *, int, int, double, double); */
int dig_point_by_line (struct Map_info *, double, double, double, double, char);
/* int dig_by_line_bbox (P_LINE *, double, double, double, double); */
int dig_prune (struct line_pnts *, double);
int dig_write_head_ascii (FILE *, struct dig_head *);
int dig_read_head_ascii (FILE *, struct dig_head *);
int dig_write_frmt_ascii (FILE *, struct Format_info *, int);
int dig_read_frmt_ascii (FILE *, struct Format_info *);
int dig_node_alloc_line (P_NODE *, int add);
int dig_alloc_nodes (struct Plus_head *, int);
int dig_alloc_lines (struct Plus_head *, int);
int dig_alloc_areas (struct Plus_head *, int);
int dig_alloc_isles (struct Plus_head *, int);
P_NODE * dig_alloc_node ();
P_LINE * dig_alloc_line ();
P_AREA * dig_alloc_area ();
P_ISLE * dig_alloc_isle ();
int dig_alloc_points (struct line_pnts *, int);
int dig_alloc_cats (struct line_cats *, int);
int dig_area_alloc_line (P_AREA *, int);
int dig_area_alloc_isle (P_AREA *, int);
int dig_isle_alloc_line (P_ISLE *, int);
int dig_out_of_memory (void);
int dig_struct_copy (void *, void *, int);
int dig_rmcr (char *);
int dig_write_plus_file (GVFILE *, struct Plus_head *);
int dig_write_nodes (GVFILE *, struct Plus_head *);
int dig_write_lines (GVFILE *, struct Plus_head *);
int dig_write_areas (GVFILE *, struct Plus_head *);
int dig_write_isles (GVFILE *, struct Plus_head *);

int dig_Rd_P_node (struct Plus_head *, int i, GVFILE *);
int dig_Wr_P_node (struct Plus_head *, int i, GVFILE *);
int dig_Rd_P_line (struct Plus_head *, int i, GVFILE *);
int dig_Wr_P_line (struct Plus_head *, int i, GVFILE *);
int dig_Rd_P_area (struct Plus_head *, int i, GVFILE *);
int dig_Wr_P_area (struct Plus_head *, int i, GVFILE *);
int dig_Rd_P_isle (struct Plus_head *, int i, GVFILE *);
int dig_Wr_P_isle (struct Plus_head *, int i, GVFILE *);
int dig_Rd_Plus_head (GVFILE *, struct Plus_head *);
int dig_Wr_Plus_head (GVFILE *, struct Plus_head *);

/* file loaded to memory */
long dig_ftell ( GVFILE *file );
int dig_fseek ( GVFILE *file, long offset, int whence );
void dig_rewind ( GVFILE *file );
int dig_fflush ( GVFILE *file );
size_t dig_fread ( void *ptr, size_t size, size_t nmemb, GVFILE *file );
size_t dig_fwrite ( void *ptr, size_t size, size_t nmemb, GVFILE *file );
void dig_file_init ( GVFILE *file );
int dig_file_load ( GVFILE *file );
void dig_file_free ( GVFILE *file );
