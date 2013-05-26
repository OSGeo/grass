
/****************************************************************************
 *
 * MODULE:       i.cca
 * AUTHOR(S):    David Satnik, Central Washington University, and  
 *               Ali R. Vali, University of Texas (original contributors)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      canonical components transformation: takes from two to eight
 *               (raster) band files and a signature file, and outputs the same
 *               number of raster band files transformed to provide maximum 
 *               separability of the categories indicated by the signatures.  
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/*

   Ali Vali contact info:
   Center for Space Research
   WRW 402
   University of Texas
   Austin, TX 78712-1085

   (512) 471-6824

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main(int argc, char *argv[])
{
    /* Global variable & function declarations */

    int i, j, k;		/* Loop control variables */
    int bands;			/* Number of image bands */
    int nclass;			/* Number of classes */
    int samptot;		/* Total number of sample points */
    double **mu;		/* Mean vector for image classes */
    double **w;		/* Within Class Covariance Matrix */
    double **p;		/* Between class Covariance Matrix */
    double **l;		/* Diagonal matrix of eigenvalues */
    double **q;		/* Transformation matrix */
    double ***cov;	/* Individual class Covariance Matrix */
    double *nsamp;		/* Number of samples in a given class */
    double *eigval;		/* Eigen value vector */
    double **eigmat;	/* Eigen Matrix */
    char tempname[1024];

    /* used to make the color tables */
    CELL *outbandmax;	/* will hold the maximums found in the out maps */
    CELL *outbandmin;	/* will hold the minimums found in the out maps */
    struct Colors color_tbl;
    struct Signature sigs;
    FILE *sigfp;
    struct Ref refs;
    int *datafds;
    int *outfds;

    struct GModule *module;
    struct Option *grp_opt, *subgrp_opt, *sig_opt, *out_opt;

	/***** Start of main *****/
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("statistics"));
    G_add_keyword("CCA");
    module->description =
	_("Canonical components analysis (CCA) "
	  "program for image processing.");

    grp_opt = G_define_standard_option(G_OPT_I_GROUP);

    subgrp_opt = G_define_standard_option(G_OPT_I_GROUP);
    subgrp_opt->key = "subgroup";
    subgrp_opt->description = _("Name of input imagery subgroup");

    sig_opt = G_define_option();
    sig_opt->key = "signature";
    sig_opt->type = TYPE_STRING;
    sig_opt->required = YES;
    sig_opt->key_desc = "name";
    sig_opt->description = _("File containing spectral signatures");

    out_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_opt->description = _("Output raster map prefix name");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* check group, subgroup */
    I_init_group_ref(&refs);
    if (I_find_group(grp_opt->answer) <= 0)
	G_fatal_error(_("Unknown imagery group."));

    if (I_get_subgroup_ref(grp_opt->answer, subgrp_opt->answer, &refs) <= 0)
	G_fatal_error(_("Unable to find subgroup reference information."));

    /* open and input the signatures file */
    if ((sigfp =
	 I_fopen_signature_file_old(grp_opt->answer, subgrp_opt->answer,
				    sig_opt->answer)) == NULL)
	G_fatal_error(_("Unable to open the signature file"));

    I_init_signatures(&sigs, refs.nfiles);
    if (I_read_signatures(sigfp, &sigs) < 0)
	G_fatal_error(_("Error while reading the signatures file."));

    fclose(sigfp);
    nclass = sigs.nsigs;
    if (nclass < 2)
	G_fatal_error(_("Need at least two signatures in signature file."));

    /* check the number of input bands */
    bands = refs.nfiles;

    /*memory allocation*/
    mu = G_alloc_matrix(nclass, bands);
    w = G_alloc_matrix(bands, bands);
    p = G_alloc_matrix(bands, bands);
    l = G_alloc_matrix(bands, bands);
    q = G_alloc_matrix(bands, bands);
    eigmat = G_alloc_matrix(bands, bands);
    nsamp = G_alloc_vector(nclass);
    eigval = G_alloc_vector(bands);

    cov = (double***)G_calloc(nclass, sizeof(double**));
    for(i = 0; i < nclass; i++)
    {
        cov[i] = G_alloc_matrix(bands,bands);
    }

    outbandmax = (CELL*)G_calloc(nclass, sizeof(CELL));
    outbandmin = (CELL*)G_calloc(nclass, sizeof(CELL));
    datafds = (int*)G_calloc(nclass, sizeof(int));
    outfds = (int*)G_calloc(nclass, sizeof(int));


    /*
       Here is where the information regarding
       a) Number of samples per class
       b) Covariance matrix for each class
       are read into the  program
     */

    samptot = 0;
    for (i = 1; i <= nclass; i++) {
	nsamp[i] = sigs.sig[i - 1].npoints;
	samptot += nsamp[i];
	for (j = 1; j <= bands; j++) {
	    mu[i][j] = sigs.sig[i - 1].mean[j - 1];
	    for (k = 1; k <= j; k++)
		cov[i][j][k] = cov[i][k][j] =
		    sigs.sig[i - 1].var[j - 1][k - 1];
	}
    }

    within(samptot, nclass, nsamp, cov, w, bands);
    between(samptot, nclass, nsamp, mu, p, bands);
    G_math_d_copy(w[0], eigmat[0], bands*bands);
    G_math_eigen(eigmat, eigval, bands);
    G_math_egvorder(eigval, eigmat, bands);
    setdiag(eigval, bands, l);
    getsqrt(w, bands, l, eigmat);
    solveq(q, bands, w, p);
    G_math_d_copy(q[0], eigmat[0], bands*bands);
    G_math_eigen(eigmat, eigval, bands);
    G_math_egvorder(eigval, eigmat, bands);
    G_math_d_AB(eigmat, w, q, bands, bands, bands);

    for(i = 0; i < bands; i++)
    {
        G_verbose_message("%i. eigen value: %+6.5f", i, eigval[i]);
        G_verbose_message("eigen vector:");
	for(j = 0; j < bands; j++)
            G_verbose_message("%+6.5f ", eigmat[i][j]);

    }


    /* open the cell maps */
    for (i = 1; i <= bands; i++) {
	outbandmax[i] = (CELL) 0;
	outbandmin[i] = (CELL) 0;

	datafds[i] = Rast_open_old(refs.file[i - 1].name,
				   refs.file[i - 1].mapset);

	sprintf(tempname, "%s.%d", out_opt->answer, i);
	outfds[i] = Rast_open_c_new(tempname);
    }

    /* do the transform */
    transform(datafds, outfds, Rast_window_rows(), Rast_window_cols(), q, bands,
	      outbandmin, outbandmax);

    /* make grey scale color table */
    Rast_init_colors(&color_tbl);

    /* close the cell maps */
    for (i = 1; i <= bands; i++) {
	Rast_close(datafds[i]);
	Rast_close(outfds[i]);

	if (outbandmin[i] < (CELL) 0 || outbandmax[i] > (CELL) 255) {
	    G_warning(_("The output cell map <%s.%d> has values "
			"outside the 0-255 range."), out_opt->answer, i);
	}

	Rast_make_grey_scale_colors(&color_tbl, 0, outbandmax[i]);
	sprintf(tempname, "%s.%d", out_opt->answer, i);

	/* write a color table */
	Rast_write_colors(tempname, G_mapset(), &color_tbl);
    }

    I_free_signatures(&sigs);
    I_free_group_ref(&refs);
    
    /*free memory*/
    G_free_matrix(mu);
    G_free_matrix(w);
    G_free_matrix(p);
    G_free_matrix(l);
    G_free_matrix(q);
    G_free_matrix(eigmat);
    for(i = 0; i < nclass; i++)
        G_free_matrix(cov[i]);
    G_free(cov);

    G_free_vector(nsamp);
    G_free_vector(eigval);

    G_free(outbandmax);
    G_free(outbandmin);
    G_free(datafds);
    G_free(outfds);

    exit(EXIT_SUCCESS);
}
