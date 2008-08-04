#ifndef __CDHC_H__
#define __CDHC_H__


double enormp(double);
double normp(double);
double xinormal(double);
double *dmax(double *, int);
double *dmax_exp(double *, int);
double *omnibus_moments(double *, int);
double *geary_test(double *, int);
double *dagostino_d(double *, int);
double *extreme(double *, int);
double *kuipers_v(double *, int);
double *watson_u2(double *, int);
double *durbins_exact(double *, int);
double *anderson_darling(double *, int);
double *cramer_von_mises(double *, int);
double *kolmogorov_smirnov(double *, int);
double *chi_square(double *, int);
double *shapiro_wilk(double *, int);
double *shapiro_francia(double *, int);
double *weisberg_bingham(double *, int);
double *royston(double *, int);
double *shapiro_wilk_exp(double *, int);
double *kolmogorov_smirnov_exp(double *, int);
double *cramer_von_mises_exp(double *, int);
double *kuipers_v_exp(double *, int);
double *watson_u2_exp(double *, int);
double *anderson_darling_exp(double *, int);
double *chi_square_exp(double *, int);
double *mod_maxlik_ratio(double *, int);
double *coeff_variation(double *, int);
double *kotz_families(double *, int);


#endif /* __CDHC_H__ */
