#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>

struct GASTATS
{
    double count;
    double min;
    double max;
    double sum;
    double sumsq;
    double sumabs;
    double mean;
    double meanabs;
    double var;
    double stdev;
};

double class_apply_algorithm(char *, double *, int, int *, double *);
int class_interval(double *, int, int, double *);
int class_quant(double *, int, int, double *);
double class_discont(double *, int, int, double *);
double class_stdev(double *, int, int, double *);
int class_equiprob(double *, int, int *, double *);

int class_frequencies(double *, int, int, double *, int *);

void eqdrt(double[], double[], int, int, double *);
void basic_stats(double *, int, struct GASTATS *);
