#ifndef GRASS_GMATHDEFS_H
#define GRASS_GMATHDEFS_H

/* dalloc.c */
double *G_alloc_vector(size_t);
double **G_alloc_matrix(int, int);
float  *G_alloc_fvector(size_t);
float  **G_alloc_fmatrix(int, int);
void G_free_vector(double *);
void G_free_matrix(double **);
void G_free_fvector(float *);
void G_free_fmatrix(float **);

/* ialloc.c */
int *G_alloc_ivector(size_t);
int **G_alloc_imatrix(int, int);
void G_free_ivector(int *);
void G_free_imatrix(int **);

/* fft.c */
extern int fft(int, double *[2], int, int, int);
extern int fft2(int, double (*)[2], int, int, int);

/* gauss.c */
extern double G_math_rand_gauss(double);

/* max_pow2.c */
extern long G_math_max_pow2 (long n);
extern long G_math_min_pow2 (long n);

/* rand1.c */
extern void G_math_srand(int);
extern int G_math_srand_auto(void);
extern float G_math_rand(void);

/* del2g.c */
extern int del2g(double *[2], int, double);

/* getg.c */
extern int getg(double, double *[2], int);

/* eigen_tools.c */
extern int G_math_egvorder(double *, double **, long);

/* mult.c */
extern int G_math_complex_mult (double *v1[2], int size1, double *v2[2], int size2, double *v3[2], int size3);

/* lu.c*/
extern int G_ludcmp(double **, int, int *, double *);
extern void G_lubksb(double **a, int n, int *indx, double b[]);

/* findzc.c */
extern int G_math_findzc(double conv[], int size, double zc[], double thresh, int num_orients);


/* *************************************************************** */
/* ***** WRAPPER FOR CCMATH FUNCTIONS USED IN GRASS ************** */
/* *************************************************************** */
extern int G_math_solv(double **,double *,int);
extern int G_math_solvps(double **,double *,int);
extern void G_math_solvtd(double *,double *,double *,double *,int);
extern int G_math_solvru(double **,double *,int);
extern int G_math_minv(double **,int);
extern int G_math_psinv(double **,int);
extern int G_math_ruinv(double **,int);
extern void G_math_eigval(double **,double *,int);
extern void G_math_eigen(double **,double *,int);
extern double G_math_evmax(double **,double *,int);
extern int G_math_svdval(double *,double **,int,int);
extern int G_math_sv2val(double *,double **,int,int);
extern int G_math_svduv(double *,double **,double **, int,double **,int);
extern int G_math_sv2uv(double *,double **,double **,int,double **,int);
extern int G_math_svdu1v(double *,double **,int,double **,int);

/* *************************************************************** */
/* *************** LINEARE EQUATION SYSTEM PART ****************** */
/* *************************************************************** */

/* Sparse matrix and sparse vector functions
 * */
extern G_math_spvector *G_math_alloc_spvector(int );
extern G_math_spvector **G_math_alloc_spmatrix(int );
extern void G_math_free_spmatrix(G_math_spvector ** , int );
extern void G_math_free_spvector(G_math_spvector * );
extern int G_math_add_spvector(G_math_spvector **, G_math_spvector * , int );
extern G_math_spvector **G_math_A_to_Asp(double **, int, double);
extern double **G_math_Asp_to_A(G_math_spvector **, int);
extern double **G_math_Asp_to_sband_matrix(G_math_spvector **, int, int);
extern G_math_spvector **G_math_sband_matrix_to_Asp(double **, int, int, double);
extern void G_math_print_spmatrix(G_math_spvector **, int);
extern void G_math_Ax_sparse(G_math_spvector **, double *, double *, int );

/*Symmetric band matrix handling */
extern double **G_math_matrix_to_sband_matrix(double **, int, int);
extern double **G_math_sband_matrix_to_matrix(double **, int, int);
extern void G_math_Ax_sband(double ** A, double *x, double *y, int rows, int bandwidth);

/*linear equation solver, most of them are multithreaded with OpenMP*/
extern int G_math_solver_gauss(double **, double *, double *, int );
extern int G_math_solver_lu(double **, double *, double *, int );
extern int G_math_solver_cholesky(double **, double *, double *, int , int );
extern void G_math_solver_cholesky_sband(double **, double *, double *, int, int);
extern int G_math_solver_jacobi(double **, double *, double *, int , int , double , double );
extern int G_math_solver_gs(double **, double *, double *, int , int , double , double );

extern int G_math_solver_pcg(double **, double *, double *, int , int , double , int );
extern int G_math_solver_cg(double **, double *, double *, int , int , double );
extern int G_math_solver_cg_sband(double **, double *, double *, int, int, int, double);
extern int G_math_solver_bicgstab(double **, double *, double *, int , int , double );
extern int G_math_solver_sparse_jacobi(G_math_spvector **, double *, double *, int , int , double , double );
extern int G_math_solver_sparse_gs(G_math_spvector **, double *, double *, int , int , double , double );
extern int G_math_solver_sparse_pcg(G_math_spvector **, double *, double *, int , int , double , int );
extern int G_math_solver_sparse_cg(G_math_spvector **, double *, double *, int , int , double );
extern int G_math_solver_sparse_bicgstab(G_math_spvector **, double *, double *, int , int , double );

/* solver algoithms and helper functions*/
extern void G_math_gauss_elimination(double **, double *, int );
extern void G_math_lu_decomposition(double **, double *, int );
extern int G_math_cholesky_decomposition(double **, int , int );
extern void G_math_cholesky_sband_decomposition(double **, double **, int, int);
extern void G_math_backward_substitution(double **, double *, double *, int );
extern void G_math_forward_substitution(double **, double *, double *, int );
extern void G_math_cholesky_sband_substitution(double **, double *, double *, int, int);

/*BLAS like level 1,2 and 3 functions*/

/*level 1 vector - vector grass implementation with OpenMP thread support*/
extern void G_math_d_x_dot_y(double *, double *, double *, int );
extern void G_math_d_asum_norm(double *, double *, int );
extern void G_math_d_euclid_norm(double *, double *, int );
extern void G_math_d_max_norm(double *, double *, int );
extern void G_math_d_ax_by(double *, double *, double *, double , double , int );
extern void G_math_d_copy(double *, double *, int );

extern void G_math_f_x_dot_y(float *, float *, float *, int );
extern void G_math_f_asum_norm(float *, float *, int );
extern void G_math_f_euclid_norm(float *, float *, int );
extern void G_math_f_max_norm(float *, float *, int );
extern void G_math_f_ax_by(float *, float *, float *, float , float , int );
extern void G_math_f_copy(float *, float *, int );

extern void G_math_i_x_dot_y(int *, int *,  double *, int );
extern void G_math_i_asum_norm(int *,  double *, int );
extern void G_math_i_euclid_norm(int *,  double *,int );
extern void G_math_i_max_norm(int *,  int *, int );
extern void G_math_i_ax_by(int *, int *, int *, int , int , int );
extern void G_math_i_copy(int *, int *, int );

/*ATLAS blas level 1 wrapper*/
extern double G_math_ddot(double *, double *, int );
extern float G_math_sdot(float *, float *, int );
extern float G_math_sdsdot(float *, float *, float , int );
extern double G_math_dnrm2(double *, int );
extern double G_math_dasum(double *, int );
extern double G_math_idamax(double *, int );
extern float  G_math_snrm2(float *, int );
extern float  G_math_sasum(float *, int );
extern float  G_math_isamax(float *, int );
extern void G_math_dscal(double *, double , int );
extern void G_math_sscal(float *, float , int );
extern void G_math_dcopy(double *, double *, int );
extern void G_math_scopy(float *, float *, int );
extern void G_math_daxpy(double *, double *, double , int );
extern void G_math_saxpy(float *, float *, float , int );

/*level 2 matrix - vector grass implementation with OpenMP thread support*/
extern void G_math_d_Ax(double **, double *, double *, int , int );
extern void G_math_f_Ax(float **, float *, float *, int , int );
extern void G_math_d_x_dyad_y(double *, double *, double **, int, int );
extern void G_math_f_x_dyad_y(float *, float *, float **, int, int );
extern void G_math_d_aAx_by(double **, double *, double *, double , double , double *, int , int );
extern void G_math_f_aAx_by(float **, float *, float *, float , float , float *, int , int );
extern int G_math_d_A_T(double **A, int rows);
extern int G_math_f_A_T(float **A, int rows);

/*level 3 matrix - matrix grass implementation with OpenMP thread support*/
extern void G_math_d_aA_B(double **, double **, double , double **, int , int );
extern void G_math_f_aA_B(float **, float **, float , float **, int , int );
extern void G_math_d_AB(double **, double **, double **, int , int , int );
extern void G_math_f_AB(float **,  float **,  float **,  int , int , int );

#endif /* GRASS_GMATHDEFS_H */
