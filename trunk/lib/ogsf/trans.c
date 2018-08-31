/*!
   \file lib/ogsf/trans.c

   \brief OGSF library - matrix transformation (higher level functions)

   GRASS OpenGL gsurf OGSF Library 

   NOTE: This file should be REMOVED and any calls to the functions in this
   file should be replaced with appropriate OpenGL calls.

   This routine should be available in GL!

   Arguments are same as GL counterparts

   I threw this code together in January at the beginning of this
   class.  I was still learning about GL at the time.
   There are many places where the code could be improved.

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Dave Gerdes Jan 1990 All rights reserved, US Army Construction Engineering Research Lab
   \author Bill Brown USACERL (November 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/ogsf.h>

#define MAX_STACK 20


/* function prototypes */
static void P__transform(int num_vert, float (*in)[4],
			 float (*out)[4], float (*c)[4]);
static void P_matrix_copy(float (*from)[4], float (*to)[4], int size);


/* global variables */
static float c_stack[MAX_STACK][4][4];	/* matrix stack */
static int stack_ptr = -1;	/* index of curr matrix depth */
static float d[4][4];		/* tmp matrix */

#define NPI M_PI

/*
 **  Current transformation matrix
 */
static float trans_mat[4][4] = {
    {1., 0., 0., 0.},
    {0., 1., 0., 0.},
    {0., 0., 1., 0.},
    {0., 0., 0., 1.}
};

static float ident[4][4] = {
    {1., 0., 0., 0.},
    {0., 1., 0., 0.},
    {0., 0., 1., 0.},
    {0., 0., 0., 1.}
};

/*!
   \brief ADD

   \param x,y,z
 */
void P_scale(float x, float y, float z)
{
    d[0][0] = x;
    d[0][1] = 0.;
    d[0][2] = 0.;
    d[0][3] = 0.;
    d[1][0] = 0.;
    d[1][1] = y;
    d[1][2] = 0.;
    d[1][3] = 0.;
    d[2][0] = 0.;
    d[2][1] = 0.;
    d[2][2] = z;
    d[2][3] = 0.;
    d[3][0] = 0.;
    d[3][1] = 0.;
    d[3][2] = 0.;
    d[3][3] = 1.;

    /*
     **  will write into 1 down on matrix stack
     **  and then the popmatrix() will place it as the current T matrix
     */
    P_pushmatrix();
    P__transform(4, d, c_stack[stack_ptr], trans_mat);
    P_popmatrix();

    return;
}

/*!
   \brief Transform array of vectors using current T matrix

   Multiply 'in' matrix (homogenous coordinate generally) by
   the current transformation matrix, placing the result in 'out'

   [in][trans_mat] => [out]

   \param num_vert
   \param in
   \param out
 */
void P_transform(int num_vert, float (*in)[4], float (*out)[4])
{
    P__transform(num_vert, in, out, trans_mat);

    return;
}

/*!
   \brief Transform array of vectors using current T matrix

   Multiply 'in' matrix (homogenous coordinate generally) by
   the current transformation matrix, placing the result in 'out'

   [in][trans_mat] => [out]

   \param num_vert
   \param in
   \param out
 */
static void P__transform(int num_vert, float (*in)[4], float (*out)[4],
			 float (*c)[4])
{
    register int k, j, i;

    for (i = 0; i < num_vert; i++) {
	for (j = 0; j < 4; j++) {
	    out[i][j] = 0.;

	    for (k = 0; k < 4; k++) {
		out[i][j] += in[i][k] * c[k][j];
	    }
	}
    }

    return;
}

/*!
   \brief Copy matrix 

   \param from 'from' matrix
   \param to 'to' matrix
   \param size number of rows (ncols=4)
 */
static void P_matrix_copy(float (*from)[4], float (*to)[4], int size)
{
    register int i, j;

    for (i = 0; i < size; i++) {
	for (j = 0; j < 4; j++) {
	    to[i][j] = from[i][j];
	}
    }

    return;
}

/*!
   \brief Push current transformation matrix onto matrix stack
 */
int P_pushmatrix(void)
{
    if (stack_ptr >= MAX_STACK) {
	G_warning("P_pushmatrix(): %s", _("Out of matrix stack space"));

	return (-1);
    }

    stack_ptr++;
    P_matrix_copy(trans_mat, c_stack[stack_ptr], 4);

    return (0);
}

/*!
   \brief Pop top of matrix stack, placing it into the current transformation matrix

   \return -1 on failure
   \return 0 on success
 */
int P_popmatrix(void)
{
    if (stack_ptr < 0) {
	G_warning("P_popmatrix(): %s", _("Tried to pop an empty stack"));

	return (-1);
    }

    P_matrix_copy(c_stack[stack_ptr], trans_mat, 4);
    stack_ptr--;

    return (0);
}

/*!
   \brief Rotate matrix

   \param angle angle value
   \param axis ('x, 'y', 'z')
 */
void P_rot(float angle, char axis)
{
    double theta;

    P_matrix_copy(ident, d, 4);

    theta = (NPI / 180.) * angle;	/* convert to radians */

    /* optimize to handle rotations of mutliples of 90 deg */
    switch (axis) {
    case 'X':
    case 'x':

	d[1][1] = cos(theta);
	d[1][2] = sin(theta);
	d[2][1] = -sin(theta);
	d[2][2] = cos(theta);

	break;
    case 'Y':
    case 'y':

	d[0][0] = cos(theta);
	d[0][2] = -sin(theta);
	d[2][0] = sin(theta);
	d[2][2] = cos(theta);
	break;
    case 'Z':
    case 'z':

	d[0][0] = cos(theta);
	d[0][1] = sin(theta);
	d[1][0] = -sin(theta);
	d[1][1] = cos(theta);

	break;
    }

    P_pushmatrix();
    P__transform(4, d, c_stack[stack_ptr], trans_mat);
    P_popmatrix();

    return;
}
