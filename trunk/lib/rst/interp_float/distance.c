
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993 
 * US Army Construction Engineering Research Lab 
 * Copyright 1993, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL) 
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995 
 *
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/interpf.h>

double IL_dist_square(double *pt1, double *pt2, int dim)
{
    int i;
    double sum = 0, s;

    for (i = 0; i < dim; i++) {
	s = pt1[i] - pt2[i];
	sum += s * s;
    }
    return sum;
}
