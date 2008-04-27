/****************************************************************************
 *
 * MODULE:       r.out.vrml
 * AUTHOR(S):    Bill Brown (CERL/UIUC GMSL Laboratory) (original contributor), 
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      This module exports a GRASS raster map to the Virtual Reality
 *               Modeling Language (VRML) format for 3D visualization.
 * COPYRIGHT:    (C) 2006-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "pv.h"


struct Cell_head W;

int main(int argc, char *argv[])
{

    struct Option 	*rast_el, *rast_co, *out;
    struct Option       *exag_opt;
    char		*t_mapset;
    FILEDESC    	elevfd = 0, colorfd = 0;
    FILE                *vout = NULL;
    struct Colors       colr;
    char 		errbuf[100], outfile[256];
    int                 shh, color_ok;
    double              exag, min, max;
    struct GModule	*module;


    G_gisinit (argv[0]);
    shh = color_ok = 0;

    module = G_define_module();
    module->keywords = _("raster, export, VRML");
    module->description =
	_("Export a raster map to the Virtual Reality Modeling Language (VRML)");

    rast_el = G_define_option();
    rast_el->key            	= "elev";
    rast_el->type           	= TYPE_STRING;
    rast_el->required     	= YES;
    rast_el->gisprompt    	= "old,cell,raster";
    rast_el->description	= _("Name of elevation map");

    rast_co = G_define_option();
    rast_co->key            	= "color";
    rast_co->type           	= TYPE_STRING;
    rast_co->required     	= NO;
    rast_co->gisprompt    	= "old,cell,raster";
    rast_co->description	= _("Name of color file");

    exag_opt = G_define_option();
    exag_opt->key               = "exag";
    exag_opt->type              = TYPE_DOUBLE;
    exag_opt->required          = NO;
    exag_opt->answer            = "1.0";
    exag_opt->description	= _("Vertical exaggeration");

    out = G_define_option();
    out->key                    = "output";
    out->type                   = TYPE_STRING;
    out->required               = YES;
    out->gisprompt		= "new_file,file,output";
    out->description		= _("Name for new VRML file");

    if (G_parser (argc, argv))
	exit (EXIT_FAILURE);

    G_get_set_window (&W);

    t_mapset = NULL;
    t_mapset = G_find_file2 ("cell", rast_el->answer, "");
    if(!t_mapset){
	sprintf(errbuf,"Couldn't find raster map %s", rast_el->answer);
	G_fatal_error(errbuf);
    }
    if ((elevfd = G_open_cell_old(rast_el->answer, t_mapset)) == -1)
    {
	sprintf(errbuf,"Not able to open cellfile for [%s]", rast_el->answer);
	G_fatal_error(errbuf);
    }

    {
    CELL cmin, cmax;
    struct Range range ;
    int is_fp;
    DCELL dmin, dmax;
    struct FPRange fp_range ;

	is_fp = G_raster_map_is_fp(rast_el->answer, t_mapset);
	if(is_fp){
	    if (G_read_fp_range(rast_el->answer, t_mapset, &fp_range) != 1 ) {
		sprintf(errbuf,
			"Range info for [%s] not available (run r.support)\n",
			rast_el->answer);
		G_fatal_error(errbuf);
	    }
	    G_get_fp_range_min_max (&fp_range, &dmin, &dmax);
	    min = dmin;
	    max = dmax;
	}
	else{
	    if (G_read_range(rast_el->answer, t_mapset, &range) == -1) {
		sprintf(errbuf,
			"Range info for [%s] not available (run r.support)\n",
			rast_el->answer);
		G_fatal_error(errbuf);
	    }
	    G_get_range_min_max (&range, &cmin, &cmax);
	    min = cmin;
	    max = cmax;
	}
    }

    if(rast_co->answer){
	t_mapset = NULL;
	t_mapset = G_find_file2 ("cell", rast_co->answer, "");
	if(!t_mapset){
	    sprintf(errbuf,"Couldn't find raster map %s", rast_co->answer);
	    G_warning(errbuf);
	}
	else if ((colorfd = G_open_cell_old(rast_co->answer, t_mapset)) == -1)
	{
	    sprintf(errbuf,"Not able to open cellfile for [%s]", 
                    rast_co->answer);
	    G_warning(errbuf);
	}
	else{
	    G_read_colors (rast_co->answer, t_mapset, &colr);
	    color_ok = 1;
	}
    } 
 
    /* TODO: if file exists, just append new objects */
    if(out->answer){
	char *p;

	/* look for .wrl suffix, add if not found */
	if (NULL == (p=strrchr(out->answer,'.'))){ /* no suffix */
	    strcpy(outfile,out->answer);
	    strcat(outfile,".wrl");
	}
	else if(strncmp(p+1,"wrl",4)){ /* some other extension */
	    strcpy(outfile,out->answer);
	    strcat(outfile,".wrl");
	}
	else
	    strcpy(outfile,out->answer);

	/* open file for writing VRML */
	G_message(_("Opening %s for writing... "), outfile);
	if (NULL == (vout = fopen(outfile, "w"))){
	    sprintf(errbuf,"Couldn't open output file %s", outfile);
	    G_fatal_error(errbuf);
	}
    }
    
    exag = 1.0;
    if(exag_opt->answer){ 
	sscanf(exag_opt->answer,"%lf", &exag);
    }

    init_coordcnv(exag, &W, min, max);

    vrml_begin(vout);
/*
    vrml_put_view(vout, NULL);
*/
    vrml_put_grid(vout, &W, elevfd, colorfd, &colr, color_ok, 
		  W.rows, W.cols, shh);
    vrml_end(vout);


    G_close_cell(elevfd);
    if(color_ok)
	G_close_cell(colorfd);
    
    return(EXIT_SUCCESS);

}


static double scaleXZ, scaleY;
static double transX, transY, transZ;
static double Xrange,Yrange,Zrange;


/* REMEMBER - 
 * Y is HEIGHT 
 * Z is northing-W.south 
 * X is adjusted easting-W.west 
*/

/* 
 * This could be entered as a vrml scale to preserve real
 * geographic coords, but I'm not sure how good the average
 * vrml viewer is at setting appropriate z-depths, so testing
 * first like this. 
*/

/* TODO:
 * change scale & use G_distance for latlon to preserve meter units
*/

int
init_coordcnv(double exag, struct Cell_head *w, double min, double max)
{

    Yrange = (max-min) * exag;
    Zrange = (w->rows-1) * w->ns_res;
    Xrange = (w->cols-1) * w->ew_res;

    transX = -(G_col_to_easting (0.5, w));
    transZ = -(G_row_to_northing (0.5, w));
    transY = -min;

    if(Zrange>=Xrange && Zrange>=Yrange){ /* northing biggest */
	scaleXZ = 1.0/Zrange;
	scaleY = exag*scaleXZ;
    }
    else if(Xrange>=Zrange && Xrange>=Yrange){ /* easting biggest */
	scaleXZ = 1.0/Xrange;
	scaleY = exag*scaleXZ;
    }
    else                                 /* depth biggest */
	scaleY = scaleXZ = 1.0/Yrange;

   return 0;
}

int
do_coordcnv(double *dval, int axis)
{
double dret = 0.0;

    switch (axis){
	case 'x':
	case 'X':
	    dret = (*dval + transX) * scaleXZ;
	    break;
	case 'z':
	case 'Z':
	    dret = (Zrange - (*dval + transZ)) * scaleXZ;
	    break;
	case 'y':
	case 'Y':
	    dret = (*dval + transY) * scaleY;
	    break;
	default:
	    G_fatal_error("invalid axis: %c", axis);
	    break;
    }
    
    *dval = dret;
    return 0;
}




