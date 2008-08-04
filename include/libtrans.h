#ifndef __LIBTRANS_H__
#define __LIBTRANS_H__

/* to give the size of the multi-dim array parameter is necessary here,
   because DEC's cc V5.6-079 on Digital UNIX V4.0 (Rev. 878) seems to need it
 */
#define DIM_matrix 3

/* inverse.c */
int inverse(double[DIM_matrix][DIM_matrix]);
int isnull(double[DIM_matrix][DIM_matrix]);

/* m_mult.c */
int m_mult(double[DIM_matrix][DIM_matrix], double *, double *);

/* transform.c */
int compute_transformation_coef(double *, double *, double *, double *, int *,
				int);
int transform_a_into_b(double, double, double *, double *);
int transform_b_into_a(double, double, double *, double *);
int residuals_a_predicts_b(double *, double *, double *, double *, int *, int,
			   double *, double *);
int residuals_b_predicts_a(double *, double *, double *, double *, int *, int,
			   double *, double *);
int print_transform_matrix(void);

#endif /* __LIBTRANS_H__ */
