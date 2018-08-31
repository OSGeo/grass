
/**
 * \file findzc.c
 *
 * \brief Zero Crossing functions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * \author GRASS GIS Development Team
 * \author Brad Douglas - rez at touchofmadness com
 *
 * \date 2006
 */

#include <stdio.h>
#include <math.h>


/** \def TINY Defined as 1.0e-3 */
#define TINY    1.0e-3


/**
 * \fn int G_math_findzc (double conv[], int size, double zc[], double thresh, int num_orients)
 *
 * \brief Finds locations and orientations of zero crossings.
 *
 * Finds the locations and orientations of zero crossings in the input 
 * array <b>conv</b>, which is the result of the convolution of the
 * Marr-Hildreth operator with the image. The output array is <b>zc</b>, 
 * which is non-zero only at zero crossing pixels. At those pixels, the
 * value is 1 + (orientation), where orientation is a value from 0 to 
 * <b>num_orients</b>.
 *
 * \param[in] conv input
 * \param[in] size size of largest matrix column or row
 * \param[out] zc output
 * \param[in] thresh magnitude threshold
 * \param[in] num_orients
 * \return int always returns 0
 */

int
G_math_findzc(double conv[], int size, double zc[], double thresh,
	      int num_orients)
{
    int i, j, p;

    /* go through entire conv image - but skip border rows and cols */
    for (i = 1; i < size - 1; i++) {
	for (p = i * size + 1, j = 1; j < size - 1; j++, p++) {
	    int nbr[4];
	    int ni;

	    /* examine the 4-neighbors of position p */
	    nbr[0] = p - 1;	/* left */
	    nbr[1] = p + 1;	/* right */
	    nbr[2] = p - size;	/* up */
	    nbr[3] = p + size;	/* down */

	    zc[p] = 0;

	    for (ni = 0; ni < 4; ni++) {
		/* condition for a zc: sign is different than a neighbor
		 * and the absolute value is less than that neighbor.
		 * Also, threshold magnitudes to eliminate noise
		 */
		if ((((conv[p] > 0) && (conv[nbr[ni]] < 0)) ||
		     ((conv[p] < 0) && (conv[nbr[ni]] > 0))) &&
		    (fabs(conv[p]) < fabs(conv[nbr[ni]])) &&
		    (fabs(conv[p] - conv[nbr[ni]]) > thresh)) {
		    double ang;
		    int dir;

		    /* found a zc here, get angle of gradient */
		    if (fabs(conv[nbr[1]] - conv[nbr[0]]) < TINY) {
			ang = M_PI_2;

			if (conv[nbr[2]] - conv[nbr[3]] < 0)
			    ang = -ang;
		    }
		    else
			ang = atan2(conv[nbr[2]] - conv[nbr[3]],
				    conv[nbr[1]] - conv[nbr[0]]);

		    /* scale -PI..PI to 0..num_orients - 1 */
		    dir =
			num_orients * ((ang + M_PI) / (M_PI * 2.0)) + 0.4999;

		    /* shift scale so that 0 (not 8) is straight down */
		    dir = (3 * num_orients / 4 + dir) % num_orients;

		    /* add to differentiate between no zc and an orientation */
		    zc[p] = 1 + dir;
		    break;	/* quit looking at neighbors */
		}
	    }			/* for ni */
	}			/* for p */
    }

    return 0;
}
