#include "Python.h"
#include "Numeric/arrayobject.h"
#include "getpointer.h"

double *getpointer1(PyObject * A)
{
    PyArrayObject *array;

    array = (PyArrayObject *) A;
    return (double *)(array->data);
}

double **getpointer2(PyObject * A)
{
    PyArrayObject *array;
    int imax, i;
    double **p = NULL;

    array = (PyArrayObject *) A;
    imax = array->dimensions[0];
    p = (double **)calloc(imax, sizeof(double *));
    for (i = 0; i < imax; i++) {
	p[i] = (double *)(array->data + i * array->strides[0]);
    }
    return p;
}

double ***getpointer3(PyObject * A)
{
    PyArrayObject *array;
    int imax, jmax, i, j;
    double ***p = NULL;

    array = (PyArrayObject *) A;
    imax = array->dimensions[0];
    jmax = array->dimensions[1];
    p = (double ***)calloc(imax, sizeof(double **));
    for (i = 0; i < imax; i++) {
	p[i] = (double **)calloc(jmax, sizeof(double *));
	for (j = 0; j < jmax; j++) {
	    p[i][j] =
		(double *)(array->data + i * array->strides[0] +
			   j * array->strides[1]);
	}
    }
    return p;
}

double *getdpointer1(PyObject * A)
{
    PyArrayObject *array;

    array = (PyArrayObject *) A;
    return (double *)(array->data);
}

double **getdpointer2(PyObject * A)
{
    PyArrayObject *array;
    int imax, i;
    double **p = NULL;

    array = (PyArrayObject *) A;
    imax = array->dimensions[0];
    p = (double **)calloc(imax, sizeof(double *));
    for (i = 0; i < imax; i++) {
	p[i] = (double *)(array->data + i * array->strides[0]);
    }
    return p;
}

double ***getdpointer3(PyObject * A)
{
    PyArrayObject *array;
    int imax, jmax, i, j;
    double ***p = NULL;

    array = (PyArrayObject *) A;
    imax = array->dimensions[0];
    jmax = array->dimensions[1];
    p = (double ***)calloc(imax, sizeof(double **));
    for (i = 0; i < imax; i++) {
	p[i] = (double **)calloc(jmax, sizeof(double *));
	for (j = 0; j < jmax; j++) {
	    p[i][j] =
		(double *)(array->data + i * array->strides[0] +
			   j * array->strides[1]);
	}
    }
    return p;
}

float *getfpointer1(PyObject * A)
{
    PyArrayObject *array;

    array = (PyArrayObject *) A;
    return (float *)(array->data);
}

float **getfpointer2(PyObject * A)
{
    PyArrayObject *array;
    int imax, i;
    float **p = NULL;

    array = (PyArrayObject *) A;
    imax = array->dimensions[0];
    p = (float **)calloc(imax, sizeof(float *));
    for (i = 0; i < imax; i++) {
	p[i] = (float *)(array->data + i * array->strides[0]);
    }
    return p;
}

float ***getfpointer3(PyObject * A)
{
    PyArrayObject *array;
    int imax, jmax, i, j;
    float ***p = NULL;

    array = (PyArrayObject *) A;
    imax = array->dimensions[0];
    jmax = array->dimensions[1];
    p = (float ***)calloc(imax, sizeof(float **));
    for (i = 0; i < imax; i++) {
	p[i] = (float **)calloc(jmax, sizeof(float *));
	for (j = 0; j < jmax; j++) {
	    p[i][j] =
		(float *)(array->data + i * array->strides[0] +
			  j * array->strides[1]);
	}
    }
    return p;
}

int *getipointer1(PyObject * A)
{
    PyArrayObject *array;

    array = (PyArrayObject *) A;
    return (int *)(array->data);
}

int **getipointer2(PyObject * A)
{
    PyArrayObject *array;
    int imax, i;
    int **p = NULL;

    array = (PyArrayObject *) A;
    imax = array->dimensions[0];
    p = (int **)calloc(imax, sizeof(int *));
    for (i = 0; i < imax; i++) {
	p[i] = (int *)(array->data + i * array->strides[0]);
    }
    return p;
}

int ***getipointer3(PyObject * A)
{
    PyArrayObject *array;
    int imax, jmax, i, j;
    int ***p = NULL;

    array = (PyArrayObject *) A;
    imax = array->dimensions[0];
    jmax = array->dimensions[1];
    p = (int ***)calloc(imax, sizeof(int **));
    for (i = 0; i < imax; i++) {
	p[i] = (int **)calloc(jmax, sizeof(int *));
	for (j = 0; j < jmax; j++) {
	    p[i][j] =
		(int *)(array->data + i * array->strides[0] +
			j * array->strides[1]);
	}
    }
    return p;
}
