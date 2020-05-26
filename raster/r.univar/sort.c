/*
 *   Copyright (C) 2004-2007 by the GRASS Development Team
 *   Author(s): 1998, 1999 Sebastian Cyris
 *              2007 modified by Soeren Gebbert
 *
 *      This program is free software under the GNU General Public
 *      License (>=v2). Read the file COPYING that comes with GRASS
 *      for details.
 *
 */

#include "globals.h"
static void downheap_int(int *array, size_t n, size_t k);
static void downheap_float(float *array, size_t n, size_t k);
static void downheap_double(double *array, size_t n, size_t k);

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
void downheap_int(int *array, size_t n, size_t k)
{
    size_t j;
    int v;

    v = array[k];
    while (k <= n / 2) {
	j = k + k;

	if (j < n && array[j] < array[j + 1])
	    j++;
	if (v >= array[j])
	    break;

	array[k] = array[j];
	k = j;
    }

    array[k] = v;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
void downheap_float(float *array, size_t n, size_t k)
{
    size_t j;
    float v;

    v = array[k];
    while (k <= n / 2) {
	j = k + k;

	if (j < n && array[j] < array[j + 1])
	    j++;
	if (v >= array[j])
	    break;

	array[k] = array[j];
	k = j;
    }

    array[k] = v;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
void downheap_double(double *array, size_t n, size_t k)
{
    size_t j;
    double v;

    v = array[k];
    while (k <= n / 2) {
	j = k + k;

	if (j < n && array[j] < array[j + 1])
	    j++;
	if (v >= array[j])
	    break;

	array[k] = array[j];
	k = j;
    }

    array[k] = v;
}

/* *************************************************************** */
/* ****** heapsort for int arrays of size n ********************** */
/* *************************************************************** */
void heapsort_int(int *array, size_t n)
{
    ssize_t k;
    int t;

    --n;

    for (k = n / 2; k >= 0; k--)
	downheap_int(array, n, k);

    while (n > 0) {
	t = array[0];
	array[0] = array[n];
	array[n] = t;
	downheap_int(array, --n, 0);
    }
    return;
}


/* *************************************************************** */
/* ****** heapsort for float arrays of size n ******************** */
/* *************************************************************** */
void heapsort_float(float *array, size_t n)
{
    ssize_t k;
    float t;

    --n;

    for (k = n / 2; k >= 0; k--)
	downheap_float(array, n, k);

    while (n > 0) {
	t = array[0];
	array[0] = array[n];
	array[n] = t;
	downheap_float(array, --n, 0);
    }
    return;
}

/* *************************************************************** */
/* ****** heapsort for double arrays of size n ******************* */
/* *************************************************************** */
void heapsort_double(double *array, size_t n)
{
    ssize_t k;
    double t;

    --n;

    for (k = n / 2; k >= 0; k--)
	downheap_double(array, n, k);

    while (n > 0) {
	t = array[0];
	array[0] = array[n];
	array[n] = t;
	downheap_double(array, --n, 0);
    }
    return;
}
