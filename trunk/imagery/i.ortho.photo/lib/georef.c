#include <stdlib.h>
#include <signal.h>
#include "orthophoto.h"

/* TODO: replace with imagery lib I_compute_georef_equations() */

static int floating_exception;
static void catch(int);
static double determinant(double, double, double, double, double,
			  double, double, double, double);

/* find coefficients A,B,C for e2 = A + B*e1 + C*n1
 * also compute the reverse equations
 *
 * return 0 if no points
 *       -1 if not solvable
 *        1 if ok
 *
 * method is least squares.
 * the least squares problem reduces to solving the following
 * system of equations for A,B,C
 *
 *   s0*A + s1*B + s2*C = x0
 *   s1*A + s3*B + s4*C = x1
 *   s2*A + s4*B + s5*C = x2
 *
 * use Cramer's rule
 *
 *     | x0 s1 s2 |      | s0 x0 s2 |      | s0 s1 x0 |
 *     | x1 s3 s4 |      | s1 x1 s4 |      | s1 s3 x1 |
 *     | x2 s4 s5 |      | s2 x2 s5 |      | s2 s4 x2 |
 * A = ------------  B = ------------  C = ------------ 
 *     | s0 s1 s2 |      | s0 s1 s2 |      | s0 s1 s2 |
 *     | s1 s3 s4 |      | s1 s3 s4 |      | s1 s3 s4 |
 *     | s2 s4 s5 |      | s2 s4 s5 |      | s2 s4 s5 |
 *
 */

int
I_compute_ref_equations(struct Ortho_Photo_Points *cp, double E12[3],
			double N12[3], double E21[3], double N21[3])
{
    double s0, s1, s2, s3, s4, s5;
    double x0, x1, x2;
    double det;
    void (*sigfpe) ();
    int i;


    s0 = s1 = s2 = s3 = s4 = s5 = 0.0;
    for (i = 0; i < cp->count; i++) {
	if (cp->status[i] <= 0)
	    continue;
	s0 += 1.0;
	s1 += cp->e1[i];
	s2 += cp->n1[i];
	s3 += cp->e1[i] * cp->e1[i];
	s4 += cp->e1[i] * cp->n1[i];
	s5 += cp->n1[i] * cp->n1[i];
    }
    if (s0 < 0.5)
	return 0;

    floating_exception = 0;
    sigfpe = signal(SIGFPE, catch);

    /* eastings */
    x0 = x1 = x2 = 0.0;
    for (i = 0; i < cp->count; i++) {
	if (cp->status[i] <= 0)
	    continue;
	x0 += cp->e2[i];
	x1 += cp->e1[i] * cp->e2[i];
	x2 += cp->n1[i] * cp->e2[i];
    }

    det = determinant(s0, s1, s2, s1, s3, s4, s2, s4, s5);
    if (det == 0.0) {
	signal(SIGFPE, sigfpe);
	return -1;
    }
    E12[0] = determinant(x0, s1, s2, x1, s3, s4, x2, s4, s5) / det;
    E12[1] = determinant(s0, x0, s2, s1, x1, s4, s2, x2, s5) / det;
    E12[2] = determinant(s0, s1, x0, s1, s3, x1, s2, s4, x2) / det;

    /* northings */
    x0 = x1 = x2 = 0.0;
    for (i = 0; i < cp->count; i++) {
	if (cp->status[i] <= 0)
	    continue;
	x0 += cp->n2[i];
	x1 += cp->e1[i] * cp->n2[i];
	x2 += cp->n1[i] * cp->n2[i];
    }

    det = determinant(s0, s1, s2, s1, s3, s4, s2, s4, s5);
    if (det == 0.0) {
	signal(SIGFPE, sigfpe);
	return -1;
    }
    N12[0] = determinant(x0, s1, s2, x1, s3, s4, x2, s4, s5) / det;
    N12[1] = determinant(s0, x0, s2, s1, x1, s4, s2, x2, s5) / det;
    N12[2] = determinant(s0, s1, x0, s1, s3, x1, s2, s4, x2) / det;

    /* the inverse equations */

    s0 = s1 = s2 = s3 = s4 = s5 = 0.0;
    for (i = 0; i < cp->count; i++) {
	if (cp->status[i] <= 0)
	    continue;
	s0 += 1.0;
	s1 += cp->e2[i];
	s2 += cp->n2[i];
	s3 += cp->e2[i] * cp->e2[i];
	s4 += cp->e2[i] * cp->n2[i];
	s5 += cp->n2[i] * cp->n2[i];
    }

    /* eastings */
    x0 = x1 = x2 = 0.0;
    for (i = 0; i < cp->count; i++) {
	if (cp->status[i] <= 0)
	    continue;
	x0 += cp->e1[i];
	x1 += cp->e2[i] * cp->e1[i];
	x2 += cp->n2[i] * cp->e1[i];
    }

    det = determinant(s0, s1, s2, s1, s3, s4, s2, s4, s5);
    if (det == 0.0) {
	signal(SIGFPE, sigfpe);
	return -1;
    }
    E21[0] = determinant(x0, s1, s2, x1, s3, s4, x2, s4, s5) / det;
    E21[1] = determinant(s0, x0, s2, s1, x1, s4, s2, x2, s5) / det;
    E21[2] = determinant(s0, s1, x0, s1, s3, x1, s2, s4, x2) / det;

    /* northings */
    x0 = x1 = x2 = 0.0;
    for (i = 0; i < cp->count; i++) {
	if (cp->status[i] <= 0)
	    continue;
	x0 += cp->n1[i];
	x1 += cp->e2[i] * cp->n1[i];
	x2 += cp->n2[i] * cp->n1[i];
    }

    det = determinant(s0, s1, s2, s1, s3, s4, s2, s4, s5);
    if (det == 0.0) {
	signal(SIGFPE, sigfpe);
	return -1;
    }
    N21[0] = determinant(x0, s1, s2, x1, s3, s4, x2, s4, s5) / det;
    N21[1] = determinant(s0, x0, s2, s1, x1, s4, s2, x2, s5) / det;
    N21[2] = determinant(s0, s1, x0, s1, s3, x1, s2, s4, x2) / det;

    signal(SIGFPE, sigfpe);
    return floating_exception ? -1 : 1;
}

static double determinant(double a, double b, double c, double d,
			  double e, double f, double g, double h, double i)
{
    /* compute determinant of 3x3 matrix
     *     | a b c |
     *     | d e f |
     *     | g h i |
     */
    return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
}

static void catch(int n)
{
    floating_exception = 1;
    signal(n, catch);
}
