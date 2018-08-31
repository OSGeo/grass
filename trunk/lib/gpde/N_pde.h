
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      This file contains definitions of variables and data types
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include <grass/gmath.h>

#ifndef _N_PDE_H_
#define _N_PDE_H_

#define N_NORMAL_LES 0
#define N_SPARSE_LES 1
/*!
 * Boundary conditions for cells
 */
#define N_CELL_INACTIVE 0
#define N_CELL_ACTIVE 1
#define N_CELL_DIRICHLET 2
#define N_CELL_TRANSMISSION 3
/*!
 * \brief the maximum number of available cell states (eg: boundary condition, inactiven active)
 * */
#define N_MAX_CELL_STATE 20

#define N_5_POINT_STAR 0
#define N_7_POINT_STAR 1
#define N_9_POINT_STAR 2
#define N_27_POINT_STAR 3

#define N_MAXIMUM_NORM 0
#define N_EUKLID_NORM 1

#define N_ARRAY_SUM 0		/* summ two arrays */
#define N_ARRAY_DIF 1		/* calc the difference between two arrays */
#define N_ARRAY_MUL 2		/* multiply two arrays */
#define N_ARRAY_DIV 3		/* array division, if div with 0 the NULL value is set */

#define N_UPWIND_FULL 0		/*full upwinding stabilization */
#define N_UPWIND_EXP  1		/*exponential upwinding stabilization */
#define N_UPWIND_WEIGHT 2	/*weighted upwinding stabilization */



/* *************************************************************** */
/* *************** LINEARE EQUATION SYSTEM PART ****************** */
/* *************************************************************** */

/*!
 * \brief The linear equation system (les) structure 
 *
 * This structure manages the Ax = b system.
 * It manages regular quadratic matrices or
 * sparse matrices. The vector b and x are normal one dimensional 
 * memory structures of type double. Also the number of rows
 * and the matrix type are stored in this structure.
 * */
typedef struct
{
    double *x;			/*the value vector */
    double *b;			/*the right side of Ax = b */
    double **A;			/*the normal quadratic matrix */
    G_math_spvector **Asp;	/*the sparse matrix */
    int rows;			/*number of rows */
    int cols;			/*number of cols */
    int quad;			/*is the matrix quadratic (1-quadratic, 0 not) */
    int type;			/*the type of the les, normal == 0, sparse == 1 */
} N_les;

extern N_les *N_alloc_les_param(int cols, int rows, int type, int param);
extern N_les *N_alloc_les(int rows, int type);
extern N_les *N_alloc_les_A(int rows, int type);
extern N_les *N_alloc_les_Ax(int rows, int type);
extern N_les *N_alloc_les_Ax_b(int rows, int type);
extern N_les *N_alloc_nquad_les(int cols, int rows, int type);
extern N_les *N_alloc_nquad_les_A(int cols, int rows, int type);
extern N_les *N_alloc_nquad_les_Ax(int cols, int rows, int type);
extern N_les *N_alloc_nquad_les_Ax_b(int cols, int rows, int type);
extern void N_print_les(N_les * les);
extern void N_free_les(N_les * les);

/* *************************************************************** */
/* *************** GEOMETRY INFORMATION ************************** */
/* *************************************************************** */

/*!
 * \brief Geometric information about the structured grid
 * */
typedef struct
{
    int planimetric;		/*If the projection is not planimetric (0), the array calculation is different for each row */
    double *area;		/* the vector of area values for non-planimetric projection for each row */
    int dim;			/* 2 or 3 */

    double dx;
    double dy;
    double dz;

    double Az;

    int depths;
    int rows;
    int cols;

} N_geom_data;

extern N_geom_data *N_alloc_geom_data(void);
extern void N_free_geom_data(N_geom_data * geodata);
extern N_geom_data *N_init_geom_data_3d(RASTER3D_Region * region3d, N_geom_data * geodata);
extern N_geom_data *N_init_geom_data_2d(struct Cell_head *region, N_geom_data * geodata);
extern double N_get_geom_data_area_of_cell(N_geom_data * geom, int row);

/* *************************************************************** */
/* *************** READING RASTER AND VOLUME DATA **************** */
/* *************************************************************** */

typedef struct
{
    int type;			/* which raster type CELL_TYPE, FCELL_TYPE, DCELL_TYPE */
    int rows, cols;
    int rows_intern, cols_intern;
    int offset;			/*number of cols/rows offset at each boundary */
    CELL *cell_array;		/*The data is stored in an one dimensional array internally */
    FCELL *fcell_array;		/*The data is stored in an one dimensional array internally */
    DCELL *dcell_array;		/*The data is stored in an one dimensional array internally */
} N_array_2d;

extern N_array_2d *N_alloc_array_2d(int cols, int rows, int offset, int type);
extern void N_free_array_2d(N_array_2d * data_array);
extern int N_get_array_2d_type(N_array_2d * array2d);
extern void N_get_array_2d_value(N_array_2d * array2d, int col, int row, void *value);
extern CELL N_get_array_2d_c_value(N_array_2d * array2d, int col, int row);
extern FCELL N_get_array_2d_f_value(N_array_2d * array2d, int col, int row);
extern DCELL N_get_array_2d_d_value(N_array_2d * array2d, int col, int row);
extern void N_put_array_2d_value(N_array_2d * array2d, int col, int row, char *value);
extern void N_put_array_2d_c_value(N_array_2d * array2d, int col, int row, CELL value);
extern void N_put_array_2d_f_value(N_array_2d * array2d, int col, int row, FCELL value);
extern void N_put_array_2d_d_value(N_array_2d * array2d, int col, int row, DCELL value);
extern int N_is_array_2d_value_null(N_array_2d * array2d, int col, int row); 
extern void N_put_array_2d_value_null(N_array_2d * array2d, int col, int row);
extern void N_print_array_2d(N_array_2d * data);
extern void N_print_array_2d_info(N_array_2d * data);
extern void N_copy_array_2d(N_array_2d * source, N_array_2d * target);
extern double N_norm_array_2d(N_array_2d * array1, N_array_2d * array2, int type);
extern N_array_2d *N_math_array_2d(N_array_2d * array1, N_array_2d * array2, N_array_2d * result, int type);
extern int N_convert_array_2d_null_to_zero(N_array_2d * a);
extern N_array_2d *N_read_rast_to_array_2d(char *name, N_array_2d * array);
extern void N_write_array_2d_to_rast(N_array_2d * array, char *name);
extern void N_calc_array_2d_stats(N_array_2d * a, double *min, double *max, double *sum, int *nonzero, int withoffset);

typedef struct
{
    int type;			/* which raster type FCELL_TYPE, DCELL_TYPE */
    int rows, cols, depths;
    int rows_intern, cols_intern, depths_intern;
    int offset;			/*number of cols/rows/depths offset at each boundary */
    float *fcell_array;		/*The data is stored in an one dimensional array internally */
    double *dcell_array;	/*The data is stored in an one dimensional array internally */
} N_array_3d;

extern N_array_3d *N_alloc_array_3d(int cols, int rows, int depths, int offset, int type);
extern void N_free_array_3d(N_array_3d * data_array);
extern int N_get_array_3d_type(N_array_3d * array3d);
extern void N_get_array_3d_value(N_array_3d * array3d, int col, int row, int depth, void *value);
extern float N_get_array_3d_f_value(N_array_3d * array3d, int col, int row, int depth);
extern double N_get_array_3d_d_value(N_array_3d * array3d, int col, int row, int depth);
extern void N_put_array_3d_value(N_array_3d * array3d, int col, int row, int depth, char *value);
extern void N_put_array_3d_f_value(N_array_3d * array3d, int col, int row, int depth, float value);
extern void N_put_array_3d_d_value(N_array_3d * array3d, int col, int row, int depth, double value);
extern int N_is_array_3d_value_null(N_array_3d * array3d, int col, int row, int depth);
extern void N_put_array_3d_value_null(N_array_3d * array3d, int col, int row, int depth);
extern void N_print_array_3d(N_array_3d * data);
extern void N_print_array_3d_info(N_array_3d * data);
extern void N_copy_array_3d(N_array_3d * source, N_array_3d * target);
extern double N_norm_array_3d(N_array_3d * array1, N_array_3d * array2, int type);
extern N_array_3d *N_math_array_3d(N_array_3d * array1, N_array_3d * array2, N_array_3d * result, int type);
extern int N_convert_array_3d_null_to_zero(N_array_3d * a);
extern N_array_3d *N_read_rast3d_to_array_3d(char *name, N_array_3d * array, int mask);
extern void N_write_array_3d_to_rast3d(N_array_3d * array, char *name, int mask);
extern void N_calc_array_3d_stats(N_array_3d * a, double *min, double *max, double *sum, int *nonzero, int withoffset);

/* *************************************************************** */
/* *************** MATRIX ASSEMBLING METHODS ********************* */
/* *************************************************************** */
/*!
 * \brief Matrix entries for a mass balance 5/7/9 star system
 *
 * Matrix entries for the mass balance of a 5 star system
 *
 * The entries are center, east, west, north, south and the 
 * right side vector b of Ax = b. This system is typically used in 2d.

 \verbatim
 N
 |
 W-- C --E
 |
 S
 \endverbatim

 * Matrix entries for the mass balance of a 7 star system
 *
 * The entries are center, east, west, north, south, top, bottom and the 
 * right side vector b of Ax = b. This system is typically used in 3d.

 \verbatim
 T N
 |/
 W-- C --E
 /|
 S B
 \endverbatim

 * Matrix entries for the mass balance of a 9 star system
 *
 * The entries are center, east, west, north, south, north-east, south-east,
 * north-wast, south-west and the 
 * right side vector b of Ax = b. This system is typically used in 2d.

 \verbatim
 NW  N  NE
 \ | /
 W-- C --E
 / | \
 SW  S  SE
 \endverbatim

 * Matrix entries for the mass balance of a 27 star system
 *
 * The entries are center, east, west, north, south, north-east, south-east,
 * north-wast, south-west, same for top and bottom and the 
 * right side vector b of Ax = b. This system is typically used in 2d.

 \verbatim
 top:
 NW_T N_Z NE_T
 \ | /
 W_T-- T --E_T
 / | \
 SW_T S_T SE_T

 center:
 NW  N  NE
 \ | /
 W-- C --E
 / | \
 SW  S  SE

 bottom:
 NW_B N_B NE_B
 \ | /
 W_B-- B --E_B
 / | \
 SW_B S_B SE_B
 \endverbatim

 */
typedef struct
{
    int type;
    int count;
    double C, W, E, N, S, NE, NW, SE, SW, V;
    /*top part */
    double T, W_T, E_T, N_T, S_T, NE_T, NW_T, SE_T, SW_T;
    /*bottom part */
    double B, W_B, E_B, N_B, S_B, NE_B, NW_B, SE_B, SW_B;
} N_data_star;

/*!
 * \brief callback structure for 3d matrix assembling
 * */
typedef struct
{
    N_data_star *(*callback) ();
} N_les_callback_3d;

/*!
 * \brief callback structure for 2d matrix assembling
 * */
typedef struct
{
    N_data_star *(*callback) ();
} N_les_callback_2d;


extern void N_set_les_callback_3d_func(N_les_callback_3d * data, N_data_star * (*callback_func_3d) ());
extern void N_set_les_callback_2d_func(N_les_callback_2d * data, N_data_star * (*callback_func_2d) ());
extern N_les_callback_3d *N_alloc_les_callback_3d(void);
extern N_les_callback_2d *N_alloc_les_callback_2d(void);
extern N_data_star *N_alloc_5star(void);
extern N_data_star *N_alloc_7star(void);
extern N_data_star *N_alloc_9star(void);
extern N_data_star *N_alloc_27star(void);
extern N_data_star *N_create_5star(double C, double W, double E, double N,
				   double S, double V);
extern N_data_star *N_create_7star(double C, double W, double E, double N,
				   double S, double T, double B, double V);
extern N_data_star *N_create_9star(double C, double W, double E, double N,
				   double S, double NW, double SW, double NE,
				   double SE, double V);
extern N_data_star *N_create_27star(double C, double W, double E, double N,
				    double S, double NW, double SW, double NE,
				    double SE, double T, double W_T,
				    double E_T, double N_T, double S_T,
				    double NW_T, double SW_T, double NE_T,
				    double SE_T, double B, double W_B,
				    double E_B, double N_B, double S_B,
				    double NW_B, double SW_B, double NE_B,
				    double SE_B, double V);
extern N_data_star *N_callback_template_3d(void *data, N_geom_data * geom, int col, int row, int depth);
extern N_data_star *N_callback_template_2d(void *data, N_geom_data * geom, int col, int row);
extern N_les *N_assemble_les_3d(int les_type, N_geom_data * geom, N_array_3d * status, N_array_3d * start_val, void *data, N_les_callback_3d * callback);
extern N_les *N_assemble_les_3d_active(int les_type, N_geom_data * geom, N_array_3d * status, N_array_3d * start_val, void *data, N_les_callback_3d * callback);
extern N_les *N_assemble_les_3d_dirichlet(int les_type, N_geom_data * geom, N_array_3d * status, N_array_3d * start_val, void *data, N_les_callback_3d * callback);
extern N_les *N_assemble_les_3d_param(int les_type, N_geom_data * geom, N_array_3d * status, N_array_3d * start_val, void *data, N_les_callback_3d * callback, int cell_type);
extern N_les *N_assemble_les_2d(int les_type, N_geom_data * geom, N_array_2d * status, N_array_2d * start_val, void *data, N_les_callback_2d * callback);
extern N_les *N_assemble_les_2d_active(int les_type, N_geom_data * geom, N_array_2d * status, N_array_2d * start_val, void *data, N_les_callback_2d * callback);
extern N_les *N_assemble_les_2d_dirichlet(int les_type, N_geom_data * geom, N_array_2d * status, N_array_2d * start_val, void *data, N_les_callback_2d * callback);
extern N_les *N_assemble_les_2d_param(int les_type, N_geom_data * geom, N_array_2d * status, N_array_2d * start_val, void *data, N_les_callback_2d * callback, int cell_Type);
extern int N_les_pivot_create(N_les * les);
int N_les_integrate_dirichlet_2d(N_les * les, N_geom_data * geom, N_array_2d * status, N_array_2d * start_val);
int N_les_integrate_dirichlet_3d(N_les * les, N_geom_data * geom, N_array_3d * status, N_array_3d * start_val);

/* *************************************************************** */
/* *************** GPDE STANDARD OPTIONS ************************* */
/* *************************************************************** */

/*! \brief Standard options of the gpde library 
 * */
typedef enum
{
    N_OPT_SOLVER_SYMM,		/*! solver for symmetric, positive definite linear equation systems */
    N_OPT_SOLVER_UNSYMM,	/*! solver for unsymmetric linear equation systems */
    N_OPT_MAX_ITERATIONS,	/*! Maximum number of iteration used to solver the linear equation system */
    N_OPT_ITERATION_ERROR,	/*! Error break criteria for the iterative solver (jacobi, sor, cg or bicgstab) */
    N_OPT_SOR_VALUE,		/*! The relaxation parameter used by the jacobi and sor solver for speedup or stabilizing */
    N_OPT_CALC_TIME		/*! The calculation time in seconds */
} N_STD_OPT;

extern struct Option *N_define_standard_option(int opt);

/* *************************************************************** */
/* *************** GPDE MATHEMATICAL TOOLS *********************** */
/* *************************************************************** */

extern double N_calc_arith_mean(double a, double b);
extern double N_calc_arith_mean_n(double *a, int size);
extern double N_calc_geom_mean(double a, double b);
extern double N_calc_geom_mean_n(double *a, int size);
extern double N_calc_harmonic_mean(double a, double b);
extern double N_calc_harmonic_mean_n(double *a, int size);
extern double N_calc_quad_mean(double a, double b);
extern double N_calc_quad_mean_n(double *a, int size);

/* *************************************************************** */
/* *************** UPWIND STABILIZATION ALGORITHMS *************** */
/* *************************************************************** */

extern double N_full_upwinding(double sprod, double distance, double D);
extern double N_exp_upwinding(double sprod, double distance, double D);


/* *************************************************************** */
/* *************** METHODS FOR GRADIENT CALCULATION ************** */
/* *************************************************************** */
/*!
   \verbatim

   ______________ 
   |    |    |    |
   |    |    |    |
   |----|-NC-|----|
   |    |    |    |
   |   WC    EC   |
   |    |    |    |
   |----|-SC-|----|
   |    |    |    |
   |____|____|____|


   |  /
   TC NC
   |/
   --WC-----EC--
   /|
   SC BC
   /  |

   \endverbatim

 */

/*! \brief Gradient between the cells in X and Y direction */
typedef struct
{

    double NC, SC, WC, EC;

} N_gradient_2d;

/*! \brief Gradient between the cells in X, Y and Z direction */
typedef struct
{

    double NC, SC, WC, EC, TC, BC;

} N_gradient_3d;


/*!
   \verbatim

   Gradient in X direction between the cell neighbours
   ____ ____ ____
   |    |    |    |
   |   NWN  NEN   |
   |____|____|____|
   |    |    |    |
   |   WN    EN   |
   |____|____|____|
   |    |    |    |
   |   SWS  SES   |
   |____|____|____|

   Gradient in Y direction between the cell neighbours
   ______________ 
   |    |    |    |
   |    |    |    |
   |NWW-|-NC-|-NEE|
   |    |    |    |
   |    |    |    |
   |SWW-|-SC-|-SEE|
   |    |    |    |
   |____|____|____|

   Gradient in Z direction between the cell neighbours
   /______________/
   /|    |    |    |
   | NWZ| NZ | NEZ|
   |____|____|____|
   /|    |    |    |
   | WZ | CZ | EZ |
   |____|____|____|
   /|    |    |    |
   | SWZ| SZ | SEZ|
   |____|____|____|
   /____/____/____/


   \endverbatim
 */

/*! \brief Gradient between the cell neighbours in X direction */
typedef struct
{

    double NWN, NEN, WC, EC, SWS, SES;

} N_gradient_neighbours_x;

/*! \brief Gradient between the cell neighbours in Y direction */
typedef struct
{

    double NWW, NEE, NC, SC, SWW, SEE;

} N_gradient_neighbours_y;

/*! \brief Gradient between the cell neighbours in Z direction */
typedef struct
{

    double NWZ, NZ, NEZ, WZ, CZ, EZ, SWZ, SZ, SEZ;

} N_gradient_neighbours_z;

/*! \brief Gradient between the cell neighbours in X and Y direction */
typedef struct
{

    N_gradient_neighbours_x *x;
    N_gradient_neighbours_y *y;

} N_gradient_neighbours_2d;


/*! \brief Gradient between the cell neighbours in X, Y and Z direction */
typedef struct
{

    N_gradient_neighbours_x *xt;	/*top values */
    N_gradient_neighbours_x *xc;	/*center values */
    N_gradient_neighbours_x *xb;	/*bottom values */

    N_gradient_neighbours_y *yt;	/*top values */
    N_gradient_neighbours_y *yc;	/*center values */
    N_gradient_neighbours_y *yb;	/*bottom values */

    N_gradient_neighbours_z *zt;	/*top-center values */
    N_gradient_neighbours_z *zb;	/*bottom-center values */

} N_gradient_neighbours_3d;


/*! Two dimensional gradient field */
typedef struct
{

    N_array_2d *x_array;
    N_array_2d *y_array;
    int cols, rows;
    double min, max, mean, sum;
    int nonull;

} N_gradient_field_2d;

/*! Three dimensional gradient field */
typedef struct
{

    N_array_3d *x_array;
    N_array_3d *y_array;
    N_array_3d *z_array;
    int cols, rows, depths;
    double min, max, mean, sum;
    int nonull;

} N_gradient_field_3d;


extern N_gradient_2d *N_alloc_gradient_2d(void);
extern void N_free_gradient_2d(N_gradient_2d * grad);
extern N_gradient_2d *N_create_gradient_2d(double NC, double SC, double WC, double EC);
extern int N_copy_gradient_2d(N_gradient_2d * source, N_gradient_2d * target);
extern N_gradient_2d *N_get_gradient_2d(N_gradient_field_2d * field, N_gradient_2d * gradient, int col, int row);
extern N_gradient_3d *N_alloc_gradient_3d(void);
extern void N_free_gradient_3d(N_gradient_3d * grad);
extern N_gradient_3d *N_create_gradient_3d(double NC, double SC, double WC, double EC, double TC, double BC);
extern int N_copy_gradient_3d(N_gradient_3d * source, N_gradient_3d * target);
extern N_gradient_3d *N_get_gradient_3d(N_gradient_field_3d * field, N_gradient_3d * gradient, int col, int row, int depth);
extern N_gradient_neighbours_x *N_alloc_gradient_neighbours_x(void);
extern void N_free_gradient_neighbours_x(N_gradient_neighbours_x * grad);
extern N_gradient_neighbours_x *N_create_gradient_neighbours_x(double NWN,
							       double NEN,
							       double WC,
							       double EC,
							       double SWS,
							       double SES);
extern int N_copy_gradient_neighbours_x(N_gradient_neighbours_x * source, N_gradient_neighbours_x * target);
extern N_gradient_neighbours_y *N_alloc_gradient_neighbours_y(void);
extern void N_free_gradient_neighbours_y(N_gradient_neighbours_y * grad);
extern N_gradient_neighbours_y *N_create_gradient_neighbours_y(double NWW,
							       double NEE,
							       double NC,
							       double SC,
							       double SWW,
							       double SEE);
extern int N_copy_gradient_neighbours_y(N_gradient_neighbours_y * source, N_gradient_neighbours_y * target);
extern N_gradient_neighbours_z *N_alloc_gradient_neighbours_z(void);
extern void N_free_gradient_neighbours_z(N_gradient_neighbours_z * grad);
extern N_gradient_neighbours_z *N_create_gradient_neighbours_z(double NWZ,
							       double NZ,
							       double NEZ,
							       double WZ,
							       double CZ,
							       double EZ,
							       double SWZ,
							       double SZ,
							       double SEZ);
extern int N_copy_gradient_neighbours_z(N_gradient_neighbours_z * source, N_gradient_neighbours_z * target);
extern N_gradient_neighbours_2d *N_alloc_gradient_neighbours_2d(void);
extern void N_free_gradient_neighbours_2d(N_gradient_neighbours_2d * grad);
extern N_gradient_neighbours_2d * N_create_gradient_neighbours_2d(N_gradient_neighbours_x * x, N_gradient_neighbours_y * y);
extern int N_copy_gradient_neighbours_2d(N_gradient_neighbours_2d * source, N_gradient_neighbours_2d * target);
extern N_gradient_neighbours_2d * N_get_gradient_neighbours_2d(N_gradient_field_2d * field, N_gradient_neighbours_2d * gradient, int col, int row);
extern N_gradient_neighbours_3d *N_alloc_gradient_neighbours_3d(void);
extern void N_free_gradient_neighbours_3d(N_gradient_neighbours_3d * grad);
extern N_gradient_neighbours_3d
    * N_create_gradient_neighbours_3d(N_gradient_neighbours_x * xt,
				      N_gradient_neighbours_x * xc,
				      N_gradient_neighbours_x * xb,
				      N_gradient_neighbours_y * yt,
				      N_gradient_neighbours_y * yc,
				      N_gradient_neighbours_y * yb,
				      N_gradient_neighbours_z * zt,
				      N_gradient_neighbours_z * zb);
extern int N_copy_gradient_neighbours_3d(N_gradient_neighbours_3d * source, N_gradient_neighbours_3d * target);
extern void N_print_gradient_field_2d_info(N_gradient_field_2d * field);
extern void N_calc_gradient_field_2d_stats(N_gradient_field_2d * field);
extern N_gradient_field_2d *N_alloc_gradient_field_2d(int cols, int rows);
extern void N_free_gradient_field_2d(N_gradient_field_2d * field);
extern int N_copy_gradient_field_2d(N_gradient_field_2d * source, N_gradient_field_2d * target);
extern N_gradient_field_2d *N_compute_gradient_field_2d(N_array_2d * pot,
							N_array_2d * weight_x,
							N_array_2d * weight_y,
							N_geom_data * geom,
							N_gradient_field_2d *
							gradfield);
extern void N_compute_gradient_field_components_2d(N_gradient_field_2d * field, N_array_2d * x_comp, N_array_2d * y_comp);
extern void N_print_gradient_field_3d_info(N_gradient_field_3d * field);
extern void N_calc_gradient_field_3d_stats(N_gradient_field_3d * field);
extern N_gradient_field_3d *N_alloc_gradient_field_3d(int cols, int rows, int depths);
extern void N_free_gradient_field_3d(N_gradient_field_3d * field);
extern int N_copy_gradient_field_3d(N_gradient_field_3d * source, N_gradient_field_3d * target);
extern N_gradient_field_3d *N_compute_gradient_field_3d(N_array_3d * pot,
							N_array_3d * weight_x,
							N_array_3d * weight_y,
							N_array_3d * weight_z,
							N_geom_data * geom,
							N_gradient_field_3d *
							gradfield);
extern void N_compute_gradient_field_components_3d(N_gradient_field_3d * field, N_array_3d * x_comp, N_array_3d * y_comp, N_array_3d * z_comp);

#endif
