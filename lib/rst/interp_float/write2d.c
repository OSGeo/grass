
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993
 * University of Illinois
 * US Army Construction Engineering Research Lab  
 * Copyright 1993, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)   
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995  
 *
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>

#include <grass/interpf.h>

int IL_write_temp_2d(struct interp_params *params, int ngstc, int nszc, int offset2	/* begin. and end. column, offset */
    )

/*
 * Writes az,adx,...,adxy into appropriate place (depending on ngstc, nszc
 * and offset) in corresponding temp file
 */
{
    int j;
    static FCELL *array_cell = NULL;

    if (!array_cell) {

	if (!
	    (array_cell =
	     (FCELL *) G_malloc(sizeof(FCELL) * params->nsizc + 1))) {
	    fprintf(stderr, "Cannot allocate memory for array_cell\n");
	    return -1;
	}
    }
    if (params->Tmp_fd_z != NULL) {
	for (j = ngstc; j <= nszc; j++)
	    array_cell[j - 1] = (FCELL) params->az[j];
	if (fseek(params->Tmp_fd_z, (long)offset2, 0) == -1) {
	    fprintf(stderr, "Cannot fseek elev offset2=%d\n", (int)offset2);
	    return -1;
	}
	if (!
	    (fwrite
	     (array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
	      params->Tmp_fd_z))) {
	    fprintf(stderr, "Not enough disk space--cannot write files\n");
	    return -1;
	}
    }
    if (params->Tmp_fd_dx != NULL) {
	for (j = ngstc; j <= nszc; j++)
	    if (!params->deriv)
		array_cell[j - 1] = (FCELL) params->adx[j];
	    else
		array_cell[j - 1] = (FCELL) (params->adx[j] * params->scik1);
	if (fseek(params->Tmp_fd_dx, (long)offset2, 0) == -1) {
	    fprintf(stderr, "Cannot fseek slope\n");
	    return -1;
	}
	if (!
	    (fwrite
	     (array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
	      params->Tmp_fd_dx))) {
	    fprintf(stderr, "Not enough disk space--cannot write files\n");
	    return -1;
	}
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
	if (fseek(params->Tmp_fd_dy, (long)offset2, 0) == -1) {
	    fprintf(stderr, "Cannot fseek aspect\n");
	    return -1;
	}
	if (!
	    (fwrite
	     (array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
	      params->Tmp_fd_dy))) {
	    fprintf(stderr, "Not enough disk space--cannot write files\n");
	    return -1;
	}
    }
    if (params->Tmp_fd_xx != NULL) {
	for (j = ngstc; j <= nszc; j++) {
	    array_cell[j - 1] = (FCELL) (params->adxx[j] * params->scik1);
	}
	if (fseek(params->Tmp_fd_xx, (long)offset2, 0) == -1) {
	    fprintf(stderr, "Cannot fseek pcurv\n");
	    return -1;
	}
	if (!
	    (fwrite
	     (array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
	      params->Tmp_fd_xx))) {
	    fprintf(stderr, "Not enough disk space--cannot write files\n");
	    return -1;
	}
    }
    if (params->Tmp_fd_yy != NULL) {
	for (j = ngstc; j <= nszc; j++)
	    array_cell[j - 1] = (FCELL) (params->adyy[j] * params->scik2);
	if (fseek(params->Tmp_fd_yy, (long)offset2, 0) == -1) {
	    fprintf(stderr, "Cannot fseek tcurv\n");
	    return -1;
	}
	if (!
	    (fwrite
	     (array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
	      params->Tmp_fd_yy))) {
	    fprintf(stderr, "Not enough disk space--cannot write files\n");
	    return -1;
	}
    }
    if (params->Tmp_fd_xy != NULL) {
	for (j = ngstc; j <= nszc; j++)
	    array_cell[j - 1] = (FCELL) (params->adxy[j] * params->scik3);
	if (fseek(params->Tmp_fd_xy, (long)offset2, 0) == -1) {
	    fprintf(stderr, "Cannot fseek mcurv\n");
	    return -1;
	}
	if (!
	    (fwrite
	     (array_cell + ngstc - 1, sizeof(FCELL), nszc - ngstc + 1,
	      params->Tmp_fd_xy))) {
	    fprintf(stderr, "Not enough disk space--cannot write files\n");
	    return -1;
	}
    }
    return 1;
}
