#ifndef _NR_UTILS_H_
#define _NR_UTILS_H_

extern float sqrarg;
extern double dsqrarg;
extern double dmaxarg1, dmaxarg2;
extern double dminarg1, dminarg2;
extern float maxarg1, maxarg2;
extern float minarg1, minarg2;
extern long lmaxarg1, lmaxarg2;
extern long lminarg1, lminarg2;
extern int imaxarg1, imaxarg2;
extern int iminarg1, iminarg2;

#define SQR(a) ((sqrarg=(a)) == 0.0 ? 0.0 : sqrarg*sqrarg)

#define DSQR(a) ((dsqrarg=(a)) == 0.0 ? 0.0 : dsqrarg*dsqrarg)

#define DMAX(a,b) (dmaxarg1=(a),dmaxarg2=(b),(dmaxarg1) > (dmaxarg2) ?\
        (dmaxarg1) : (dmaxarg2))

#define DMIN(a,b) (dminarg1=(a),dminarg2=(b),(dminarg1) < (dminarg2) ?\
        (dminarg1) : (dminarg2))

#define FMAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
        (maxarg1) : (maxarg2))

#define FMIN(a,b) (minarg1=(a),minarg2=(b),(minarg1) < (minarg2) ?\
        (minarg1) : (minarg2))

#define LMAX(a,b) (lmaxarg1=(a),lmaxarg2=(b),(lmaxarg1) > (lmaxarg2) ?\
        (lmaxarg1) : (lmaxarg2))

#define LMIN(a,b) (lminarg1=(a),lminarg2=(b),(lminarg1) < (lminarg2) ?\
        (lminarg1) : (lminarg2))

#define IMAX(a,b) (imaxarg1=(a),imaxarg2=(b),(imaxarg1) > (imaxarg2) ?\
        (imaxarg1) : (imaxarg2))

#define IMIN(a,b) (iminarg1=(a),iminarg2=(b),(iminarg1) < (iminarg2) ?\
        (iminarg1) : (iminarg2))

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))


float *vector(int nl, int nh);
int *ivector(int nl, int nh);
unsigned char *cvector(int nl, int nh);
unsigned long *lvector(int nl, int nh);
double *dvector(int nl, int nh);
float **matrix(int nrl, int nrh, int ncl, int nch);
double **dmatrix(int nrl, int nrh, int ncl, int nch);
int **imatrix(int nrl, int nrh, int ncl, int nch);
float **submatrix(float **a, int oldrl, int oldrh, int oldcl, int oldch,
		  int newrl, int newcl);
float **convert_matrix(float *a, int nrl, int nrh, int ncl, int nch);
float ***f3tensor(int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
void free_vector(float *v, int nl, int nh);
void free_ivector(int *v, int nl, int nh);
void free_cvector(unsigned char *v, int nl, int nh);
void free_lvector(unsigned long *v, int nl, int nh);
void free_dvector(double *v, int nl, int nh);
void free_matrix(float **m, int nrl, int nrh, int ncl, int nch);
void free_dmatrix(double **m, int nrl, int nrh, int ncl, int nch);
void free_imatrix(int **m, int nrl, int nrh, int ncl, int nch);
void free_submatrix(float **b, int nrl, int nrh, int ncl, int nch);
void free_convert_matrix(float **b, int nrl, int nrh, int ncl, int nch);
void free_f3tensor(float ***t, int nrl, int nrh, int ncl, int nch,
		   int ndl, int ndh);

#endif /* _NR_UTILS_H_ */
