/*      Name:   del2g

   Created:        Tue Mar  5 09:22:27 1985
   Last modified:  Tue May  6 21:21:41 1986

   Purpose:        Take the Laplacian of a gaussian of the image.

   Details:        This routine does a convolution of the Marr-Hildreth operator
   (Laplacian of a gaussian) with the given image, and returns
   the result.  Uses the array processor.  Does the convolution
   in the frequency domain (ie, multiplies the fourier transforms
   of the image and the filter together, and takes the inverse
   transform).

   Author:         Bill Hoff,2-114C,8645,3563478 (hoff) at uicsl
 */


#include <grass/config.h>

#if defined(HAVE_FFTW_H) || defined(HAVE_DFFTW_H) || defined(HAVE_FFTW3_H)

#include <stdio.h>
#include <grass/gmath.h>
#include <grass/gis.h>
#include <grass/glocale.h>


#define FORWARD  1
#define INVERSE -1
#define SCALE    1
#define NOSCALE  0


/*!
 * \fn int del2g (double *img[2], int size, double w)
 *
 * \brief 
 *
 * \param img
 * \param size
 * \param w
 * \return int
 */

int del2g(double *img[2], int size, double w)
{
    double *g[2];		/* the filter function */

    G_message(_("    taking FFT of image..."));
    fft(FORWARD, img, size * size, size, size);

    g[0] = (double *)G_malloc(size * size * sizeof(double));
    g[1] = (double *)G_malloc(size * size * sizeof(double));

    G_message(_("    computing del**2 g..."));
    getg(w, g, size);

    G_message(_("    taking FFT of del**2 g..."));
    fft(FORWARD, g, size * size, size, size);

    /* multiply the complex vectors img and g, each of length size*size */
    G_message(_("    multiplying transforms..."));
    G_math_complex_mult(img, size * size, g, size * size, img, size * size);

    G_message(_("    taking inverse FFT..."));
    fft(INVERSE, img, size * size, size, size);

    return 0;
}

#endif /* HAVE_FFTW */
