/*!
   \file exag.c

   \brief Nviz library -- Exaggeration functions

   COPYRIGHT: (C) 2008 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   Based on visualization/nviz/src/exag.c

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008)

   \date 2008
 */

#include <grass/nviz.h>

/*!
   \brief Get view height

   Call after initial data has been loaded

   \param[out] val height value
   \param[out] min min value (or NULL)
   \param[out] max max value (or NULL)

   \return 1
 */
int Nviz_get_exag_height(float *val, float *min, float *max)
{
    float longdim, exag, texag, hmin, hmax, fmin, fmax;
    int nsurfs, i, *surf_list;

    surf_list = GS_get_surf_list(&nsurfs);
    if (nsurfs) {
	GS_get_longdim(&longdim);
	GS_get_zrange_nz(&hmin, &hmax);

	exag = 0.0;
	for (i = 0; i < nsurfs; i++) {
	    if (GS_get_exag_guess(surf_list[i], &texag) > -1)
		if (texag)
		    exag = texag > exag ? texag : exag;
	}
	if (exag == 0.0)
	    exag = 1.0;

	fmin = hmin - (2. * longdim / exag);
	fmax = hmin + (3 * longdim / exag);
    }
    else {
	fmax = 10000.0;
	fmin = 0.0;
    }

    *val = fmin + (fmax - fmin) / 2.0;

    if (min)
	*min = fmin;

    if (max)
	*max = fmax;

    return 1;
}

/*!
   \brief Get view z-exag value

   Call after initial data has been loaded

   \return value
 */
float Nviz_get_exag()
{
    float exag, texag;
    int nsurfs, i, *surf_list;

    surf_list = GS_get_surf_list(&nsurfs);

    exag = 0.0;
    for (i = 0; i < nsurfs; i++) {
	if (GS_get_exag_guess(surf_list[i], &texag) > -1) {
	    if (texag)
		exag = (texag > exag) ? texag : exag;
	}
    }

    if (exag == 0.0)
	exag = 1.0;

    if (nsurfs > 0)
	G_free(surf_list);

    return exag;
}
