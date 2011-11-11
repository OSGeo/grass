#ifndef GRASS_ARRAYSTATSDEFS_H
#define GRASS_ARRAYSTATSDEFS_H

double class_apply_algorithm(char *, double *, int, int *, double *);
int class_interval(double *, int, int, double *);
int class_quant(double *, int, int, double *);
double class_discont(double *, int, int, double *);
double class_stdev(double *, int, int, double *);
int class_equiprob(double *, int, int *, double *);

int class_frequencies(double *, int, int, double *, int *);

void eqdrt(double[], double[], int, int, double *);
void basic_stats(double *, int, struct GASTATS *);

#endif
