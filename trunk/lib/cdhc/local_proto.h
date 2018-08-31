#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__


/* qsort() comparison function */
int Cdhc_dcmp(const void *, const void *);

double *Cdhc_dmax(double *, int);
double *Cdhc_dmax_exp(double *, int);

/* misc internal support functions */
void wcoef(double[], int, int, double *, int *);
void Cdhc_wgp(double[], int, double, double, double, double[],
	 int, double, double, double, double, int *);
void Cdhc_nscor2(double[], int, int, int *);
void wext(double[], int, double, double[], int, double,
	  double *, double *, int *);

double Cdhc_alnorm(double, int);
double Cdhc_enormp(double);
double Cdhc_normp(double);
double Cdhc_xinormal(double);

double ppn7(double);
double ppnd16(double);

double *Cdhc_anderson_darling(double *, int);
double *Cdhc_chi_square(double *, int);
double *Cdhc_cramer_von_mises(double *, int);
double *Cdhc_dagostino_d(double *, int);
double *Cdhc_durbins_exact(double *, int);
double *Cdhc_extreme(double *, int);
double *Cdhc_geary_test(double *, int);
double *Cdhc_kolmogorov_smirnov(double *, int);
double *Cdhc_kotz_families(double *, int);
double *Cdhc_kuipers_v(double *, int);
double *Cdhc_omnibus_moments(double *, int);
double *Cdhc_royston(double *, int);
double *Cdhc_shapiro_wilk(double *, int);
double *Cdhc_watson_u2(double *, int);
double *Cdhc_weisberg_bingham(double *, int);



#endif /* __LOCAL_PROTO_H__ */
