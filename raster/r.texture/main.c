
/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from: 
 * 			prof. Giulio Antoniol - antoniol@ieee.org
 * 			prof. Michele Ceccarelli - ceccarelli@unisannio.it
 *
 * PURPOSE:      Create map raster with textural features.
 *
 * COPYRIGHT:    (C) 2003 by University of Sannio (BN), Benevento, Italy 
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted. This 
 * software is provided "as is" without express or implied warranty.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "h_measure.h"

static const char *suffixes[56] = {
    "_ASM_0", "_ASM_45", "_ASM_90", "_ASM_135",
    "_Contr_0", "_Contr_45", "_Contr_90", "_Contr_135",
    "_Corr_0", "_Corr_45", "_Corr_90", "_Corr_135",
    "_Var_0", "_Var_45", "_Var_90", "_Var_135",
    "_IDM_0", "_IDM_45", "_IDM_90", "_IDM_135",
    "_SA_0", "_SA_45", "_SA_90", "_SA_135",
    "_SV_0", "_SV_45", "_SV_90", "_SV_135",
    "_SE_0", "_SE_45", "_SE_90", "_SE_135",
    "_Entr_0", "_Entr_45", "_Entr_90", "_Entr_135",
    "_DV_0", "_DV_45", "_DV_90", "_DV_135",
    "_DE_0", "_DE_45", "_DE_90", "_DE_135",
    "_MOC-l_0", "_MOC-l_45", "_MOC-l_90", "_MOC-l_135",
    "_MOC-2_0", "_MOC-2_45", "_MOC-2_90", "_MOC-2_135",
    "_MCC_0", "_MCC_45", "_MCC_90", "_MCC_135"
};

int main(int argc, char *argv[])
{
    struct Cell_head cellhd;
    char *name, *result, *filename;
    unsigned char *outrast;
    int nrows, ncols;
    int row, col, i, j;
    CELL **data;		/* Data structure containing image */
    CELL *cell_row;
    FCELL measure;		/* Containing measure done */
    int t_measure, dist, size;	/* dist = value of distance, size = s. of sliding window */
    int infd, outfd;
    int verbose;
    int a, c, corr, v, idm, sa, sv, se, e, dv, de, moc1, moc2, mcc;
    RASTER_MAP_TYPE data_type, out_data_type;
    struct GModule *module;
    char mapname[GNAME_MAX];
    struct Option *input, *output, *size_O, *dist_O;
    struct Flag *flag2, *flag3, *flag4, *flag5,
	*flag6, *flag7, *flag8, *flag9, *flag10, *flag11,
	*flag12, *flag13, *flag14, *flag15;
    struct History history;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    G_add_keyword(_("statistics"));
    module->description =
	_("Generate images with textural features from a raster map.");

    /* Define the different options */

    input = G_define_standard_option(G_OPT_R_INPUT);

    output = G_define_option();
    output->key = "prefix";
    output->type = TYPE_STRING;
    output->required = YES;
    output->gisprompt = "new,cell,raster";
    output->description = _("Prefix for output raster map(s)");

    size_O = G_define_option();
    size_O->key = "size";
    size_O->key_desc = "value";
    size_O->type = TYPE_INTEGER;
    size_O->required = NO;
    size_O->description = _("The size of sliding window (odd and >= 3)");
    size_O->answer = "3";

    /* Textural character is in direct relation of the spatial size of the texture primitives. */

    dist_O = G_define_option();
    dist_O->key = "distance";
    dist_O->key_desc = "value";
    dist_O->type = TYPE_INTEGER;
    dist_O->required = NO;
    dist_O->description = _("The distance between two samples (>= 1)");
    dist_O->answer = "1";

    /* Define the different flags */

    /* "Normalized" unused in the code ??? 
       flag0 = G_define_flag() ;
       flag0->key         = 'N' ;
       flag0->description = _("Normalized") ;
       flag0->guisection  = _("Features");
     */

    flag2 = G_define_flag();
    flag2->key = 'a';
    flag2->description = _("Angular Second Moment");
    flag2->guisection = _("Features");

    flag3 = G_define_flag();
    flag3->key = 'c';
    flag3->description = _("Contrast");
    flag3->guisection = _("Features");

    flag4 = G_define_flag();
    flag4->key = 'k';
    flag4->description = _("Correlation");
    flag4->guisection = _("Features");

    flag5 = G_define_flag();
    flag5->key = 'v';
    flag5->description = _("Variance");
    flag5->guisection = _("Features");

    flag6 = G_define_flag();
    flag6->key = 'i';
    flag6->description = _("Inverse Diff Moment");
    flag6->guisection = _("Features");

    flag7 = G_define_flag();
    flag7->key = 's';
    flag7->description = _("Sum Average");
    flag7->guisection = _("Features");

    flag8 = G_define_flag();
    flag8->key = 'w';
    flag8->description = _("Sum Variance");
    flag8->guisection = _("Features");

    flag9 = G_define_flag();
    flag9->key = 'x';
    flag9->description = _("Sum Entropy");
    flag9->guisection = _("Features");

    flag10 = G_define_flag();
    flag10->key = 'e';
    flag10->description = _("Entropy");
    flag10->guisection = _("Features");

    flag11 = G_define_flag();
    flag11->key = 'd';
    flag11->description = _("Difference Variance");
    flag11->guisection = _("Features");

    flag12 = G_define_flag();
    flag12->key = 'p';
    flag12->description = _("Difference Entropy");
    flag12->guisection = _("Features");

    flag13 = G_define_flag();
    flag13->key = 'm';
    flag13->description = _("Measure of Correlation-1");
    flag13->guisection = _("Features");

    flag14 = G_define_flag();
    flag14->key = 'n';
    flag14->description = _("Measure of Correlation-2");
    flag14->guisection = _("Features");

    flag15 = G_define_flag();
    flag15->key = 'o';
    flag15->description = _("Max Correlation Coeff");
    flag15->guisection = _("Features");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = input->answer;
    result = output->answer;
    a = (!flag2->answer);
    c = (!flag3->answer);
    corr = (!flag4->answer);
    v = (!flag5->answer);
    idm = (!flag6->answer);
    sa = (!flag7->answer);
    sv = (!flag8->answer);
    se = (!flag9->answer);
    e = (!flag10->answer);
    dv = (!flag11->answer);
    de = (!flag12->answer);
    moc1 = (!flag13->answer);
    moc2 = (!flag14->answer);
    mcc = (!flag15->answer);
    size = atoi(size_O->answer);
    dist = atoi(dist_O->answer);

    if (a && c && corr && v && idm && sa && sv && se && e && dv && de && moc1
	&& moc2 && mcc)
	G_fatal_error(_("Nothing to compute. Use at least one of the flags."));

    infd = Rast_open_old(name, "");

    /* determine the inputmap type (CELL/FCELL/DCELL) */
    data_type = Rast_get_map_type(infd);

    Rast_get_cellhd(name, "", &cellhd);

    out_data_type = FCELL_TYPE;
    /* Allocate output buffer, use FCELL data_type */
    outrast = Rast_allocate_buf(out_data_type);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* Load raster map. */

    /* allocate the space for one row of cell map data *A* */
    cell_row = Rast_allocate_c_buf();

    /* Allocate appropriate memory for the structure containing the image */
    data = (int **)G_malloc(nrows * sizeof(int *));
    for (i = 0; i < nrows; i++) {
	data[i] = (int *)G_malloc(ncols * sizeof(int));
    }

    /* Read in cell map values */
    G_important_message(_("Reading raster map..."));
    for (j = 0; j < nrows; j++) {
	Rast_get_row(infd, cell_row, j, CELL_TYPE);
	for (i = 0; i < ncols; i++)
	    data[j][i] = (int)cell_row[i];
    }

    /* close input cell map and release the row buffer */
    Rast_close(infd);
    G_free(cell_row);

    /* Now raster map is into memory. */

    /* Open image's files, their names show the measure */
    filename = G_malloc(strlen(result) + 11);
    strcpy(filename, result);
    result = filename;
    while (*result != '\0')
	result++;

    /* *************************************************************************************************
     *
     * Compute of the matrix S.G.L.D. (Spatial Gray-Level Dependence Matrices) or co-occurrence matrix.
     * The image is analized for piece, every piece is naming sliding window (s.w.). The s.w. must be    
     * square with number of size's samples odd, that because we want the sample at the center of matrix. 
     *
     ***************************************************************************************************/

    for (t_measure = 0; t_measure < 56; t_measure++)
	if ((t_measure == 0 && a)
	    || (t_measure == 4 && c)
	    || (t_measure == 8 && corr)
	    || (t_measure == 12 && v)
	    || (t_measure == 16 && idm)
	    || (t_measure == 20 && sa)
	    || (t_measure == 24 && sv)
	    || (t_measure == 28 && se)
	    || (t_measure == 32 && e)
	    || (t_measure == 36 && dv)
	    || (t_measure == 40 && de)
	    || (t_measure == 44 && moc1)
	    || (t_measure == 48 && moc2)
	    || (t_measure == 52 && mcc))
	    t_measure += 3;
	else {
	    outfd = Rast_open_new(strcat(filename, suffixes[t_measure]),
				  out_data_type);
	    *result = '\0';

	    for (row = 0; row < nrows - (size - 1); row++) {
		if (verbose)
		    G_percent(row, nrows, 2);

		/*process the data */
		for (col = 0; col < ncols - (size - 1); col++) {

		    /* Pointer for the s.w. */
		    data = data + row;
		    for (j = 0; j < size; j++)
			*(data + j) += col;

/***********************************************************************************************
 *
 * Parameter of the h_measure:
 *   - data: structure containing image;
 *   - size: size of the s. w., defined by operator; 
 *           The s.w. can be rectangolar but not make sense.
 *   - t_measure: measure's type required;
 *   - dist: distance required, defined by operator.
 *
 **********************************************************************************************/

		    measure =
			(FCELL) h_measure(data, size, size, t_measure, dist);
		    /* The early (size/2) samples take value from (size/2+1)'th sample */
		    if (col < (size / 2))
			for (j = 0; j < (size / 2); j++)
			    ((FCELL *) outrast)[j] = measure;
		    ((FCELL *) outrast)[col + ((size / 2))] = measure;
		    /* The last few (size/2) samples take value from nrows-(size/2+1)'th sample */
		    if (col == (ncols - size))
			for (j = 0; j <= (size / 2); j++)
			    ((FCELL *) outrast)[ncols - j] = measure;

		    for (j = (size - 1); j >= 0; j--)
			*(data + j) -= col;
		    data = data - row;
		}
		/* The early (size/2) samples take value from (size/2+1)'th sample */
		if (row == 0)
		    for (j = 0; j < (size / 2); j++)
			Rast_put_row(outfd, outrast, out_data_type);

		Rast_put_row(outfd, outrast, out_data_type);
	    }
	    /* The last few (size/2) samples take value from nrows-(size/2+1)'th sample */
	    if ((row >= nrows - (size - 1)) && (row < nrows))
		for (j = 0; j < (size / 2); j++)
		    Rast_put_row(outfd, outrast, out_data_type);

	    Rast_close(outfd);
	    strcpy(mapname, filename);
	    strcat(mapname, suffixes[t_measure]);
	    G_important_message(_("Calculated measure #%d <%s> (56 measures available)"),
				(t_measure + 1), mapname);

	    Rast_short_history(mapname, "raster", &history);
	    Rast_command_history(&history);
	    Rast_write_history(mapname, &history);

	}
    G_free(outrast);
    G_free(data);

    exit(EXIT_SUCCESS);
}
