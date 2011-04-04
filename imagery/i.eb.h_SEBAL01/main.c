
/****************************************************************************
 *
 * MODULE:       i.eb.h_SEBAL01
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates sensible heat flux by SEBAL iteration
 *               Delta T will be reassessed in the iterations !
 *               This has been seen in Bastiaanssen (1995), 
 *               later modified by Chemin and Alexandridis (2001).
 *               This code is implemented in Alexandridis et al. (2009).
 *
 * COPYRIGHT:    (C) 2002-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 * CHANGELOG:	
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

double **G_alloc_matrix(int rows, int cols)
{
    double **m;
    int i;

    m = (double **)G_calloc(rows, sizeof(double *));
    m[0] = (double *)G_calloc(rows * cols, sizeof(double));
    for (i = 1; i < rows; i++)
	m[i] = m[i - 1] + cols;

    return m;
}

int main(int argc, char *argv[])
{
    struct Cell_head cellhd;
    /* buffer for in, tmp and out raster */
    void *inrast_Rn, *inrast_g0;
    void *inrast_z0m, *inrast_t0dem;
    DCELL *outrast;
    int nrows, ncols;
    int row, col;
    int row_wet, col_wet;
    int row_dry, col_dry;
    double m_row_wet, m_col_wet;
    double m_row_dry, m_col_dry;
    int infd_Rn, infd_g0;
    int infd_z0m, infd_t0dem;
    int outfd;
    char *Rn, *g0;
    char *z0m, *t0dem;
    char *h0;

    double ustar, ea;
    struct History history;
    struct GModule *module;
    struct Option *input_Rn, *input_g0;
    struct Option *input_z0m, *input_t0dem, *input_ustar;
    struct Option *input_ea, *output;
    struct Option *input_row_wet, *input_col_wet;
    struct Option *input_row_dry, *input_col_dry;
    struct Flag *flag2, *flag3;
    /********************************/
    double xp, yp;
    double xmin, ymin;
    double xmax, ymax;
    double stepx, stepy;
    double latitude, longitude;
    int rowDry, colDry, rowWet, colWet;
    /********************************/
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("evaporative fraction"));
    G_add_keyword(_("soil moisture"));
    G_add_keyword(_("energy balance"));
    G_add_keyword(_("SEBAL"));
    module->description = _("Computes sensible heat flux iteration SEBAL 01.");

    /* Define different options */
    input_Rn = G_define_standard_option(G_OPT_R_INPUT);
    input_Rn->key = "netradiation";
    input_Rn->description =
	_("Name of instantaneous Net Radiation raster map [W/m2]");

    input_g0 = G_define_standard_option(G_OPT_R_INPUT);
    input_g0->key = "soilheatflux";
    input_g0->description =
	_("Name of instantaneous soil heat flux raster map [W/m2]");

    input_z0m = G_define_standard_option(G_OPT_R_INPUT);
    input_z0m->key = "aerodynresistance";
    input_z0m->description =
	_("Name of aerodynamic resistance to heat momentum raster map [s/m]");

    input_t0dem = G_define_standard_option(G_OPT_R_INPUT);
    input_t0dem->key = "temperaturemeansealevel";
    input_t0dem->description =
	_("Name of altitude corrected surface temperature raster map [K]");

    input_ustar = G_define_option();
    input_ustar->key = "frictionvelocitystar";
    input_ustar->type = TYPE_DOUBLE;
    input_ustar->required = YES;
    input_ustar->gisprompt = "old,value";
    input_ustar->answer = "0.32407";
    input_ustar->description = _("Value of the height independent friction velocity (u*) [m/s]");
    input_ustar->guisection = _("Parameters");

    input_ea = G_define_option();
    input_ea->key = "vapourpressureactual";
    input_ea->type = TYPE_DOUBLE;
    input_ea->required = YES;
    input_ea->answer = "1.511";
    input_ea->description = _("Value of the actual vapour pressure (e_act) [KPa]");
    input_ea->guisection = _("Parameters");

    input_row_wet = G_define_option();
    input_row_wet->key = "row_wet_pixel";
    input_row_wet->type = TYPE_DOUBLE;
    input_row_wet->required = NO;
    input_row_wet->description = _("Row value of the wet pixel");
    input_row_wet->guisection = _("Parameters");

    input_col_wet = G_define_option();
    input_col_wet->key = "column_wet_pixel";
    input_col_wet->type = TYPE_DOUBLE;
    input_col_wet->required = NO;
    input_col_wet->description = _("Column value of the wet pixel");
    input_col_wet->guisection = _("Parameters");

    input_row_dry = G_define_option();
    input_row_dry->key = "row_dry_pixel";
    input_row_dry->type = TYPE_DOUBLE;
    input_row_dry->required = NO;
    input_row_dry->description = _("Row value of the dry pixel");
    input_row_dry->guisection = _("Parameters");

    input_col_dry = G_define_option();
    input_col_dry->key = "column_dry_pixel";
    input_col_dry->type = TYPE_DOUBLE;
    input_col_dry->required = NO;
    input_col_dry->description = _("Column value of the dry pixel");
    input_col_dry->guisection = _("Parameters");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->description = _("Name for output sensible heat flux raster map [W/m2]");
    
    /* Define the different flags */
    flag2 = G_define_flag();
    flag2->key = 'a';
    flag2->description = _("Automatic wet/dry pixel (careful!)");

    flag3 = G_define_flag();
    flag3->key = 'c';
    flag3->description =
	_("Dry/Wet pixels coordinates are in image projection, not row/col");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* get entered parameters */
    Rn = input_Rn->answer;
    g0 = input_g0->answer;
    z0m = input_z0m->answer;
    t0dem = input_t0dem->answer;

    h0 = output->answer;

    ustar = atof(input_ustar->answer);
    ea = atof(input_ea->answer);

    if(input_row_wet->answer&&
    input_col_wet->answer&&
    input_row_dry->answer&&
    input_col_dry->answer){
        m_row_wet = atof(input_row_wet->answer);
        m_col_wet = atof(input_col_wet->answer);
        m_row_dry = atof(input_row_dry->answer);
        m_col_dry = atof(input_col_dry->answer);
    }
    if ((!input_row_wet->answer || !input_col_wet->answer ||
	 !input_row_dry->answer || !input_col_dry->answer) &&
	!flag2->answer) {
	G_fatal_error(_("Either auto-mode either wet/dry pixels coordinates should be provided!"));
    }
    if (flag3->answer) {
	G_message(_("Manual wet/dry pixels in image coordinates"));
	G_message(_("Wet Pixel=> x:%f y:%f"), m_col_wet, m_row_wet);
	G_message(_("Dry Pixel=> x:%f y:%f"), m_col_dry, m_row_dry);
    }
    else {
        if(flag2->answer)
	    G_message(_("Automatic mode selected"));
	else {
	    G_message(_("Wet Pixel=> row:%.0f col:%.0f"), m_row_wet, m_col_wet);
	    G_message(_("Dry Pixel=> row:%.0f col:%.0f"), m_row_dry, m_col_dry);
	}
    }
    /* check legal output name */
    if (G_legal_filename(h0) < 0)
	G_fatal_error(_("<%s> is an illegal name"), h0);

    infd_Rn = Rast_open_old(Rn, "");
    infd_g0 = Rast_open_old(g0, "");
    infd_z0m = Rast_open_old(z0m, "");
    infd_t0dem = Rast_open_old(t0dem, "");

    Rast_get_cellhd(Rn, "", &cellhd);
    Rast_get_cellhd(g0, "", &cellhd);
    Rast_get_cellhd(z0m, "", &cellhd);
    Rast_get_cellhd(t0dem, "", &cellhd);

    /* Allocate input buffer */
    inrast_Rn = Rast_allocate_d_buf();
    inrast_g0 = Rast_allocate_d_buf();
    inrast_z0m = Rast_allocate_d_buf();
    inrast_t0dem = Rast_allocate_d_buf();

    /***************************************************/
    /* Setup pixel location variables */
    /***************************************************/
    stepx = cellhd.ew_res;
    stepy = cellhd.ns_res;

    xmin = cellhd.west;
    xmax = cellhd.east;
    ymin = cellhd.south;
    ymax = cellhd.north;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /***************************************************/
    /* Allocate output buffer */
    /***************************************************/
    outrast = Rast_allocate_d_buf();
    outfd = Rast_open_new(h0, DCELL_TYPE);
    /***************************************************/
    /* Allocate memory for temporary images            */
    double **d_Roh, **d_Rah;

    if ((d_Roh = G_alloc_matrix(nrows, ncols)) == NULL)
	G_message("Unable to allocate memory for temporary d_Roh image");
    if ((d_Rah = G_alloc_matrix(nrows, ncols)) == NULL)
	G_message("Unable to allocate memory for temporary d_Rah image");
    /***************************************************/

    /* MANUAL T0DEM WET/DRY PIXELS */
    DCELL d_Rn_dry,d_g0_dry;
    DCELL d_t0dem_dry,d_t0dem_wet;

    if (flag2->answer) {
	/* Process tempk min / max pixels */
	/* Internal use only */
	DCELL d_Rn_wet,d_g0_wet;
	DCELL d_Rn,d_g0,d_h0;
	DCELL t0dem_min,t0dem_max;
        /*********************/
	for (row = 0; row < nrows; row++) {
	    DCELL d_t0dem;
	    G_percent(row, nrows, 2);
	    Rast_get_d_row(infd_t0dem,inrast_t0dem,row);
	    Rast_get_d_row(infd_Rn,inrast_Rn,row);
	    Rast_get_d_row(infd_g0,inrast_g0,row);
	    /*process the data */
	    for (col = 0; col < ncols; col++) {
		d_t0dem = ((DCELL *) inrast_t0dem)[col];
		d_Rn = ((DCELL *) inrast_Rn)[col];
		d_g0 = ((DCELL *) inrast_g0)[col];
		if (Rast_is_d_null_value(&d_t0dem) ||
		    Rast_is_d_null_value(&d_Rn) || 
                    Rast_is_d_null_value(&d_g0)) {
		    /* do nothing */
		}
		else {
		    if (d_t0dem <= 250.0) {
			/* do nothing */
		    }
		    else {
			d_h0 = d_Rn - d_g0;
			if (d_t0dem < t0dem_min &&
			    d_Rn > 0.0 && d_g0 > 0.0 && d_h0 > 0.0 &&
			    d_h0 < 100.0) {
			    t0dem_min = d_t0dem;
			    d_t0dem_wet = d_t0dem;
			    d_Rn_wet = d_Rn;
			    d_g0_wet = d_g0;
			    m_col_wet = col;
			    m_row_wet = row;
			}
			if (d_t0dem > t0dem_max &&
			    d_Rn > 0.0 && d_g0 > 0.0 && d_h0 > 100.0 &&
			    d_h0 < 500.0) {
			    t0dem_max = d_t0dem;
			    d_t0dem_dry = d_t0dem;
			    d_Rn_dry = d_Rn;
			    d_g0_dry = d_g0;
			    m_col_dry = col;
			    m_row_dry = row;
			}
		    }
		}
	    }
	}
	G_message("row_wet=%d\tcol_wet=%d", row_wet, col_wet);
	G_message("row_dry=%d\tcol_dry=%d", row_dry, col_dry);
	G_message("g0_wet=%f", d_g0_wet);
	G_message("Rn_wet=%f", d_Rn_wet);
	G_message("LE_wet=%f", d_Rn_wet - d_g0_wet);
	G_message("t0dem_dry=%f", d_t0dem_dry);
	G_message("rnet_dry=%f", d_Rn_dry);
	G_message("g0_dry=%f", d_g0_dry);
	G_message("h0_dry=%f", d_Rn_dry - d_g0_dry);
    }/* END OF FLAG2 */

    G_message("Passed here");

    /* MANUAL T0DEM WET/DRY PIXELS */
    /*DRY PIXEL */
    if (flag3->answer) {
	/*Calculate coordinates of row/col from projected ones */
	row = (int)((ymax - m_row_dry) / (double)stepy);
	col = (int)((m_col_dry - xmin) / (double)stepx);
	G_message("Dry Pixel | row:%i col:%i", row, col);
    }
    else {
	row = (int)m_row_dry;
	col = (int)m_col_dry;
	G_message("Dry Pixel | row:%i col:%i", row, col);
    }
    rowDry = row;
    colDry = col;
    Rast_get_d_row(infd_Rn, inrast_Rn, row);
    Rast_get_d_row(infd_g0, inrast_g0, row);
    Rast_get_d_row(infd_t0dem, inrast_t0dem, row);
    d_Rn_dry = ((DCELL *) inrast_Rn)[col];
    d_g0_dry = ((DCELL *) inrast_g0)[col];
    d_t0dem_dry = ((DCELL *) inrast_t0dem)[col];
    /*WET PIXEL */
    if (flag3->answer) {
	/*Calculate coordinates of row/col from projected ones */
	row = (int)((ymax - m_row_wet) / (double)stepy);
	col = (int)((m_col_wet - xmin) / (double)stepx);
	G_message("Wet Pixel | row:%i col:%i", row, col);
    }
    else {
	row = m_row_wet;
	col = m_col_wet;
	G_message("Wet Pixel | row:%i col:%i", row, col);
    }
    rowWet = row;
    colWet = col;
    Rast_get_d_row(infd_t0dem, inrast_t0dem, row);
    d_t0dem_wet = ((DCELL *) inrast_t0dem)[col];
    /* END OF MANUAL WET/DRY PIXELS */
    double h_dry;

    h_dry = d_Rn_dry - d_g0_dry;
    G_message("h_dry = %f", h_dry);
    G_message("t0dem_dry = %f", d_t0dem_dry);
    G_message("t0dem_wet = %f", d_t0dem_wet);
    DCELL d_rah_dry;
    DCELL d_roh_dry;

    /* INITIALIZATION */
    for (row = 0; row < nrows; row++) {
	DCELL d_t0dem,d_z0m;
	DCELL d_rah1,d_roh1;
	DCELL d_u5;
	G_percent(row, nrows, 2);
	/* read a line input maps into buffers */
	Rast_get_d_row(infd_z0m, inrast_z0m, row);
	Rast_get_d_row(infd_t0dem, inrast_t0dem,row);
	/* read every cell in the line buffers */
	for (col = 0; col < ncols; col++) {
            d_z0m = ((DCELL *) inrast_z0m)[col];
            d_t0dem = ((DCELL *) inrast_t0dem)[col];
	    if (Rast_is_d_null_value(&d_t0dem) || Rast_is_d_null_value(&d_z0m)) {
		/* do nothing */
		d_Roh[row][col] = -999.9;
		d_Rah[row][col] = -999.9;
	    }
	    else {
		d_u5 = (ustar / 0.41) * log(5 / d_z0m);
		d_rah1=(1/(d_u5*pow(0.41,2)))*log(5/d_z0m)*log(5/(d_z0m*0.1));
		d_roh1=((998-ea)/(d_t0dem*2.87))+(ea/(d_t0dem*4.61));
		if (d_roh1 > 5)  d_roh1 = 1.0;
		else d_roh1=((1000-4.65)/(d_t0dem*2.87))+(4.65/(d_t0dem*4.61));
		if (row == rowDry && col == colDry) {	/*collect dry pix info */
		    d_rah_dry = d_rah1;
		    d_roh_dry = d_roh1;
		    G_message("d_rah_dry=%f d_roh_dry=%f",d_rah_dry,d_roh_dry);
		}
		d_Roh[row][col] = d_roh1;
		d_Rah[row][col] = d_rah1;
	    }
	}
    }
    DCELL d_dT_dry;

    /*Calculate dT_dry */
    d_dT_dry = (h_dry * d_rah_dry) / (1004 * d_roh_dry);
    double a, b;

    /*Calculate coefficients for next dT equation */
    /*a = 1.0/ ((d_dT_dry-0.0) / (d_t0dem_dry-d_t0dem_wet)); */
    /*b = ( a * d_t0dem_wet ) * (-1.0); */
    double sumx = d_t0dem_wet + d_t0dem_dry;
    double sumy = d_dT_dry + 0.0;
    double sumx2 = pow(d_t0dem_wet, 2) + pow(d_t0dem_dry, 2);
    double sumxy = (d_t0dem_wet * 0.0) + (d_t0dem_dry * d_dT_dry);
    a = (sumxy - ((sumx * sumy) / 2.0)) / (sumx2 - (pow(sumx, 2) / 2.0));
    b = (sumy - (a * sumx)) / 2.0;
    G_message("d_dT_dry=%f", d_dT_dry);
    G_message("dT1=%f * t0dem + (%f)", a, b);
    DCELL d_h_dry;

    /* ITERATION 1 */
    for (row = 0; row < nrows; row++) {
	DCELL d_t0dem,d_z0m;
	DCELL d_h1,d_rah1,d_rah2,d_roh1;
	DCELL d_L,d_x,d_psih,d_psim;
	DCELL d_u5;
	G_percent(row, nrows, 2);
	/* read a line input maps into buffers */
	Rast_get_d_row(infd_z0m, inrast_z0m, row);
	Rast_get_d_row(infd_t0dem, inrast_t0dem,row);
	/* read every cell in the line buffers */
	for (col = 0; col < ncols; col++) {
            d_z0m = ((DCELL *) inrast_z0m)[col];
            d_t0dem = ((DCELL *) inrast_t0dem)[col];
	    d_rah1 = d_Rah[row][col];
	    d_roh1 = d_Roh[row][col];
	    if (Rast_is_d_null_value(&d_t0dem) || Rast_is_d_null_value(&d_z0m)) {
		/* do nothing */
	    }
	    else {
		if (d_rah1 < 1.0) 
		    d_h1 = 0.0;
		else 
		    d_h1 = (1004 * d_roh1) * (a * d_t0dem + b) / d_rah1;
		d_L =-1004*d_roh1*pow(ustar,3)*d_t0dem/(d_h1*9.81*0.41);
		d_x = pow((1-16*(5/d_L)),0.25);
		d_psim =2*log((1+d_x)/2)+log((1+pow(d_x,2))/2)-2*atan(d_x)+0.5*M_PI;
		d_psih =2*log((1+pow(d_x,2))/2);
		d_u5 =(ustar/0.41)*log(5/d_z0m);
		d_rah2 = (1/(d_u5*pow(0.41,2)))*log((5/d_z0m)-d_psim)
                        *log((5/(d_z0m*0.1))-d_psih);
		if (row == rowDry && col == colDry) {/*collect dry pix info */
		    d_rah_dry = d_rah2;
		    d_h_dry = d_h1;
		}
		d_Rah[row][col] = d_rah1;
	    }
	}
    }

    /*Calculate dT_dry */
    d_dT_dry = (d_h_dry * d_rah_dry) / (1004 * d_roh_dry);
    /*Calculate coefficients for next dT equation */
    /*      a = (d_dT_dry-0)/(d_t0dem_dry-d_t0dem_wet); */
    /*      b = (-1.0) * ( a * d_t0dem_wet ); */
    /*      G_message("d_dT_dry=%f",d_dT_dry); */
    /*      G_message("dT2=%f * t0dem + (%f)", a, b); */
    sumx = d_t0dem_wet + d_t0dem_dry;
    sumy = d_dT_dry + 0.0;
    sumx2 = pow(d_t0dem_wet, 2) + pow(d_t0dem_dry, 2);
    sumxy = (d_t0dem_wet * 0.0) + (d_t0dem_dry * d_dT_dry);
    a = (sumxy - ((sumx * sumy) / 2.0)) / (sumx2 - (pow(sumx, 2) / 2.0));
    b = (sumy - (a * sumx)) / 2.0;
    G_message("d_dT_dry=%f", d_dT_dry);
    G_message("dT1=%f * t0dem + (%f)", a, b);

    /* ITERATION 2 */
    /***************************************************/
    /***************************************************/
    for (row = 0; row < nrows; row++) {
	DCELL d_t0dem;
	DCELL d_z0m;
	DCELL d_rah2;
	DCELL d_rah3;
	DCELL d_roh1;
	DCELL d_h2;
	DCELL d_L;
	DCELL d_x;
	DCELL d_psih;
	DCELL d_psim;
	DCELL d_u5;
	G_percent(row, nrows, 2);
	/* read a line input maps into buffers */
	Rast_get_d_row(infd_z0m,inrast_z0m,row);
	Rast_get_d_row(infd_t0dem,inrast_t0dem,row);
	/* read every cell in the line buffers */
	for (col = 0; col < ncols; col++) {
            d_z0m = ((DCELL *) inrast_z0m)[col];
            d_t0dem = ((DCELL *) inrast_t0dem)[col];
	    d_rah2 = d_Rah[row][col];
	    d_roh1 = d_Roh[row][col];
	    if (Rast_is_d_null_value(&d_t0dem) || Rast_is_d_null_value(&d_z0m)) {
		/* do nothing */
	    }
	    else {
		if (d_rah2 < 1.0) {
		    d_h2 = 0.0;
		}
		else {
		    d_h2 =(1004*d_roh1)*(a*d_t0dem+b)/d_rah2;
		}
		d_L =-1004*d_roh1*pow(ustar,3)*d_t0dem/(d_h2*9.81*0.41);
		d_x = pow((1 - 16 * (5 / d_L)), 0.25);
		d_psim =2*log((1+d_x)/2)+log((1+pow(d_x,2))/2)-
		    2*atan(d_x)+0.5*M_PI;
		d_psih =2*log((1+pow(d_x,2))/2);
		d_u5 =(ustar/0.41)*log(5/d_z0m);
		d_rah3=(1/(d_u5*pow(0.41,2)))*log((5/d_z0m)-d_psim)*
                       log((5/(d_z0m*0.1))-d_psih);
		if (row == rowDry && col == colDry) {/*collect dry pix info */
		    d_rah_dry = d_rah2;
		    d_h_dry = d_h2;
		}
		d_Rah[row][col] = d_rah2;
	    }
	}
    }

    /*Calculate dT_dry */
    d_dT_dry = (d_h_dry * d_rah_dry) / (1004 * d_roh_dry);
    /*Calculate coefficients for next dT equation */
    /*      a = (d_dT_dry-0)/(d_t0dem_dry-d_t0dem_wet); */
    /*      b = (-1.0) * ( a * d_t0dem_wet ); */
    /*      G_message("d_dT_dry=%f",d_dT_dry); */
    /*      G_message("dT3=%f * t0dem + (%f)", a, b); */
    sumx = d_t0dem_wet + d_t0dem_dry;
    sumy = d_dT_dry + 0.0;
    sumx2 = pow(d_t0dem_wet, 2) + pow(d_t0dem_dry, 2);
    sumxy = (d_t0dem_wet * 0.0) + (d_t0dem_dry * d_dT_dry);
    a = (sumxy - ((sumx * sumy) / 2.0)) / (sumx2 - (pow(sumx, 2) / 2.0));
    b = (sumy - (a * sumx)) / 2.0;
    G_message("d_dT_dry=%f", d_dT_dry);
    G_message("dT1=%f * t0dem + (%f)", a, b);

    /* ITERATION 3 */
    /***************************************************/
    /***************************************************/

    for (row = 0; row < nrows; row++) {
	DCELL d_t0dem;
	DCELL d_z0m;
	DCELL d_rah3;
	DCELL d_roh1;
	DCELL d_h3;
	DCELL d_L;
	DCELL d_x;
	DCELL d_psih;
	DCELL d_psim;
	DCELL d;		/* Output pixel */
	G_percent(row, nrows, 2);
	/* read a line input maps into buffers */
	Rast_get_d_row(infd_z0m, inrast_z0m, row);
	Rast_get_d_row(infd_t0dem,inrast_t0dem,row);
	/* read every cell in the line buffers */
	for (col = 0; col < ncols; col++) {
            d_z0m = ((DCELL *) inrast_z0m)[col];
            d_t0dem = ((DCELL *) inrast_t0dem)[col];
	    d_rah3 = d_Rah[row][col];
	    d_roh1 = d_Roh[row][col];
	    if (Rast_is_d_null_value(&d_t0dem) || Rast_is_d_null_value(&d_z0m)) {
		Rast_set_d_null_value(&outrast[col], 1);
	    }
	    else {
		if (d_rah3 < 1.0) {
		    d_h3 = 0.0;
		}
		else {
		    d_h3 = (1004 * d_roh1) * (a * d_t0dem + b) / d_rah3;
		}
		if (d_h3 < 0 && d_h3 > -50) {
		    d_h3 = 0.0;
		}
		if (d_h3 < -50 || d_h3 > 1000) {
		    Rast_set_d_null_value(&outrast[col], 1);
		}
		outrast[col] = d_h3;
	    }
	}
	Rast_put_d_row(outfd, outrast);
    }


    G_free(inrast_z0m);
    Rast_close(infd_z0m);
    G_free(inrast_t0dem);
    Rast_close(infd_t0dem);

    G_free(outrast);
    Rast_close(outfd);

    /* add command line incantation to history file */
    Rast_short_history(h0, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(h0, &history);

    exit(EXIT_SUCCESS);
}
