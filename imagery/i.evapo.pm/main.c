/* Created by Anjuta version 1.0.2 */

/****************************************************************************
 *
 * MODULE:       i.evapo.PM
 * AUTHOR(S):    Massimiliano Cannata - massimiliano.cannata AT supsi.ch
 *               Maria A. Brovelli
 * PURPOSE:      Originally r.evapo.PM from HydroFOSS
 *               Calculates the Penman-Monteith reference evapotranspiration 
 *               and Open Water Evaporation. 
 *
 * COPYRIGHT:    (C) 2006-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct Cell_head cellhd;

    /* buffer for in out raster */
    DCELL *inrast_T, *inrast_RH, *inrast_u2;
    DCELL *inrast_Rn, *inrast_DEM, *inrast_hc, *outrast;
    unsigned char *EPo;

    int nrows, ncols;
    int row, col;
    int infd_T, infd_RH, infd_u2, infd_Rn, infd_DEM, infd_hc;
    int outfd;

    char *mapset_T, *mapset_RH, *mapset_u2;
    char *mapset_Rn, *mapset_DEM, *mapset_hc;
    char *T, *RH, *u2, *Rn, *DEM, *hc;
    DCELL d_T, d_RH, d_u2, d_Rn, d_Z, d_hc;
    DCELL d_EPo;

    int d_night;

    struct History history;
    struct GModule *module;
    struct Option *input_DEM, *input_T, *input_RH;
    struct Option *input_u2, *input_Rn, *input_hc, *output;
    struct Flag *flag1, *day, *zero;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->description =
	_("Potontial Evapotranspiration Calculation with hourly Penman-Monteith");

    /* Define different options */
    input_DEM = G_define_option();
    input_DEM->key = "DEM";
    input_DEM->type = TYPE_STRING;
    input_DEM->required = YES;
    input_DEM->gisprompt = "old,cell,raster";
    input_DEM->description = _("Name of DEM raster map [m a.s.l.]");

    input_T = G_define_option();
    input_T->key = "T";
    input_T->type = TYPE_STRING;
    input_T->required = YES;
    input_T->gisprompt = "old,cell,raster";
    input_T->description = _("Name of Temperature raster map [°C]");

    input_RH = G_define_option();
    input_RH->key = "RU";
    input_RH->type = TYPE_STRING;
    input_RH->required = YES;
    input_RH->gisprompt = "old,cell,raster";
    input_RH->description = _("Name of Relative Umidity raster map [%]");

    input_u2 = G_define_option();
    input_u2->key = "WS";
    input_u2->type = TYPE_STRING;
    input_u2->required = YES;
    input_u2->gisprompt = "old,cell,raster";
    input_u2->description = _("Name of Wind Speed raster map [m/s]");

    input_Rn = G_define_option();
    input_Rn->key = "NSR";
    input_Rn->type = TYPE_STRING;
    input_Rn->required = YES;
    input_Rn->gisprompt = "old,cell,raster";
    input_Rn->description =
	_("Name of Net Solar Radiation raster map [MJ/m2/h]");

    input_hc = G_define_option();
    input_hc->key = "Vh";
    input_hc->type = TYPE_STRING;
    input_hc->required = YES;
    input_hc->gisprompt = "old,cell,raster";
    input_hc->description = _("Name of crop height raster map [m]");

    output = G_define_option();
    output->key = "EPo";
    output->type = TYPE_STRING;
    output->required = YES;
    output->gisprompt = "new,cell,raster";
    output->description =
	_("Name of output Reference Potential Evapotranspiration layer [mm/h]");

    /* Define the different flags */
    //flag1 = G_define_flag() ;
    //flag1->key         = 'q' ;
    //flag1->description = "Quiet" ;

    zero = G_define_flag();
    zero->key = 'z';
    zero->description = _("set negative evapo to zero");

    day = G_define_flag();
    day->key = 'n';
    day->description = _("night-time");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* get entered parameters */
    T = input_T->answer;
    RH = input_RH->answer;
    u2 = input_u2->answer;
    Rn = input_Rn->answer;
    EPo = output->answer;
    DEM = input_DEM->answer;
    hc = input_hc->answer;

    if (day->answer) {
	d_night = TRUE;
    }
    else {
	d_night = FALSE;
    }

    /* check legal output name */
    if (G_legal_filename(EPo) < 0)
	G_fatal_error(_("[%s] is an illegal name"), EPo);

    /* determine the input map type (CELL/FCELL/DCELL) */
    //data_type = G_raster_map_type(T, mapset);

    infd_T = Rast_open_old(T, "");
    infd_RH = Rast_open_old(RH, "");
    infd_u2 = Rast_open_old(u2, "");
    infd_Rn = Rast_open_old(Rn, "");
    infd_DEM = Rast_open_old(DEM, "");
    infd_hc = Rast_open_old(hc, "");

    Rast_get_cellhd(T, "", &cellhd);
    Rast_get_cellhd(RH, "", &cellhd);
    Rast_get_cellhd(u2, "", &cellhd);
    Rast_get_cellhd(Rn, "", &cellhd);
    Rast_get_cellhd(DEM, "", &cellhd);
    Rast_get_cellhd(hc, "", &cellhd);

    /* Allocate input buffer */
    inrast_T = Rast_allocate_d_buf();
    inrast_RH = Rast_allocate_d_buf();
    inrast_u2 = Rast_allocate_d_buf();
    inrast_Rn = Rast_allocate_d_buf();
    inrast_DEM = Rast_allocate_d_buf();
    inrast_hc = Rast_allocate_d_buf();

    /* Allocate output buffer */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_d_buf();

    outfd = Rast_open_new(EPo, DCELL_TYPE);

    for (row = 0; row < nrows; row++) {

	/* read a line input maps into buffers */
	Rast_get_d_row(infd_T, inrast_T, row);
	Rast_get_d_row(infd_RH, inrast_RH, row);
	Rast_get_d_row(infd_u2, inrast_u2, row);
	Rast_get_d_row(infd_Rn, inrast_Rn, row);
	Rast_get_d_row(infd_DEM, inrast_DEM, row);
	Rast_get_d_row(infd_hc, inrast_hc, row);

	/* read every cell in the line buffers */
	for (col = 0; col < ncols; col++) {
	    d_T = ((DCELL *) inrast_T)[col];
	    d_RH = ((DCELL *) inrast_RH)[col];
	    d_u2 = ((DCELL *) inrast_u2)[col];
	    d_Rn = ((DCELL *) inrast_Rn)[col];
	    d_Z = ((DCELL *) inrast_DEM)[col];
	    d_hc = ((DCELL *) inrast_hc)[col];

	    //calculate evapotranspiration
	    if (d_hc < 0) {
		//calculate evaporation
		d_EPo =
		    calc_openwaterETp(d_T, d_Z, d_u2, d_Rn, d_night, d_RH,
				      d_hc);
	    }
	    else {
		//calculate evapotranspiration
		d_EPo = calc_ETp(d_T, d_Z, d_u2, d_Rn, d_night, d_RH, d_hc);
	    }

	    if (zero->answer && d_EPo < 0)
		d_EPo = 0;

	    ((DCELL *) outrast)[col] = d_EPo;
	}
	Rast_put_d_row(outfd, outrast);
    }
    G_free(inrast_T);
    G_free(inrast_RH);
    G_free(inrast_u2);
    G_free(inrast_Rn);
    G_free(inrast_DEM);
    G_free(inrast_hc);
    G_free(outrast);
    Rast_close(infd_T);
    Rast_close(infd_RH);
    Rast_close(infd_u2);
    Rast_close(infd_Rn);
    Rast_close(infd_DEM);
    Rast_close(infd_hc);
    Rast_close(outfd);

    /* add command line incantation to history file */
    Rast_short_history(EPo, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(EPo, &history);

    exit(EXIT_SUCCESS);
}
