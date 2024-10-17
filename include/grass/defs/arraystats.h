#ifndef GRASS_ARRAYSTATSDEFS_H
#define GRASS_ARRAYSTATSDEFS_H

/* basic.c */
void AS_eqdrt(double[], double[], int, int, double *);
void AS_basic_stats(double *, int, struct GASTATS *);

/* class.c */
int AS_option_to_algorithm(const struct Option *);
double AS_class_apply_algorithm(int, double *, int, int *, double *);
int AS_class_interval(double *, int, int, double *);
int AS_class_quant(double *, int, int, double *);
double AS_class_discont(double *, int, int, double *);
double AS_class_stdev(double *, int, int, double *);
int AS_class_equiprob(double *, int, int *, double *);
int AS_class_frequencies(double *, int, int, double *, int *);

#endif
