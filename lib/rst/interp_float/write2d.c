/*!
 * \file secpar2d.c
 *
 * \author H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993 (original authors)
 * \author modified by McCauley in August 1995
 * \author modified by Mitasova in August 1995
 * \author H. Mitasova (University of Illinois)
 * \author I. Kosinovsky, (USA-CERL)
 * \author D.Gerdes (USA-CERL)   
 *
 * \copyright
 * (C) 1993-1995 by Helena Mitasova and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS for details.
 */


#include <grass/config.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/interpf.h>


/* parameter descriptions takes from a strange comment */
/*!
 * Writes az,adx,...,adxy into appropriate place (depending on ngstc, nszc
 * and offset) in corresponding temp file
 */
int IL_write_temp_2d(struct interp_params *params,
                     int ngstc,  /*!< begin. column */
                     int nszc,  /*!<  end. column */
                     off_t offset2  /*!< offset */
                     )
{
    int j;
    static FCELL *array_cell = NULL;

    if (!array_cell)
	array_cell = G_malloc(sizeof(FCELL) * params->nsizc + 1);
    if (params->Tmp_fd_z != NULL) {
	for (j = ngstc; j <= nszc; j++)
	    array_cell[j - 1] = (FCELL) params->az[j];
	G_fseek(params->Tmp_fd_z, offset2, SEEK_SET);
	if (!fwrite(array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
		    params->Tmp_fd_z))
	    G_fatal_error(_("Cannot write files"));
    }
    if (params->Tmp_fd_dx != NULL) {
	for (j = ngstc; j <= nszc; j++)
	    if (!params->deriv)
		array_cell[j - 1] = (FCELL) params->adx[j];
	    else
		array_cell[j - 1] = (FCELL) (params->adx[j] * params->scik1);
	G_fseek(params->Tmp_fd_dx, offset2, SEEK_SET);
	if (!fwrite(array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
		    params->Tmp_fd_dx))
	    G_fatal_error(_("Cannot write files"));
    }
    if (params->Tmp_fd_dy != NULL) {
	for (j = ngstc; j <= nszc; j++) {
	    if (!params->deriv) {
		if (params->ady[j] > 0. && params->ady[j] < 0.5)
		    params->ady[j] = 360.;
		array_cell[j - 1] = (FCELL) params->ady[j];
	    }
	    else
		array_cell[j - 1] = (FCELL) (params->ady[j] * params->scik1);
	}
	G_fseek(params->Tmp_fd_dy, offset2, SEEK_SET);
	if (!fwrite(array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
		    params->Tmp_fd_dy))
	    G_fatal_error(_("Cannot write files"));
    }
    if (params->Tmp_fd_xx != NULL) {
	for (j = ngstc; j <= nszc; j++) {
	    array_cell[j - 1] = (FCELL) (params->adxx[j] * params->scik1);
	}
	G_fseek(params->Tmp_fd_xx, offset2, SEEK_SET);
	if (!fwrite(array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
		    params->Tmp_fd_xx))
	    G_fatal_error(_("Cannot write files"));
    }
    if (params->Tmp_fd_yy != NULL) {
	for (j = ngstc; j <= nszc; j++)
	    array_cell[j - 1] = (FCELL) (params->adyy[j] * params->scik2);
	G_fseek(params->Tmp_fd_yy, offset2, SEEK_SET);
	if (!fwrite(array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
		    params->Tmp_fd_yy))
	    G_fatal_error(_("Cannot write files"));
    }
    if (params->Tmp_fd_xy != NULL) {
	for (j = ngstc; j <= nszc; j++)
	    array_cell[j - 1] = (FCELL) (params->adxy[j] * params->scik3);
	G_fseek(params->Tmp_fd_xy, offset2, SEEK_SET);
	if (!fwrite(array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
		    params->Tmp_fd_xy))
	    G_fatal_error(_("Cannot write files"));
    }
    return 1;
}
