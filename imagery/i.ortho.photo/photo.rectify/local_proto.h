/* ask_elev.c */
int ask_elev_data(void);
/* ask_files.c */
int ask_files(char *);
int dots(char *, int);
/* ask_files2.c */
int ask_file_from_list(char *, char *);
/* ask_wind.c */
int ask_window(struct Cell_head *);
/* aver_z.c */
int get_aver_elev(struct Ortho_Control_Points *, double *);
/* compress.c */
int compress(char *);
/* conv.c */
int view_to_col(View *, int);
int view_to_row(View *, int);
int col_to_view(View *, int);
int row_to_view(View *, int);
double row_to_northing(struct Cell_head *, int, double);
double col_to_easting(struct Cell_head *, int, double);
double northing_to_row(struct Cell_head *, double);
double easting_to_col(struct Cell_head *, double);
/* cp.c */
int get_conz_points(void);
int get_ref_points(void);
/* elev_data.c */
int elev_data(char *, int);
/* env.c */
int select_current_env(void);
int select_target_env(void);
int show_env(void);
/* equ.c */
int Compute_ortho_equation(void);
int Compute_ref_equation(void);
/* exec.c */
int exec_rectify(void);
/* get_wind.c */
int get_target_window(void);
int georef_window(struct Cell_head *, struct Cell_head *);
/* matrix.c */
int compute_georef_matrix(struct Cell_head *, struct Cell_head *);
/* perform.c */
int perform_georef (int, void *rast);
/* ps_cp.c */
int get_psuedo_control_pt(int, int);
/* rectify.c */
int rectify(char *, char *, char *);
/* report.c */
int report(char *, char *, char *, long, long, int);
/* target.c */
int get_target(char *);
/* write.c */
int write_map(char *);
int write_matrix(int, int);
