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

#define MAIN
#include <grass/imagery.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main (int argc, char *argv[])
{
        /* Global variable & function declarations */

        int i,j,k;             /* Loop control variables */
        int bands;            /* Number of image bands */
        int nclass;           /* Number of classes */
        int samptot;          /* Total number of sample points */
        double mu[MC][MX];      /* Mean vector for image classes */
        double w[MX][MX];       /* Within Class Covariance Matrix */
        double p[MX][MX];       /* Between class Covariance Matrix */
        double l[MX][MX];       /* Diagonal matrix of eigenvalues */
        double q[MX][MX];       /* Transformation matrix */
        double cov[MC][MX][MX]; /* Individual class Covariance Matrix */
        double nsamp[MC];       /* Number of samples in a given class */
        double eigval[MX];      /* Eigen value vector */
        double eigmat[MX][MX];  /* Eigen Matrix */
        char tempname[50];

        /* used to make the color tables */
        CELL outbandmax[MX];  /* will hold the maximums found in the out maps */
        CELL outbandmin[MX];  /* will hold the minimums found in the out maps */
        struct Colors color_tbl;
        struct Signature sigs;
        FILE *sigfp;
        struct Ref refs;
        int datafds[MX];
        int outfds[MX];

        struct GModule *module;
        struct Option *grp_opt, *subgrp_opt, *sig_opt, *out_opt;

        /***** Start of main *****/
        G_gisinit(argv[0]);

		module = G_define_module();
		module->keywords = _("imagery");
    module->description =
			_("Canonical components analysis (cca) " \
			"program for image processing.");

        grp_opt = G_define_standard_option (G_OPT_I_GROUP);

        subgrp_opt = G_define_standard_option (G_OPT_I_GROUP);
        subgrp_opt->key        = "subgroup";
        subgrp_opt->description= _("Name of input imagery subgroup");

        sig_opt = G_define_option() ;
        sig_opt->key        = "signature";
        sig_opt->type       = TYPE_STRING;
        sig_opt->required   = YES;
        sig_opt->description= _("Ascii file containing spectral signatures");

        out_opt = G_define_standard_option (G_OPT_R_OUTPUT);
        out_opt->description= _("Output raster map prefix name");

        if (G_parser(argc, argv) < 0)
                exit(EXIT_FAILURE);

        if (G_legal_filename (grp_opt->answer) < 0)
            G_fatal_error (_("Illegal group name <%s>"), grp_opt->answer);

        if (G_legal_filename (subgrp_opt->answer) < 0)
            G_fatal_error (_("Illegal subgroup name <%s>"), subgrp_opt->answer);

        if (G_legal_filename (sig_opt->answer) < 0)
            G_fatal_error (_("Illegal signature file name <%s>"), sig_opt->answer);

        if (G_legal_filename (out_opt->answer) < 0)
            G_fatal_error (_("Illegal output file name <%s>"), out_opt->answer);

        /* check group, subgroup */
        I_init_group_ref(&refs);
        if (I_find_group (grp_opt->answer) <= 0)
                G_fatal_error(_("Unknown imagery group."));

        if (I_get_subgroup_ref (grp_opt->answer, subgrp_opt->answer, &refs) <= 0)
                G_fatal_error(_("Unable to find subgroup reference information."));

        /* open and input the signatures file */
        if ((sigfp = I_fopen_signature_file_old(grp_opt->answer, subgrp_opt->answer, sig_opt->answer)) == NULL)
                G_fatal_error (_("Unable to open the signature file"));

        I_init_signatures(&sigs, refs.nfiles);
        if (I_read_signatures(sigfp, &sigs)<0)
                G_fatal_error(_("Error while reading the signatures file."));

        fclose(sigfp);
        nclass = sigs.nsigs;
        if (nclass<2)
                G_fatal_error(_("Need at least two signatures in signature file."));

        /* check the number of input bands */
        bands = refs.nfiles;
        if (bands > MX-1)
                G_fatal_error(_("Subgroup too large.  Maximum number of bands is %d\n."), MX - 1);

        /*
    Here is where the information regarding
      a) Number of samples per class
      b) Covariance matrix for each class
    are read into the  program
    */

        samptot = 0;
        for (i=1; i<=nclass; i++) {
                nsamp[i] = sigs.sig[i-1].npoints;
                samptot += nsamp[i];
                for (j=1; j<=bands; j++) {
                        mu[i][j] = sigs.sig[i-1].mean[j-1];
                        for (k=1; k<=j; k++)
                                cov[i][j][k] = cov[i][k][j] = sigs.sig[i-1].var[j-1][k-1];
                }
        }

        within(samptot,nclass,nsamp,cov,w,bands);
        between(samptot,nclass,nsamp,mu,p,bands);
        jacobi(w,(long)bands,eigval,eigmat);
        egvorder(eigval,eigmat,(long)bands);
        setdiag(eigval,bands,l);
        getsqrt(w,bands,l,eigmat);
        solveq(q,bands,w,p);
        jacobi(q,(long)bands,eigval,eigmat);
        egvorder(eigval,eigmat,(long)bands);
        matmul(q,eigmat,w,bands);

        /* open the cell maps */
        for (i=1; i<=bands; i++) {
                outbandmax[i] = (CELL) 0;
                outbandmin[i] = (CELL) 0;

                if ((datafds[i]=G_open_cell_old(refs.file[i-1].name,
                    refs.file[i-1].mapset)) < 0) {
                        G_fatal_error(_("Cannot open raster map <%s>"),
                                    refs.file[i-1].name);
                }

                sprintf(tempname, "%s.%d", out_opt->answer, i);
                if ((outfds[i]=G_open_cell_new(tempname)) < 0)
                        G_fatal_error(_("Cannot create raster map <%s>"), tempname);
        }

        /* do the transform */
    transform(datafds, outfds, G_window_rows (), G_window_cols (), q, bands, outbandmin, outbandmax);

        /* make grey scale color table */
        G_init_colors(&color_tbl);

        /* close the cell maps */
        for (i=1; i<=bands; i++) {
                G_close_cell(datafds[i]);
                G_close_cell(outfds[i]);

                if (outbandmin[i] < (CELL) 0 || outbandmax[i] > (CELL) 255) {
                        G_warning(_("The output cell map <%s.%d> has values "
                            "outside the 0-255 range."), out_opt->answer, i);
                }

                G_make_grey_scale_colors(&color_tbl, 0, outbandmax[i]);
                sprintf(tempname, "%s.%d", out_opt->answer, i);

                /* write a color table */
                G_write_colors(tempname, G_mapset(), &color_tbl);
        }

        I_free_signatures(&sigs);
        I_free_group_ref(&refs);

	exit(EXIT_SUCCESS);
}



