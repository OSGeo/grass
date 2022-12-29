/* angle.c */
int camera_angle(struct Ortho_Image_Group *, char *);

/* aver_z.c */
int get_aver_elev(struct Ortho_Control_Points *, double *);

/* cp.c */
int get_conz_points(struct Ortho_Image_Group *);
int get_ref_points(struct Ortho_Image_Group *);

/* elev_data.c */
int elev_data(char *, int);

/* env.c */
int select_current_env(void);
int select_target_env(void);
int show_env(void);

/* equ.c */
int Compute_ortho_equation(struct Ortho_Image_Group *);
int Compute_ref_equation(struct Ortho_Image_Group *);

/* exec.c */
int exec_rectify(struct Ortho_Image_Group *, int *, char *, char *, char *);

/* get_wind.c */
int get_ref_window(struct Ref *, int *, struct Cell_head *);
int georef_window(struct Ortho_Image_Group *, struct Cell_head *, struct Cell_head *, double);

/* rectify.c */
int rectify(struct Ortho_Image_Group *, char *, char *, struct cache *, double, char *, char *);

/* readcell.c */
struct cache *readcell(int, int, int);
block *get_block(struct cache *, int);
void release_cache(struct cache *);

/* report.c */
int report(time_t, int);

/* target.c */
int get_target(char *);

/* declare resampling methods */
/* bilinear.c */
extern void p_bilinear(struct cache *, void *, int, double *, double *,
		       struct Cell_head *);
/* cubic.c */
extern void p_cubic(struct cache *, void *, int, double *, double *,
		    struct Cell_head *);
/* nearest.c */
extern void p_nearest(struct cache *, void *, int, double *, double *,
		      struct Cell_head *);
/* bilinear_f.c */
extern void p_bilinear_f(struct cache *, void *, int, double *, double *,
		       struct Cell_head *);
/* cubic_f.c */
extern void p_cubic_f(struct cache *, void *, int, double *, double *,
		    struct Cell_head *);
/* lanczos.c */
extern void p_lanczos(struct cache *, void *, int, double *, double *,
		    struct Cell_head *);
extern void p_lanczos_f(struct cache *, void *, int, double *, double *,
		    struct Cell_head *);
