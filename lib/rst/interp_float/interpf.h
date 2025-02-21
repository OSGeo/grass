/*
 * modified by Brown in June 1999 - added elatt & smatt
 * modified by Mitasova Nov. 9, 1999 - added parameter for dtens to output2d
 */

#include <grass/config.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/bitmap.h>
#include <grass/dataquad.h>
#include <grass/qtree.h>
#include <grass/dbmi.h>
#ifdef GRASS_CMAKE_BUILD
#include <export/grass_interpfl_export.h>
#else
#define GRASS_INTERPFL_EXPORT
#endif

/* for resample program */
struct fcell_triple {
    double x;
    double y;
    FCELL z;
    double smooth;
};

#ifdef POINT2D_C
GRASS_INTERPFL_EXPORT struct line_pnts *Pnts;
GRASS_INTERPFL_EXPORT struct line_cats *Cats2;
GRASS_INTERPFL_EXPORT dbDriver *driver2;
GRASS_INTERPFL_EXPORT dbString sql2;
GRASS_INTERPFL_EXPORT struct Map_info Map2;
GRASS_INTERPFL_EXPORT struct field_info *ff;
GRASS_INTERPFL_EXPORT int count;
#else
GRASS_INTERPFL_EXPORT extern struct line_pnts *Pnts;
GRASS_INTERPFL_EXPORT extern struct line_cats *Cats2;
GRASS_INTERPFL_EXPORT extern dbDriver *driver2;
GRASS_INTERPFL_EXPORT extern dbString sql2;
GRASS_INTERPFL_EXPORT extern struct Map_info Map2;
GRASS_INTERPFL_EXPORT extern struct field_info *ff;
GRASS_INTERPFL_EXPORT extern int count;
#endif

struct interp_params;

typedef int grid_calc_fn(struct interp_params *, struct quaddata *, struct BM *,
                         double, double, double *, double *, double *, double *,
                         double *, double *, double *, double *, double *,
                         double *, off_t, double);

typedef int matrix_create_fn(struct interp_params *, struct triple *, int,
                             double **, int *);

typedef int check_points_fn(struct interp_params *, struct quaddata *, double *,
                            double *, double, double, struct triple *);

typedef int secpar_fn(struct interp_params *, int, int, int, struct BM *,
                      double *, double *, double *, double *, double *,
                      double *, int, int);

typedef double interp_fn(double, double);

typedef int interpder_fn(double, double, double *, double *);

typedef int wr_temp_fn(struct interp_params *, int, int, off_t);

struct interp_params {

    double zmult; /**< multiplier for z-values */

    FILE *fdinp; /**< input stream */

    int elatt; /**< which floating point attr to
                * use? first = 1, second = 2, etc
                */

    int smatt; /**< which floating point attr to use
                  for smoothing? first = 1, second =
                  2, etc */

    int kmin; /**< min number of points per segment
                 for interpolation */

    int kmax; /**< max number of points per segment
               */

    char *maskmap; /**< name of mask */

    int nsizr, nsizc; /**< number of rows and columns */

    DCELL *az, *adx, *ady, *adxx, *adyy, *adxy;
    /**< array for interpolated values */

    double fi; /**< tension */

    int KMAX2; /**< max num. of points for interp.
                */

    int scik1, scik2, scik3; /**< multipliers for
                                interp. values */

    double rsm; /**< smoothing */

    char *elev, *slope, *aspect, *pcurv, *tcurv, *mcurv;
    /**< output files */

    double dmin; /**< min distance between points */

    double x_orig, y_orig; /**< origin */

    int deriv, cv; /**< 1 if compute partial derivs */

    double theta; /**< anisotropy angle,
                     0=East,counter-clockwise */

    double scalex; /**< anisotropy scaling factor */

    struct TimeStamp *ts; /**< timestamp for raster files */

    FILE *Tmp_fd_z, *Tmp_fd_dx, *Tmp_fd_dy, *Tmp_fd_xx, *Tmp_fd_yy, *Tmp_fd_xy;
    /**< temp files for writing interp. values */

    bool create_devi; /**< create deviations file? */

    grid_calc_fn *grid_calc; /**< calculates grid for given segm */

    matrix_create_fn *matrix_create; /**< creates matrix for a given segm */

    check_points_fn *check_points; /**< checks interp. func. at points */

    secpar_fn *secpar; /**< calculates aspect,slope,curv. */

    interp_fn *interp; /**< radial  based interp. function */

    interpder_fn *interpder; /**< interp. func. for derivatives */

    wr_temp_fn *wr_temp; /**< writes temp files */

    const char *wheresql; /**< SQL statement to select input points */
};

/* distance.c */
double IL_dist_square(double *, double *, int);

/* func2d.c */
double IL_crst(double, double);
int IL_crstg(double, double, double *, double *);

/* init2d.c */
void IL_init_params_2d(struct interp_params *, FILE *, int, int, double, int,
                       int, char *, int, int, DCELL *, DCELL *, DCELL *,
                       DCELL *, DCELL *, DCELL *, double, int, int, int, int,
                       double, char *, char *, char *, char *, char *, char *,
                       double, double, double, int, double, double, FILE *,
                       FILE *, FILE *, FILE *, FILE *, FILE *, bool,
                       struct TimeStamp *, int, const char *);

void IL_init_func_2d(struct interp_params *, grid_calc_fn *, matrix_create_fn *,
                     check_points_fn *, secpar_fn *, interp_fn *,
                     interpder_fn *, wr_temp_fn *);

/* input2d.c */
int IL_input_data_2d(struct interp_params *, struct tree_info *, double *,
                     double *, double *, double *, double *, double *, int *);
struct BM *IL_create_bitmask(struct interp_params *);
int translate_quad(struct multtree *, double, double, double, int);

/* interp2d.c */
int IL_grid_calc_2d(struct interp_params *, struct quaddata *, struct BM *,
                    double, double, double *, double *, double *, double *,
                    double *, double *, double *, double *, double *, double *,
                    off_t, double);
/* matrix.c */
int IL_matrix_create(struct interp_params *, struct triple *, int, double **,
                     int *);
int IL_matrix_create_alloc(struct interp_params *, struct triple *, int,
                           double **, int *, double *);
/* minmax.c */
int min1(int, int);
int max1(int, int);
double amax1(double, double);
double amin1(double, double);

/* newsegm2d.c */
int IL_interp_segments_new_2d(struct interp_params *, struct tree_info *,
                              struct multtree *, struct BM *, double, double,
                              double *, double *, double *, double *, double *,
                              double *, double *, double *, double *, int, int,
                              double);
/* output2d.c */
int IL_output_2d(struct interp_params *, struct Cell_head *, double, double,
                 double, double, double, double, double, double, double, double,
                 double, char *, double, int, int, int);
/* point2d.c */
int IL_check_at_points_2d(struct interp_params *, struct quaddata *, double *,
                          double *, double, double, struct triple *);
int IL_write_point_2d(struct triple, double);

/* point2d_parallel.c */
int IL_check_at_points_2d_cvdev(struct interp_params *, struct quaddata *,
                                double *, double *, double, double,
                                struct triple *);

/* resout2d.c */
/* resout2dmod.c */
int IL_resample_output_2d(struct interp_params *, double, double, double,
                          double, double, double, double, double, double,
                          double, double, char *, double *, struct Cell_head *,
                          struct Cell_head *, char *, int);
/* ressegm2d.c */
int IL_resample_interp_segments_2d(struct interp_params *, struct BM *, double,
                                   double, double *, double *, double *,
                                   double *, double *, double *, double *,
                                   double *, double *, off_t, double *, int,
                                   int, int, int, int, double, double, double,
                                   double, int);
/* secpar2d.c */
int IL_secpar_loop_2d(struct interp_params *, int, int, int, struct BM *,
                      double *, double *, double *, double *, double *,
                      double *, int, int);
/* segmen2d.c */
double smallest_segment(struct multtree *, int);
int IL_interp_segments_2d(struct interp_params *, struct tree_info *,
                          struct multtree *, struct BM *, double, double,
                          double *, double *, double *, double *, double *,
                          double *, double *, double *, double *, int, off_t,
                          double);
/* segmen2d_parallel.c */
int IL_interp_segments_2d_parallel(struct interp_params *, struct tree_info *,
                                   struct multtree *, struct BM *, double,
                                   double, double *, double *, double *,
                                   double *, double *, double *, double *,
                                   double *, double *, int, off_t, double, int);
/* vinput2d.c */
int IL_vector_input_data_2d(struct interp_params *, struct Map_info *, int,
                            char *, char *, struct tree_info *, double *,
                            double *, double *, double *, double *, double *,
                            int *, double *);
int process_point(double, double, double, double, struct tree_info *, double,
                  double *, double *, double *, double *, double *, double *,
                  int *, int *, int *);
/* write2d.c */
int IL_write_temp_2d(struct interp_params *, int, int, off_t);
