/*
 * modified by Brown in June 1999 - added elatt & smatt
 * modified by Mitasova Nov. 9, 1999 - added parameter for dtens to output2d
 */
#include <grass/Vect.h>
#include <grass/bitmap.h>
#include <grass/dataquad.h>
#include <grass/qtree.h>
#include <grass/dbmi.h>

/* for resample program */
struct fcell_triple
{
    double x;
    double y;
    FCELL z;
    double smooth;
};

#ifdef POINT2D_C
struct line_pnts *Pnts;
struct line_cats *Cats2;
dbDriver *driver2;
dbString sql2;
struct Map_info Map2;
struct field_info *ff;
int count;
#else
extern struct line_pnts *Pnts;
extern struct line_cats *Cats2;
extern dbDriver *driver2;
extern dbString sql2;
extern struct Map_info Map2;
extern struct field_info *ff;
extern int count;
#endif

struct interp_params
{
    double zmult;		/* multiplier for z-values */
    FILE *fdinp;		/* input stream */
    int elatt;			/* which floating point attr to use? first = 1, second = 2, etc */
    int smatt;			/* which floating point attr to use for smoothing? first = 1, second = 2, etc */
    int kmin;			/* min number of points per segment for interpolation */
    int kmax;			/* max number of points per segment */
    char *maskmap;		/* name of mask */
    int nsizr, nsizc;		/* number of rows and columns */
    DCELL *az, *adx, *ady, *adxx, *adyy, *adxy;	/* array for interpolated values */
    double fi;			/* tension */
    int KMAX2;			/* max num. of points for interp. */
    int scik1, scik2, scik3;	/* multipliers for interp. values */
    double rsm;			/* smoothing */
    char *elev, *slope, *aspect, *pcurv, *tcurv, *mcurv;	/* output files */
    double dmin;		/* min distance between points */
    double x_orig, y_orig;	/* origin */
    int deriv, cv;		/* 1 if compute partial derivs */
    double theta;		/* anisotropy angle, 0=East,counter-clockwise */
    double scalex;		/* anisotropy scaling factor */
    struct TimeStamp *ts;	/* timestamp for raster files */
    FILE *Tmp_fd_z, *Tmp_fd_dx, *Tmp_fd_dy,	/* temp files for writing interp. */
     *Tmp_fd_xx, *Tmp_fd_yy, *Tmp_fd_xy;	/* values */
    FILE *fddevi;		/* pointer to deviations file */

    int (*grid_calc) ();	/*calculates grid for given segm */
    int (*matrix_create) ();	/*creates matrix for a given segm */
    int (*check_points) ();	/*checks interp. func. at points */
    int (*secpar) ();		/* calculates aspect,slope,curv. */
    double (*interp) ();	/* radial  based interp. function */
    int (*interpder) ();	/* interp. func. for derivatives */
    int (*wr_temp) ();		/* writes temp files */
    char *wheresql;		/* SQL statement to select input points */
};

/* distance.c */
double IL_dist_square(double *, double *, int);

/* func2d.c */
double IL_crst(double, double);
int IL_crstg(double, double, double *, double *);

/* init2d.c */
void IL_init_params_2d(struct interp_params *, FILE *, int, int, double,
		       int, int, char *, int, int,
		       DCELL *, DCELL *, DCELL *, DCELL *, DCELL *, DCELL *,
		       double, int, int, int, int, double,
		       char *, char *, char *, char *, char *, char *,
		       double, double, double, int, double, double,
		       FILE *, FILE *, FILE *, FILE *, FILE *, FILE *, FILE *,
		       struct TimeStamp *, int, char *);

void IL_init_func_2d(struct interp_params *, int (*)(), int (*)(), int (*)(),
		     int (*)(), double (*)(), int (*)(), int (*)());
/* input2d.c */
int IL_input_data_2d(struct interp_params *, struct tree_info *, double *,
		     double *, double *, double *, double *, double *, int *);
struct BM *IL_create_bitmask(struct interp_params *);
int translate_quad(struct multtree *, double, double, double, int);

/* interp2d.c */
int IL_grid_calc_2d(struct interp_params *, struct quaddata *, struct BM *,
		    double, double, double *, double *, double *, double *,
		    double *, double *, double *, double *, double *,
		    double *, int, double);
/* matrix.c */
int IL_matrix_create(struct interp_params *, struct triple *, int, double **,
		     int *);
/* minmax.c */
int min1(int, int);
int max1(int, int);
double amax1(double, double);
double amin1(double, double);

/* newsegm2d.c */
int IL_interp_segments_new_2d(struct interp_params *, struct tree_info *,
			      struct multtree *, struct BM *, double, double,
			      double *, double *, double *, double *,
			      double *, double *, double *, double *,
			      double *, int, int, double);
/* output2d.c */
int IL_output_2d(struct interp_params *, struct Cell_head *, double, double,
		 double, double, double, double, double, double, double,
		 double, double, char *, double, int, int, int);
/* point2d.c */
int IL_check_at_points_2d(struct interp_params *, struct quaddata *, double *,
			  double *, double, double, struct triple);
/* resout2d.c */
/* resout2dmod.c */
int IL_resample_output_2d(struct interp_params *, double, double, double,
			  double, double, double, double, double, double,
			  double, double, char *, double *,
			  struct Cell_head *, struct Cell_head *, char *,
			  int);
/* ressegm2d.c */
int IL_resample_interp_segments_2d(struct interp_params *, struct BM *,
				   double, double, double *, double *,
				   double *, double *, double *, double *,
				   double *, double *, double *, int,
				   double *, int, int, int, int, int, double,
				   double, double, double, int);
/* secpar2d.c */
int IL_secpar_loop_2d(struct interp_params *, int, int, int, struct BM *,
		      double *, double *, double *, double *, double *,
		      double *, int, int);
/* segmen2d.c */
int IL_interp_segments_2d(struct interp_params *, struct tree_info *,
			  struct multtree *, struct BM *, double, double,
			  double *, double *, double *, double *, double *,
			  double *, double *, double *, double *, int, int,
			  double);
/* vinput2d.c */
int IL_vector_input_data_2d(struct interp_params *, struct Map_info *, int,
			    char *, char *, struct tree_info *, double *,
			    double *, double *, double *, double *, double *,
			    int *, double *);
int process_point(double, double, double, double, struct tree_info *, double,
		  double *, double *, double *, double *, double *, double *,
		  int *, int *, int *);
/* write2d.c */
int IL_write_temp_2d(struct interp_params *, int, int, int);
