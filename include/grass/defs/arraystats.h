#ifndef GRASS_ARRAYSTATSDEFS_H
#define GRASS_ARRAYSTATSDEFS_H

/* basic.c */
void AS_eqdrt(double[], double[], int, int, double *, double *, double *);
void AS_basic_stats(const double[], int, struct GASTATS *);

/* class.c */
int AS_option_to_algorithm(const struct Option *);
double AS_class_apply_algorithm(int, const double[], int, int *, double[]);
int AS_class_interval(const double[], int, int, double[]);
int AS_class_quant(const double[], int, int, double[]);
double AS_class_discont(const double[], int, int, double[]);
double AS_class_stdev(const double[], int, int, double[]);
int AS_class_equiprob(const double[], int, int *, double[]);
int AS_class_frequencies(const double[], int, int, double[], int[]);

#endif
