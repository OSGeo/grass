#ifndef GRASS_VECTORDEFS_H
#define GRASS_VECTORDEFS_H

/*
 * "Public" functions, for use in modules
 */
/* Points (line) */
struct line_pnts *Vect_new_line_struct(void);
int Vect_append_point(struct line_pnts *, double, double, double);
int Vect_append_points(struct line_pnts *, const struct line_pnts *, int);
int Vect_line_insert_point(struct line_pnts *, int, double, double, double);
int Vect_line_delete_point(struct line_pnts *, int);
int Vect_line_get_point(const struct line_pnts *, int ,
                        double *, double *, double *);
int Vect_get_num_line_points(const struct line_pnts *);
int Vect_line_prune(struct line_pnts *);
int Vect_line_prune_thresh(struct line_pnts *, double);
void Vect_line_reverse(struct line_pnts *);
int Vect_copy_xyz_to_pnts(struct line_pnts *, const double *, const double *, const double *,
                          int);
int Vect_copy_pnts_to_xyz(const struct line_pnts *, double *, double *, double *,
                          int *);
void Vect_reset_line(struct line_pnts *);
void Vect_destroy_line_struct(struct line_pnts *);
int Vect_point_on_line(const struct line_pnts *, double, double *, double *,
                       double *, double *, double *);
int Vect_line_segment(const struct line_pnts *, double, double, struct line_pnts *);
double Vect_line_length(const struct line_pnts *);
double Vect_line_geodesic_length(const struct line_pnts *);
int Vect_line_distance(const struct line_pnts *, double, double, double, int,
                       double *, double *, double *, double *, double *,
                       double *);
int Vect_line_geodesic_distance(const struct line_pnts *, double, double, double, int,
                       double *, double *, double *, double *, double *,
                       double *);
void Vect_line_box(const struct line_pnts *, struct bound_box *);
void Vect_line_parallel(struct line_pnts *, double, double, int,
                        struct line_pnts *);
void Vect_line_parallel2(struct line_pnts *, double, double,
                         double, int, int, double,
                         struct line_pnts *);
void Vect_line_buffer(const struct line_pnts *, double, double, struct line_pnts *);
void Vect_line_buffer2(const struct line_pnts *, double, double,
                       double, int, int, double,
                       struct line_pnts **,
                       struct line_pnts ***, int *);
void Vect_area_buffer2(const struct Map_info *, int, double, double,
                       double, int, int, double,
                       struct line_pnts **,
                       struct line_pnts ***, int *);
void Vect_point_buffer2(double, double, double, double,
                        double, int, double,
                        struct line_pnts **);


/* Categories */
struct line_cats *Vect_new_cats_struct(void);
int Vect_cat_set(struct line_cats *, int, int);
int Vect_cat_get(const struct line_cats *, int, int *);
int Vect_cat_del(struct line_cats *, int);
int Vect_field_cat_del(struct line_cats *, int, int);
int Vect_field_cat_get(const struct line_cats *, int, struct ilist *);
int Vect_cat_in_array(int, const int *, int);
int Vect_reset_cats(struct line_cats *);
void Vect_destroy_cats_struct(struct line_cats *);
int Vect_get_area_cats(const struct Map_info *, int, struct line_cats *);
int Vect_get_area_cat(const struct Map_info *, int, int);
int Vect_get_line_cat(const struct Map_info *, int, int);
struct cat_list *Vect_cats_set_constraint(struct Map_info *, int, char *, char *);
int Vect_cats_in_constraint(struct line_cats *, int, struct cat_list *);

/* List of categories */
struct cat_list *Vect_new_cat_list(void);
int Vect_str_to_cat_list(const char *, struct cat_list *);
int Vect_array_to_cat_list(const int *, int, struct cat_list *);
int Vect_cat_in_cat_list(int, const struct cat_list *);
void Vect_destroy_cat_list(struct cat_list *);

/* Vector array */
struct varray *Vect_new_varray(int);
int Vect_set_varray_from_cat_string(const struct Map_info *, int, const char *, int,
                                    int, struct varray *);
int Vect_set_varray_from_cat_list(const struct Map_info *, int, struct cat_list *,
                                  int, int, struct varray *);
int Vect_set_varray_from_db(const struct Map_info *, int, const char *, int, int,
                            struct varray *);

/* DB connection - field info */
struct dblinks *Vect_new_dblinks_struct(void);
void Vect_reset_dblinks(struct dblinks *);
int Vect_add_dblink(struct dblinks *, int, const char *,
                    const char *, const char *, const char *, const char *);
int Vect_check_dblink(const struct dblinks *, int, const char *);
int Vect_map_add_dblink(struct Map_info *, int, const char *,
                        const char *, const char *, const char *,
                        const char *);
int Vect_map_del_dblink(struct Map_info *, int);
void Vect_copy_map_dblinks(const struct Map_info *, struct Map_info *, int);
int Vect_map_check_dblink(const struct Map_info *, int, const char *);
int Vect_read_dblinks(struct Map_info *);
int Vect_write_dblinks(struct Map_info *);
struct field_info *Vect_default_field_info(struct Map_info *, int,
                                           const char *, int);
struct field_info *Vect_get_dblink(const struct Map_info *, int);
struct field_info *Vect_get_field(const struct Map_info *, int);
struct field_info *Vect_get_field_by_name(const struct Map_info *, const char *);
struct field_info *Vect_get_field2(const struct Map_info *, const char *);
int Vect_get_field_number(const struct Map_info *, const char *);
void Vect_set_db_updated(struct Map_info *);
const char *Vect_get_column_names(const struct Map_info *, int);
const char *Vect_get_column_types(const struct Map_info *, int);
const char *Vect_get_column_names_types(const struct Map_info *, int);

/* List of FID (feature ID) (integers) */
struct ilist *Vect_new_list(void);
int Vect_list_append(struct ilist *, int);
int Vect_list_append_list(struct ilist *, const struct ilist *);
int Vect_list_delete(struct ilist *, int);
int Vect_list_delete_list(struct ilist *, const struct ilist *);
int Vect_val_in_list(const struct ilist *, int);
int Vect_reset_list(struct ilist *);
void Vect_destroy_list(struct ilist *);

/* List of bounding boxes with ids */
struct boxlist *Vect_new_boxlist(int);
int Vect_boxlist_append(struct boxlist *, int, const struct bound_box *);
int Vect_boxlist_append_boxlist(struct boxlist *, const struct boxlist *);
int Vect_boxlist_delete(struct boxlist *, int);
int Vect_boxlist_delete_boxlist(struct boxlist *, const struct boxlist *);
int Vect_val_in_boxlist(const struct boxlist *, int);
int Vect_reset_boxlist(struct boxlist *);
void Vect_destroy_boxlist(struct boxlist *);

/* Bounding box (MBR) */
int Vect_point_in_box(double, double, double, const struct bound_box *);
int Vect_box_overlap(const struct bound_box *, const struct bound_box *);
int Vect_box_copy(struct bound_box *, const struct bound_box *);
int Vect_box_extend(struct bound_box *, const struct bound_box *);
int Vect_box_clip(double *, double *, double *, double *, const struct bound_box *);
int Vect_region_box(const struct Cell_head *, struct bound_box *);

/* Category index */
int Vect_cidx_get_num_fields(const struct Map_info *);
int Vect_cidx_get_field_number(const struct Map_info *, int);
int Vect_cidx_get_field_index(const struct Map_info *, int);
int Vect_cidx_get_num_unique_cats_by_index(const struct Map_info *, int);
int Vect_cidx_get_num_cats_by_index(const struct Map_info *, int);
int Vect_cidx_get_num_types_by_index(const struct Map_info *, int);
int Vect_cidx_get_type_count_by_index(const struct Map_info *, int, int, int *,
                                      int *);
int Vect_cidx_get_type_count(const struct Map_info *, int, int);
int Vect_cidx_get_cat_by_index(const struct Map_info *, int, int, int *, int *,
                               int *);
int Vect_cidx_find_next(const struct Map_info *, int, int, int, int, int *, int *);
void Vect_cidx_find_all(const struct Map_info *, int, int, int, struct ilist *);
int Vect_cidx_dump(const struct Map_info *, FILE *);
int Vect_cidx_save(struct Map_info *);
int Vect_cidx_open(struct Map_info *, int);


/* Set/get map header info */
int Vect_read_header(struct Map_info *);
int Vect_write_header(const struct Map_info *);
const char *Vect_get_name(const struct Map_info *);
const char *Vect_get_mapset(const struct Map_info *);
const char *Vect_get_full_name(const struct Map_info *);
const char *Vect_get_finfo_dsn_name(const struct Map_info *);
char *Vect_get_finfo_layer_name(const struct Map_info *);
const char *Vect_get_finfo_format_info(const struct Map_info *);
const char *Vect_get_finfo_geometry_type(const struct Map_info *);
const struct Format_info *Vect_get_finfo(const struct Map_info *);
int Vect_is_3d(const struct Map_info *);
int Vect_set_organization(struct Map_info *, const char *);
const char *Vect_get_organization(const struct Map_info *);
int Vect_set_date(struct Map_info *, const char *);
const char *Vect_get_date(const struct Map_info *);
int Vect_set_person(struct Map_info *, const char *);
const char *Vect_get_person(const struct Map_info *);
int Vect_set_map_name(struct Map_info *, const char *);
const char *Vect_get_map_name(const struct Map_info *);
int Vect_set_map_date(struct Map_info *, const char *);
const char *Vect_get_map_date(const struct Map_info *);
int Vect_set_comment(struct Map_info *, const char *);
const char *Vect_get_comment(const struct Map_info *);
int Vect_set_scale(struct Map_info *, int);
int Vect_get_scale(const struct Map_info *);
int Vect_set_zone(struct Map_info *, int);
int Vect_get_zone(const struct Map_info *);
int Vect_get_proj(const struct Map_info *);
int Vect_set_proj(struct Map_info *, int);
const char *Vect_get_proj_name(const struct Map_info *);
int Vect_set_thresh(struct Map_info *, double);
double Vect_get_thresh(const struct Map_info *);
int Vect_get_constraint_box(const struct Map_info *, struct bound_box *);


/* Get map level 2 information */
int Vect_level(const struct Map_info *);
int Vect_get_line_type(const struct Map_info *, int);
plus_t Vect_get_num_nodes(const struct Map_info *);
plus_t Vect_get_num_primitives(const struct Map_info *, int);
plus_t Vect_get_num_lines(const struct Map_info *);
plus_t Vect_get_num_areas(const struct Map_info *);
plus_t Vect_get_num_faces(const struct Map_info *);
plus_t Vect_get_num_kernels(const struct Map_info *);
plus_t Vect_get_num_volumes(const struct Map_info *);
plus_t Vect_get_num_islands(const struct Map_info *);
plus_t Vect_get_num_holes(const struct Map_info *);
int Vect_get_line_box(const struct Map_info *, int, struct bound_box *);
int Vect_get_area_box(const struct Map_info *, int, struct bound_box *);
int Vect_get_isle_box(const struct Map_info *, int, struct bound_box *);
int Vect_get_map_box(const struct Map_info *, struct bound_box *);
int V__map_overlap(struct Map_info *, double, double, double, double);
void Vect_set_release_support(struct Map_info *);
void Vect_set_category_index_update(struct Map_info *);

/* Open/close/rewind/set_constraints for map */
int Vect_check_input_output_name(const char *, const char *, int);
int Vect_legal_filename(const char *);
int Vect_set_open_level(int);
int Vect_open_old(struct Map_info *, const char *, const char *);
int Vect_open_old2(struct Map_info *, const char *, const char *, const char *);
int Vect_open_old_head(struct Map_info *, const char *, const char *);
int Vect_open_old_head2(struct Map_info *, const char *, const char *, const char *);
int Vect_open_new(struct Map_info *, const char *, int);
int Vect_open_tmp_new(struct Map_info *, const char *, int);
int Vect_open_update(struct Map_info *, const char *, const char *);
int Vect_open_update2(struct Map_info *, const char *, const char *, const char *);
int Vect_open_update_head(struct Map_info *, const char *, const char *);
int Vect_copy_head_data(const struct Map_info *, struct Map_info *);
int Vect_build(struct Map_info *);
int Vect_topo_check(struct Map_info *, struct Map_info *);
int Vect_get_built(const struct Map_info *);
int Vect_build_partial(struct Map_info *, int);
int Vect_set_constraint_region(struct Map_info *, double, double, double,
                                double, double, double);
int Vect_set_constraint_type(struct Map_info *, int);
int Vect_set_constraint_field(struct Map_info *, int);
void  Vect_remove_constraints(struct Map_info *);
int Vect_rewind(struct Map_info *);
int Vect_close(struct Map_info *);
void Vect_set_error_handler_io(struct Map_info *, struct Map_info *);

/* Read/write lines, nodes, areas */
/* Level 1 and 2 */
int Vect_get_next_line_id(const struct Map_info *);
int Vect_read_next_line(const struct Map_info *, struct line_pnts *,
                        struct line_cats *);
off_t Vect_write_line(struct Map_info *, int, const struct line_pnts *,
                      const struct line_cats *);

int Vect_get_num_dblinks(const struct Map_info *);

/* Level 2 only */
int Vect_read_line(const struct Map_info *, struct line_pnts *, struct line_cats *,
                   int);
off_t Vect_rewrite_line(struct Map_info *, int, int, const struct line_pnts *,
                      const struct line_cats *);
int Vect_delete_line(struct Map_info *, int);
int Vect_restore_line(struct Map_info *, int, off_t);

int Vect_line_alive(const struct Map_info *, int);
int Vect_node_alive(const struct Map_info *, int);
int Vect_area_alive(const struct Map_info *, int);
int Vect_isle_alive(const struct Map_info *, int);
int Vect_get_line_nodes(const struct Map_info *, int, int *, int *);
int Vect_get_line_areas(const struct Map_info *, int, int *, int *);
off_t Vect_get_line_offset(const struct Map_info *, int);

int Vect_get_node_coor(const struct Map_info *, int, double *, double *, double *);
int Vect_get_node_n_lines(const struct Map_info *, int);
int Vect_get_node_line(const struct Map_info *, int, int);
float Vect_get_node_line_angle(const struct Map_info *, int, int);

double Vect_area_perimeter(const struct line_pnts *);
int Vect_get_area_points(const struct Map_info *, int, struct line_pnts *);
int Vect_get_area_centroid(const struct Map_info *, int);
int Vect_get_area_num_isles(const struct Map_info *, int);
int Vect_get_area_isle(const struct Map_info *, int, int);
double Vect_get_area_area(const struct Map_info *, int);
int Vect_get_area_boundaries(const struct Map_info *, int, struct ilist *);

int Vect_get_isle_points(const struct Map_info *, int, struct line_pnts *);
int Vect_get_isle_area(const struct Map_info *, int);
int Vect_get_isle_boundaries(const struct Map_info *, int, struct ilist *);

int Vect_get_centroid_area(const struct Map_info *, int);

/* Level 2 update only */
int Vect_get_num_updated_lines(const struct Map_info *);
int Vect_get_updated_line(const struct Map_info *, int);
off_t Vect_get_updated_line_offset(const struct Map_info *, int);
int Vect_get_num_updated_nodes(const struct Map_info *);
int Vect_get_updated_node(const struct Map_info *, int);
void Vect_set_updated(struct Map_info *, int);
void Vect_reset_updated(struct Map_info *);

/* History */
int Vect_hist_command(struct Map_info *);
int Vect_hist_write(struct Map_info *, const char *);
int Vect_hist_copy(const struct Map_info *, struct Map_info *);
void Vect_hist_rewind(struct Map_info *);
char *Vect_hist_read(char *, int, const struct Map_info *);

/* Selecting features */
int Vect_select_lines_by_box(struct Map_info *, const struct bound_box *,
                         int, struct boxlist *);
int Vect_select_areas_by_box(struct Map_info *, const struct bound_box *,
                             struct boxlist *);
int Vect_select_isles_by_box(struct Map_info *, const struct bound_box *,
			     struct boxlist *);
int Vect_select_nodes_by_box(struct Map_info *, const struct bound_box *,
                             struct ilist *);
int Vect_find_node(struct Map_info *, double, double, double, double, int);
int Vect_find_line(struct Map_info *, double, double, double, int, double,
                   int, int);
int Vect_find_line_list(struct Map_info *, double, double, double, int,
                        double, int, const struct ilist *, struct ilist *);
int Vect_find_area(struct Map_info *, double, double);
int Vect_find_island(struct Map_info *, double, double);
int Vect_select_lines_by_polygon(struct Map_info *, struct line_pnts *, int,
                                 struct line_pnts **, int, struct ilist *);
int Vect_select_areas_by_polygon(struct Map_info *, struct line_pnts *, int,
                                 struct line_pnts **, struct ilist *);

/* Analysis */
int Vect_tin_get_z(struct Map_info *, double, double, double *, double *,
                   double *);

/* int Vect_point_in_islands (struct Map_info *, int, double, double); */
int Vect_find_poly_centroid(const struct line_pnts *, double *, double *);
int Vect__intersect_line_with_poly(const struct line_pnts *, double,
                                   struct line_pnts *);
int Vect_get_point_in_area(const struct Map_info *, int, double *, double *);
int Vect_get_point_in_poly(const struct line_pnts *, double *, double *);
int Vect_get_point_in_poly_isl(const struct line_pnts *, const struct line_pnts **, int,
                               double *, double *);
int Vect_point_in_area(double, double, const struct Map_info *, int, struct bound_box *);
int Vect_point_in_area_outer_ring(double, double, const struct Map_info *, int, struct bound_box *);
int Vect_point_in_island(double, double, const struct Map_info *, int, struct bound_box *);
int Vect_point_in_poly(double, double, const struct line_pnts *);

/* Cleaning */
void Vect_break_lines(struct Map_info *, int, struct Map_info *);
int Vect_break_lines_list(struct Map_info *, struct ilist *, struct ilist *,
                          int, struct Map_info *);
int Vect_check_line_breaks(struct Map_info *, int, struct Map_info *);
int Vect_check_line_breaks_list(struct Map_info *, struct ilist *, struct ilist *,
                          int, struct Map_info *);
int Vect_merge_lines(struct Map_info *, int, int *, struct Map_info *);
void Vect_break_polygons(struct Map_info *, int, struct Map_info *);
void Vect_remove_duplicates(struct Map_info *, int, struct Map_info *);
int Vect_line_check_duplicate(const struct line_pnts *,
                              const struct line_pnts *, int);
void Vect_snap_lines(struct Map_info *, int, double, struct Map_info *);
void Vect_snap_lines_list(struct Map_info *, const struct ilist *, double,
                          struct Map_info *);
int Vect_snap_line(struct Map_info *, struct ilist *, struct line_pnts *,
                   double, int, int *, int *);
void Vect_remove_dangles(struct Map_info *, int, double, struct Map_info *);
void Vect_chtype_dangles(struct Map_info *, double, struct Map_info *);
void Vect_select_dangles(struct Map_info *, int, double, struct ilist *);
void Vect_remove_bridges(struct Map_info *, struct Map_info *, int *, int *);
void Vect_chtype_bridges(struct Map_info *, struct Map_info *, int *, int *);
int Vect_remove_small_areas(struct Map_info *, double, struct Map_info *,
                            double *);
int Vect_clean_small_angles_at_nodes(struct Map_info *, int,
                                     struct Map_info *);

/* Overlay */
int Vect_overlay_str_to_operator(const char *);
int Vect_overlay(struct Map_info *, int, struct ilist *, struct ilist *,
                 struct Map_info *, int, struct ilist *, struct ilist *,
                 int, struct Map_info *);
int Vect_overlay_and(struct Map_info *, int, struct ilist *,
                     struct ilist *, struct Map_info *, int,
                     struct ilist *, struct ilist *, struct Map_info *);

/* Graph */
void Vect_graph_init(dglGraph_s *, int);
void Vect_graph_build(dglGraph_s *);
void Vect_graph_add_edge(dglGraph_s *, int, int, double, int);
void Vect_graph_set_node_costs(dglGraph_s *, int, double);
int Vect_graph_shortest_path(dglGraph_s *, int, int, struct ilist *, double *);

/* Network (graph) */
int Vect_net_build_graph(struct Map_info *, int, int, int, const char *,
                         const char *, const char *, int, int);
int Vect_net_shortest_path(struct Map_info *, int, int, struct ilist *,
                           double *);
int Vect_net_get_line_cost(const struct Map_info *, int, int, double *);
int Vect_net_get_node_cost(const struct Map_info *, int, double *);
int Vect_net_nearest_nodes(struct Map_info *, double, double, double, int,
                           double, int *, int *, int *, double *, double *,
                           struct line_pnts *, struct line_pnts *, double *);
int Vect_net_shortest_path_coor(struct Map_info *, double, double, double,
                                double, double, double, double, double,
                                double *, struct line_pnts *, struct ilist *,
                                struct line_pnts *, struct line_pnts *,
                                double *, double *);
int Vect_net_shortest_path_coor2(struct Map_info *, double, double, double,
                                double, double, double, double, double,
                                double *, struct line_pnts *, struct ilist *, struct ilist *,
                                struct line_pnts *, struct line_pnts *,
                                double *, double *);

/* Miscellaneous */
int Vect_topo_dump(const struct Map_info *, FILE *);
double Vect_points_distance(double, double, double, double, double, double,
                            int);
int Vect_option_to_types(const struct Option *);
int Vect_copy_map_lines(struct Map_info *, struct Map_info *);
int Vect_copy_map_lines_field(struct Map_info *, int, struct Map_info *);
int Vect_copy(const char *, const char *, const char *);
int Vect_rename(const char *, const char *);
int Vect_copy_table(const struct Map_info *, struct Map_info *, int, int,
                    const char *, int);
int Vect_copy_table_by_cats(const struct Map_info *, struct Map_info *, int, int,
                            const char *, int, int *, int);
int Vect_copy_tables(const struct Map_info *, struct Map_info *, int);
int Vect_delete(const char *);
int Vect_segment_intersection(double, double, double, double, double, double,
                              double, double, double, double, double, double,
                              double *, double *, double *, double *,
                              double *, double *, int);
int Vect_line_intersection(struct line_pnts *, struct line_pnts *,
                           struct bound_box *, struct bound_box *,
                           struct line_pnts ***, struct line_pnts ***, int *,
                           int *, int);
int Vect_line_check_intersection(struct line_pnts *, struct line_pnts *, int);
int Vect_line_get_intersections(struct line_pnts *, struct line_pnts *,
                                struct line_pnts *, int);
char *Vect_subst_var(const char *, const struct Map_info *);

/* Custom spatial index */
void Vect_spatial_index_init(struct spatial_index *, int);
void Vect_spatial_index_destroy(struct spatial_index *);
void Vect_spatial_index_add_item(struct spatial_index *, int, const struct bound_box *);
void Vect_spatial_index_del_item(struct spatial_index *, int, const struct bound_box *);
int Vect_spatial_index_select(const struct spatial_index *, const struct bound_box *, struct ilist *);

/* GRASS ASCII vector format */
int Vect_read_ascii(FILE *, struct Map_info *);
int Vect_read_ascii_head(FILE *, struct Map_info *);
int Vect_write_ascii(FILE *, FILE *, struct Map_info *, int,
                     int, int, char *, int, int,
                     int, const struct cat_list *, const char*,
                     const char **, int);
void Vect_write_ascii_head(FILE *, struct Map_info *);

/* Simple Features */
SF_FeatureType Vect_sfa_get_line_type(const struct line_pnts *, int, int);
int Vect_sfa_get_type(SF_FeatureType);
int Vect_sfa_check_line_type(const struct line_pnts *, int, SF_FeatureType, int);
int Vect_sfa_line_dimension(int);
char *Vect_sfa_line_geometry_type(const struct line_pnts *, int);
int Vect_sfa_line_astext(const struct line_pnts *, int, int, int, FILE *);
int Vect_sfa_is_line_simple(const struct line_pnts *, int, int);
int Vect_sfa_is_line_closed(const struct line_pnts *, int, int);

/*
 * Internal functions, MUST NOT be used in modules
 */
int Vect_print_header(const struct Map_info *);
void Vect__init_head(struct Map_info *);

/* Open/close/rewind map */
int Vect_coor_info(const struct Map_info *, struct Coor_info *);
const char *Vect_maptype_info(const struct Map_info *);
int Vect_maptype(const struct Map_info *);
int Vect_open_topo(struct Map_info *, int);
int Vect_open_topo_pg(struct Map_info *, int);
int Vect_save_topo(struct Map_info *);
int Vect_open_sidx(struct Map_info *, int);
int Vect_save_sidx(struct Map_info *);
int Vect_sidx_dump(const struct Map_info *, FILE *);
int Vect_build_sidx_from_topo(const struct Map_info *);
int Vect_build_sidx(struct Map_info *);
int Vect_open_fidx(struct Map_info *, struct Format_info_offset *);
int Vect_save_fidx(struct Map_info *, struct Format_info_offset *);
int Vect_fidx_dump(const struct Map_info *, FILE *);
int Vect_save_frmt(struct Map_info *);

int Vect__write_head(const struct Map_info *);
int Vect__read_head(struct Map_info *);
int V1_open_old_nat(struct Map_info *, int);
int V1_open_old_ogr(struct Map_info *, int);
int V1_open_old_pg(struct Map_info *, int);
int V2_open_old_ogr(struct Map_info *);
int V2_open_old_pg(struct Map_info *);
int V1_open_new_nat(struct Map_info *, const char *, int);
int V1_open_new_ogr(struct Map_info *, const char *, int);
int V1_open_new_pg(struct Map_info *, const char *, int);
int V2_open_new_ogr(struct Map_info *, int);
int V2_open_new_pg(struct Map_info *, int);
int V1_rewind_nat(struct Map_info *);
int V1_rewind_ogr(struct Map_info *);
int V1_rewind_pg(struct Map_info *);
int V2_rewind_nat(struct Map_info *);
int V2_rewind_ogr(struct Map_info *);
int V2_rewind_pg(struct Map_info *);
int V1_close_nat(struct Map_info *);
int V1_close_ogr(struct Map_info *);
int V1_close_pg(struct Map_info *);
int V2_close_ogr(struct Map_info *);
int V2_close_pg(struct Map_info *);

/* Read/write lines (internal use only) */
int V1_read_line_nat(struct Map_info *, struct line_pnts *,
                     struct line_cats *, off_t);
int V1_read_line_ogr(struct Map_info *, struct line_pnts *,
                     struct line_cats *, off_t);
int V1_read_line_pg(struct Map_info *, struct line_pnts *,
                    struct line_cats *, off_t);
int V2_read_line_nat(struct Map_info *, struct line_pnts *,
                     struct line_cats *, int);
int V2_read_line_sfa(struct Map_info *, struct line_pnts *,
                     struct line_cats *, int);
int V2_read_line_pg(struct Map_info *, struct line_pnts *,
                    struct line_cats *, int);
int V1_read_next_line_nat(struct Map_info *, struct line_pnts *,
                          struct line_cats *);
int V1_read_next_line_ogr(struct Map_info *, struct line_pnts *,
                          struct line_cats *);
int V1_read_next_line_pg(struct Map_info *, struct line_pnts *,
                         struct line_cats *);
int V2_read_next_line_nat(struct Map_info *, struct line_pnts *,
                          struct line_cats *);
int V2_read_next_line_ogr(struct Map_info *, struct line_pnts *,
                          struct line_cats *);
int V2_read_next_line_pg(struct Map_info *, struct line_pnts *,
                         struct line_cats *);
int V1_delete_line_nat(struct Map_info *, off_t);
int V1_delete_line_ogr(struct Map_info *, off_t);
int V1_delete_line_pg(struct Map_info *, off_t);
int V2_delete_line_nat(struct Map_info *, int);
int V2_delete_line_sfa(struct Map_info *, int);
int V1_restore_line_nat(struct Map_info *, off_t);
int V2_restore_line_nat(struct Map_info *, int, off_t);
off_t V1_write_line_nat(struct Map_info *, int, const struct line_pnts *,
                        const struct line_cats *);
off_t V1_write_line_ogr(struct Map_info *, int, const struct line_pnts *,
                        const struct line_cats *);
off_t V1_write_line_pg(struct Map_info *, int, const struct line_pnts *,
                       const struct line_cats *);
off_t V2_write_line_nat(struct Map_info *, int, const struct line_pnts *,
                        const struct line_cats *);
off_t V2_write_line_sfa(struct Map_info *, int, const struct line_pnts *,
                        const struct line_cats *);
off_t V2_write_line_pg(struct Map_info *, int, const struct line_pnts *,
                       const struct line_cats *);
off_t V1_rewrite_line_nat(struct Map_info *, int, int, off_t,
                          const struct line_pnts *, const struct line_cats *);
off_t V1_rewrite_line_ogr(struct Map_info *, int, int, off_t,
                          const struct line_pnts *, const struct line_cats *);
off_t V1_rewrite_line_pg(struct Map_info *, int, int, off_t,
                         const struct line_pnts *, const struct line_cats *);
off_t V2_rewrite_line_nat(struct Map_info *, int, int, off_t,
                          const struct line_pnts *, const struct line_cats *);
off_t V2_rewrite_line_sfa(struct Map_info *, int, int, off_t,
                          const struct line_pnts *, const struct line_cats *);

    /* Build topology */
int Vect_build_nat(struct Map_info *, int);
void Vect__build_downgrade(struct Map_info *, int);
int Vect__build_sfa(struct Map_info *, int);
int Vect_build_ogr(struct Map_info *, int);
int Vect_build_pg(struct Map_info *, int);
int Vect_build_line_area(struct Map_info *, int, int);
int Vect_isle_find_area(struct Map_info *, int);
int Vect_attach_isle(struct Map_info *, int);
int Vect_attach_isles(struct Map_info *, const struct bound_box *);
int Vect_attach_centroids(struct Map_info *, const struct bound_box *);

    /* GEOS support */
#ifdef HAVE_GEOS
GEOSGeometry *Vect_read_line_geos(struct Map_info *, int, int*);
GEOSGeometry *Vect_line_to_geos(struct Map_info *, const struct line_pnts*, int);
GEOSGeometry *Vect_read_area_geos(struct Map_info *, int);
GEOSCoordSequence *Vect_get_area_points_geos(struct Map_info *, int);
GEOSCoordSequence *Vect_get_isle_points_geos(struct Map_info *, int);
#endif

    /* Raster color tables */
int Vect_read_colors(const char *, const char *, struct Colors *);
int Vect_remove_colors(const char *, const char *);
void Vect_write_colors(const char *, const char *, struct Colors *);

/* Simplified RTree search using an ilist to store rectangle ids */
int RTreeSearch2(struct RTree *, struct RTree_Rect *, struct ilist *);

#endif /* GRASS_VECTORDEFS_H */
