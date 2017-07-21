
/* cp.c */
int get_control_points(struct Image_Group *, int);

/* env.c */
int select_current_env(void);
int select_target_env(void);
int show_env(void);

/* exec.c */
int exec_rectify(struct Image_Group *, int *, char *, char *, int);

/* get_wind.c */
int get_ref_window(struct Ref *, int *, struct Cell_head *);
int georef_window(struct Image_Group *, struct Cell_head *, struct Cell_head *, int, double);

/* rectify.c */
int rectify(struct Image_Group *, char *, char *, char *, int, char *);

/* readcell.c */
struct cache *readcell(int, int);
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
