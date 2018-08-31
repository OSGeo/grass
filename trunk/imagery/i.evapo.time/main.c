
/****************************************************************************
 *
 * MODULE:       i.evapo.time
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * 		 Ines Cherif - icherif@yahoo.com
 * PURPOSE:      Integrate in time the evapotranspiration from satellite,
 *		 following a daily pattern from meteorological ETo.
 *
 * COPYRIGHT:    (C) 2008-2009, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU Lesser General Public
 *   	    	 License. Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define MAXFILES 400

int main(int argc, char *argv[])
{
    struct Cell_head cellhd;	/*region+header info */
    char *mapset;		/*mapset name */
    int nrows, ncols;
    int row, col;
    struct GModule *module;
    struct Option *input, *input1, *input2, *input3, *input4, *input5, *output;
    struct History history;	/*metadata */
    struct Colors colors;	/*Color rules */

    /************************************/
    char *name, *name1, *name2;	/*input raster name */
    char *result;		/*output raster name */

    /*File Descriptors */
    int nfiles, nfiles1, nfiles2;
    int infd[MAXFILES], infd1[MAXFILES], infd2[MAXFILES];
    int outfd;

    /****************************************/
    /* Pointers for file names              */
    char **names;
    char **ptr;
    char **names1;
    char **ptr1;
    char **names2;
    char **ptr2;

    /****************************************/
    int DOYbeforeETa[MAXFILES], DOYafterETa[MAXFILES];
    int bfr, aft;

    /****************************************/
    int ok;
    int i = 0, j = 0;
    double etodoy;		/*minimum ETo DOY */
    double startperiod, endperiod;  /*first and last days (DOYs) of the period studied */
    void *inrast[MAXFILES], *inrast1[MAXFILES], *inrast2[MAXFILES];
    DCELL *outrast;
    CELL val1, val2;
    
    RASTER_MAP_TYPE in_data_type[MAXFILES];	/* ETa */
    RASTER_MAP_TYPE in_data_type1[MAXFILES];	/* DOY of ETa */
    RASTER_MAP_TYPE in_data_type2[MAXFILES];	/* ETo */
    RASTER_MAP_TYPE out_data_type = DCELL_TYPE;

    /************************************/
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("evapotranspiration"));
    module->description =_("Computes temporal integration of satellite "
			   "ET actual (ETa) following the daily ET reference "
			   "(ETo) from meteorological station(s).");
    
    /* Define the different options */
    input = G_define_standard_option(G_OPT_R_INPUTS);
    input->key = "eta";
    input->description = _("Names of satellite ETa raster maps [mm/d or cm/d]");

    input1 = G_define_standard_option(G_OPT_R_INPUTS);
    input1->key = "eta_doy";
    input1->description =
	_("Names of satellite ETa Day of Year (DOY) raster maps [0-400] [-]");

    input2 = G_define_standard_option(G_OPT_R_INPUTS);
    input2->key = "eto";
    input2->description =
	_("Names of meteorological station ETo raster maps [0-400] [mm/d or cm/d]");

    input3 = G_define_option();
    input3->key = "eto_doy_min";
    input3->type = TYPE_DOUBLE;
    input3->required = YES;
    input3->description = _("Value of DOY for ETo first day");
    
    input4 = G_define_option();
    input4->key = "start_period";
    input4->type = TYPE_DOUBLE;
    input4->required = YES;
    input4->description = _("Value of DOY for the first day of the period studied");

    input5 = G_define_option();
    input5->key = "end_period";
    input5->type = TYPE_DOUBLE;
    input5->required = YES;
    input5->description = _("Value of DOY for the last day of the period studied");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    
    /* init nfiles */
    nfiles = 1;
    nfiles1 = 1;
    nfiles2 = 1;

    /********************/

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    ok = 1;
    names = input->answers;
    ptr = input->answers;
    names1 = input1->answers;
    ptr1 = input1->answers;
    names2 = input2->answers;
    ptr2 = input2->answers;
    etodoy = atof(input3->answer);
    startperiod = atof(input4->answer);
    endperiod = atof(input5->answer);
    result = output->answer;

    /****************************************/
    if (endperiod<startperiod) {
	G_fatal_error(_("The DOY for end_period can not be smaller than start_period"));
	ok = 0;
    }
    if (etodoy>startperiod) {
	G_fatal_error(_("The DOY for start_period can not be smaller than eto_doy_min"));
	ok = 0;
    }
    for (; *ptr != NULL; ptr++) {
	if (nfiles > MAXFILES)
	    G_fatal_error(_("Too many ETa files. Only %d allowed."),
			  MAXFILES);
	name = *ptr;
	/* Allocate input buffer */
	infd[nfiles] = Rast_open_old(name, "");
	Rast_get_cellhd(name, "", &cellhd);
	inrast[nfiles] = Rast_allocate_d_buf();
	nfiles++;
    }
    nfiles--;
    if (nfiles <= 1)
	G_fatal_error(_("The min specified input map is two"));
	
	/****************************************/
    for (; *ptr1 != NULL; ptr1++) {
	if (nfiles1 > MAXFILES)
	    G_fatal_error(_("Too many ETa_doy files. Only %d allowed."),
			  MAXFILES);
	name1 = *ptr1;
	/* Allocate input buffer */
	infd1[nfiles1] = Rast_open_old(name1, "");
	Rast_get_cellhd(name1, "", &cellhd);
	inrast1[nfiles1] = Rast_allocate_d_buf();
	nfiles1++;
    }
    nfiles1--;
    if (nfiles1 <= 1)
	G_fatal_error(_("The min specified input map is two"));


	/****************************************/
    if (nfiles != nfiles1)
	G_fatal_error(_("ETa and ETa_DOY file numbers are not equal!"));

	/****************************************/

    for (; *ptr2 != NULL; ptr2++) {
	if (nfiles > MAXFILES)
	    G_fatal_error(_("Too many ETo files. Only %d allowed."),
			  MAXFILES);
	name2 = *ptr2;
	/* Allocate input buffer */
	infd2[nfiles2] = Rast_open_old(name2, "");
	Rast_get_cellhd(name2, "", &cellhd);
	inrast2[nfiles2] = Rast_allocate_d_buf();
	nfiles2++;
    }
    nfiles2--;
    if (nfiles2 <= 1)
	G_fatal_error(_("The min specified input map is two"));

    /* Allocate output buffer, use input map data_type */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_d_buf();

    /* Create New raster files */
    outfd = Rast_open_new(result, 1);

    /*******************/
    /* Process pixels */
    double doy[MAXFILES];
    double sum[MAXFILES];

    for (row = 0; row < nrows; row++) 
    {
	DCELL d_out;
	DCELL d_ETrF[MAXFILES];
	DCELL d[MAXFILES];
	DCELL d1[MAXFILES];
	DCELL d2[MAXFILES];
	G_percent(row, nrows, 2);

	/* read input map */
	for (i = 1; i <= nfiles; i++) 
	    Rast_get_d_row(infd[i], inrast[i], row);
	
	for (i = 1; i <= nfiles1; i++) 
	    Rast_get_d_row(infd1[i], inrast1[i], row);

	for (i = 1; i <= nfiles2; i++) 
	    Rast_get_d_row (infd2[i], inrast2[i], row);

	/*process the data */
	for (col = 0; col < ncols; col++) 
        {
            int	d1_null=0;
            int	d_null=0;
	    for (i = 1; i <= nfiles; i++) 
            {
		    if (Rast_is_d_null_value(&((DCELL *) inrast[i])[col]))
		    	d_null=1;
		    else
	                d[i] = ((DCELL *) inrast[i])[col];
	    }
	    for (i = 1; i <= nfiles1; i++) 
            {
		    if (Rast_is_d_null_value(&((DCELL *) inrast1[i])[col]))
			d1_null=1;
		    else
	                d1[i] = ((DCELL *) inrast1[i])[col];
	    }

	    for (i = 1; i <= nfiles2; i++) 
		    d2[i] = ((DCELL *) inrast2[i])[col];

	    /* Find out the DOY of the eto image    */
	    for (i = 1; i <= nfiles1; i++) 
            {
		if ( d_null==1 || d1_null==1 )
			Rast_set_d_null_value(&outrast[col],1);	
		else
		{
			doy[i] = d1[i] - etodoy+1;
			if (Rast_is_d_null_value(&d2[(int)doy[i]]) || d2[(int)doy[i]]==0 )
				Rast_set_d_null_value(&outrast[col],1);
			else
				d_ETrF[i] = d[i] / d2[(int)doy[i]];
		} 
	    }

	    for (i = 1; i <= nfiles1; i++) 
            {
		/* do nothing	*/
		if ( d_null==1 || d1_null==1)
                {
			/*G_message("  null value ");*/
                }
		else
		{
			DOYbeforeETa[i]=0; DOYafterETa[i]=0;
			if (i == 1)   
				DOYbeforeETa[i] = startperiod;
			else
			{
 				int k=i-1;
				while (d1[k]>=startperiod )
				{
					if (d1[k]<0)	 /* case were d1[k] is null */
						k=k-1;					
					else
					{
						DOYbeforeETa[i] = 1+((d1[i] + d1[k])/2.0);
						break;
					}			
				}

			}
	
			if (i == nfiles1)  
				DOYafterETa[i] = endperiod;
			else
			{
				int k=i+1;
				while (d1[k]<=endperiod)
				{
					if (d1[k]<0)   /* case were d1[k] is null */
						k=k+1;
					else
					{
						DOYafterETa[i] = (d1[i] + d1[k]) / 2.0;
						break;
	   				}					
				}
			}
		}	
	    }

	    sum[MAXFILES] = 0.0;
	    for (i = 1; i <= nfiles1; i++) 
            {
		if(d_null==1 || d1_null==1)
                {
		    /* do nothing	 */
		} 
                else
                {
			if (DOYbeforeETa[i]==0 || DOYbeforeETa[i]==0 ) 	
                            Rast_set_d_null_value(&outrast[col],1);
			else 
                        {
				bfr = (int)DOYbeforeETa[i];
				aft = (int)DOYafterETa[i];
				sum[i]=0.0;
				for (j = bfr; j < aft; j++) 
					sum[i] += d2[(int)(j-etodoy+1)];
			}
		}
	    }
	
	    d_out = 0.0;
	    for (i = 1; i <= nfiles1; i++)
            {
		if(d_null==1 || d_null==1)
			Rast_set_d_null_value(&outrast[col],1);
		else
                {	
			d_out += d_ETrF[i] * sum[i];
		     	outrast[col] = d_out;
		}	
	    }
	}
	Rast_put_row(outfd, outrast, out_data_type);
    }

    for (i = 1; i <= nfiles; i++) {
	G_free(inrast[i]);
	Rast_close(infd[i]);
    }
    for (i = 1; i <= nfiles1; i++) {
	G_free(inrast1[i]);
	Rast_close(infd1[i]);
    }
    for (i = 1; i <= nfiles2; i++) {
	G_free(inrast2[i]);
	Rast_close(infd2[i]);
    }
    G_free(outrast);
    Rast_close(outfd);

    /* Color table from 0.0 to 10.0 */
    Rast_init_colors(&colors);
    val1 = 0;
    val2 = 10;
    Rast_add_c_color_rule(&val1, 0, 0, 0, &val2, 255, 255, 255, &colors);
    /* Metadata */
    Rast_short_history(result, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result, &history);

    exit(EXIT_SUCCESS);
}
