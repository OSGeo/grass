/*  
    Copyright 2003,2004 Rodrigo Caballero Augi
    rca@geosci.uchicago.edu
    Department of the Geophysical Sciences, University of Chicago
    2003

    This file is part of the Numeric Pointer Module.

    The Numeric Pointer Module is free software; you can redistribute it 
    and/or modify it under the terms of the GNU General Public License as 
    published by the Free Software Foundation; either version 2 of the License,
    or (at your option) any later version.

    The Numeric Pointer Module is distributed in the hope that it will be 
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Numeric Pointer Module; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

%module NumPtr

%{
#include "getpointer.h"
#include "test.h"
%}

double *   getpointer1(PyObject *array);
double **  getpointer2(PyObject *array);
double *** getpointer3(PyObject *array);

double *   getdpointer1(PyObject *array);
double **  getdpointer2(PyObject *array);
double *** getdpointer3(PyObject *array);

float *   getfpointer1(PyObject *array);
float **  getfpointer2(PyObject *array);
float *** getfpointer3(PyObject *array);

int *   getipointer1(PyObject *array);
int **  getipointer2(PyObject *array);
int *** getipointer3(PyObject *array);

void  test1(double *   a, int n);
void  test2(double **  a, int n, int m);
void  test3(double *** a, int n, int m, int l);

void  testd1(double *   a, int n);
void  testd2(double **  a, int n, int m);
void  testd3(double *** a, int n, int m, int l);

void  testf1(float *   a, int n);
void  testf2(float **  a, int n, int m);
void  testf3(float *** a, int n, int m, int l);

void  testi1(int *   a, int n);
void  testi2(int **  a, int n, int m);
void  testi3(int *** a, int n, int m, int l);
