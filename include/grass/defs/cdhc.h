#ifndef CDHCDEFS_H
#define CDHCDEFS_H

double Cdhc_enormp(double);
double Cdhc_normp(double);
double Cdhc_xinormal(double);
double *Cdhc_dmax(double *, int);
double *Cdhc_dmax_exp(double *, int);
double *Cdhc_omnibus_moments(double *, int);
double *Cdhc_geary_test(double *, int);
double *Cdhc_dagostino_d(double *, int);
double *Cdhc_extreme(double *, int);
double *Cdhc_kuipers_v(double *, int);
double *Cdhc_watson_u2(double *, int);
double *Cdhc_durbins_exact(double *, int);
double *Cdhc_anderson_darling(double *, int);
double *Cdhc_cramer_von_mises(double *, int);
double *Cdhc_kolmogorov_smirnov(double *, int);
double *Cdhc_chi_square(double *, int);
double *Cdhc_shapiro_wilk(double *, int);
double *Cdhc_shapiro_francia(double *, int);
double *Cdhc_weisberg_bingham(double *, int);
double *Cdhc_royston(double *, int);
double *Cdhc_shapiro_wilk_exp(double *, int);
double *Cdhc_kolmogorov_smirnov_exp(double *, int);
double *Cdhc_cramer_von_mises_exp(double *, int);
double *Cdhc_kuipers_v_exp(double *, int);
double *Cdhc_watson_u2_exp(double *, int);
double *Cdhc_anderson_darling_exp(double *, int);
double *Cdhc_chi_square_exp(double *, int);
double *Cdhc_mod_maxlik_ratio(double *, int);
double *Cdhc_coeff_variation(double *, int);
double *Cdhc_kotz_families(double *, int);

#endif /* CDHCDEFS_H */
