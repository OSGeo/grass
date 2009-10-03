/* Author: Bill Hoff,2-114C,8645,3563478 (hoff) at uicsl */

/*!
 * \fn int G_math_complex_mult (double *v1[2], int size1, double *v2[2], int size2, double *v3[2], int size3)
 *
 * \brief Multiply two complex vectors, point by point
 *
 * Vectors are in the form: real, imaginary (each a floating number).  
 * A vector can be of any size. Computes <b>v3</b> = <b>v1</b> * 
 * <b>v2</b>.  <b>v3</b> should as big as the biggest of <b>v1</b> and 
 * <b>v2</b>.
 *
 * \param v1
 * \param size1
 * \param v2
 * \param size2
 * \param v3
 * \param size3
 * \return int
 */

int
G_math_complex_mult(double *v1[2], int size1, double *v2[2], int size2, double *v3[2],
     int size3)
{
    int i, n;

    n = (size1 < size2 ? size1 : size2);	/* get the smaller size */
    for (i = 0; i < n; i++) {
	*(v3[0] + i) =
	    *(v1[0] + i) * *(v2[0] + i) - *(v1[1] + i) * *(v2[1] + i);
	*(v3[1] + i) =
	    *(v1[0] + i) * *(v2[1] + i) + *(v2[0] + i) * *(v1[1] + i);
    }

    /* if unequal size, zero out remaining elements of larger vector */
    if (size1 != size2)
	for (i = n; i < size3; i++) {
	    *(v3[0] + i) = 0.0;
	    *(v3[1] + i) = 0.0;
	}

    return 0;
}
