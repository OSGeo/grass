/****************************************************************************
 *
 * MODULE:       r.bilinear
 *
 * AUTHOR(S):    Bill Brown - CERL (Thu Jul 27, 1995)
 *
 * PURPOSE:      Bilinear interpolation for raster map layers
 *
 * COPYRIGHT:    (C) 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

typedef int FILEDESC;


int main( int argc, char *argv[])
{
    struct GModule *module;
    struct Option 	*rastin, *rastout;
    struct Option 	*eastoff, *northoff;
    char                *inmap;
    FILEDESC    	infile = (FILEDESC)NULL, outfile = (FILEDESC)NULL;
    DCELL		*inbuf1, *inbuf2;
    DCELL		*outbuf;
    int			row, col; 
    struct Cell_head    w, mapw;
    float               o_east, o_north;


    G_gisinit (argv[0]);

	module = G_define_module();
	module->keywords = _("raster");
    module->description =
		_("Bilinear interpolation utility for raster map layers.");

    rastin = G_define_standard_option(G_OPT_R_INPUT);

    rastout = G_define_standard_option(G_OPT_R_OUTPUT);

    northoff = G_define_option();
    northoff->key                    = "north";
    northoff->type                   = TYPE_DOUBLE;
    northoff->description            = _("Specific input value to be assigned to the north and/or south poles for longitude-latitude grids");
    northoff->required               = NO;

    eastoff = G_define_option();
    eastoff->key                    = "east";
    eastoff->type                   = TYPE_DOUBLE;
    eastoff->description            = _("Specific input value to be assigned to the north and/or south poles for longitude-latitude grids");
    eastoff->required               = NO;

    if (G_parser (argc, argv))
	exit (EXIT_FAILURE);

    G_warning(_("This module is deprecated. Please use 'r.resamp.interp' instead."));

    o_east = o_north = 0.0;

    if(northoff->answer)
	sscanf(northoff->answer, "%f", &o_north);

    if(eastoff->answer)
	sscanf(eastoff->answer, "%f", &o_east);

    G_get_set_window (&w);

    inmap = G_find_file2 ("cell", rastin->answer, "");
    if(!inmap){
	G_fatal_error(_("Raster map <%s> not found"), rastin->answer);
    }

    /* set window to old map */
    G_get_cellhd(rastin->answer, inmap, &mapw);
    G_set_window(&mapw);

    /* G_malloc 2 buffers for input rows */
    inbuf1 = G_allocate_d_raster_buf();
    inbuf2 = G_allocate_d_raster_buf();

    /* open old map */
    if ((infile = G_open_cell_old(rastin->answer, inmap)) == -1)
    {
	G_fatal_error(_("Unable to open raster map <%s>"), rastin->answer);
    }

    /* reset window to current region */
    G_set_window(&w);
 
    outbuf = G_allocate_d_raster_buf();

    /* open new map */
    if ((outfile = G_open_raster_new(rastout->answer, DCELL_TYPE)) < 0)
    {
	G_fatal_error(_("Unable to create raster map <%s>"), rastout->answer);
    }
    G_suppress_warnings(1);
    /* otherwise get complaints about window changes */
    
    {
	double east, north, mapeast, mapnorth, t, u, new;
	DCELL *tmp, c1, c2, c3, c4;
	int maprow1, mapcol1, maprow2, mapcol2;
	int bufrow1, bufrow2;
	double maprow_f, mapcol_f;
	
	bufrow1 = bufrow2 = -1;
	for (row = 0; row < w.rows; row++) {

	    G_percent(row, w.rows - 1, 2);

	    north = G_row_to_northing((double)(row +.5), &w); 
	    north += o_north;
	    
	    /* in region? */
	    if(north > mapw.north || north <= mapw.south){
		G_set_d_null_value(outbuf, w.cols);
		G_put_d_raster_row(outfile, outbuf);
		continue;
	    }

	    /* on edges? */
	    if (north >= (mapw.north - mapw.ns_res/2.)){
		maprow1 = 0;
		maprow2 = 1;
	        u = 0.0;
	    }
	    else if (north <= (mapw.south + mapw.ns_res/2.)){
		maprow2 = mapw.rows - 1;
		maprow1 = maprow2 - 1;
	        u = 1.0;
	    }
	    
	    else{
		maprow_f = G_northing_to_row(north,&mapw);
		if((int)(maprow_f + .5) == (int)maprow_f){
		    maprow2 = (int)maprow_f;
		    maprow1 = maprow2 - 1;
		}
		else{
		    maprow1 = (int)maprow_f;
		    maprow2 = maprow1 + 1;
		}
		mapnorth=G_row_to_northing((double)(maprow1 +.5), &mapw);
		u = (mapnorth - north)/mapw.ns_res; 
	    }
/*
fprintf(stderr,"row %d maprow1 %d maprow2 %d\n", row, maprow1, maprow2);
*/

	    if(bufrow1 < 0){  /* read both */
		G_set_window(&mapw);
		G_get_d_raster_row(infile, inbuf1, maprow1);
		bufrow1 = maprow1;
		G_get_d_raster_row(infile, inbuf2, maprow2);
		bufrow2 = maprow2;
		G_set_window(&w);
	    }
	    else if(maprow1 != bufrow1 || maprow2 != bufrow2){
		/* still might be able to reuse row2 */
		if(maprow1 == bufrow2){
		    tmp = inbuf1;
		    inbuf1 = inbuf2;
		    inbuf2 = tmp;
		    bufrow1 = maprow1;
		    G_set_window(&mapw);
		    G_get_d_raster_row(infile, inbuf2, maprow2);
		    bufrow2 = maprow2;
		    G_set_window(&w);
		}
		else{ /* need to read both */
		    G_set_window(&mapw);
		    G_get_d_raster_row(infile, inbuf1, maprow1);
		    bufrow1 = maprow1;
		    G_get_d_raster_row(infile, inbuf2, maprow2);
		    bufrow2 = maprow2;
		    G_set_window(&w);
		}
	    }

	    for(col=0; col < w.cols; col++){
		east = G_col_to_easting((double)(col +.5), &w); 
		east += o_east;

		/* in region? */
		if(east < mapw.west || east >= mapw.east){
		    G_set_d_null_value(&outbuf[col], 1);
		    continue;
		}

		/* on edges? */
		if (east <= (mapw.west + mapw.ew_res/2.)){
		    mapcol1 = 0;
		    mapcol2 = 1;
		    t = 0.0;
		}
		else if (east >= (mapw.east - mapw.ew_res/2.)){
		    mapcol2 = mapw.cols - 1;
		    mapcol1 = mapcol2 - 1;
		    t = 1.0;
		}

		else{
		    mapcol_f = G_easting_to_col(east,&mapw);
		    if((int)(mapcol_f + .5) == (int)mapcol_f){
			mapcol2 = (int)mapcol_f;
			mapcol1 = mapcol2 - 1;
		    }
		    else{
			mapcol1 = (int)mapcol_f;
			mapcol2 = mapcol1 + 1;
		    }
		    mapeast=G_col_to_easting((double)(mapcol1 +.5), &mapw);
		    t = (east - mapeast)/mapw.ew_res; 
		}

		c1 = inbuf1[mapcol1];
		c2 = inbuf1[mapcol2];
		c3 = inbuf2[mapcol1];
		c4 = inbuf2[mapcol2];

		if (G_is_d_null_value(&c1) ||
		    G_is_d_null_value(&c2) ||
		    G_is_d_null_value(&c3) ||
		    G_is_d_null_value(&c4))
		{
			G_set_d_null_value(&outbuf[col], 1);
		}
		else
		{
		    new = (1.0 - t) * (1.0 - u) * c1 +
			t * (1.0 - u) * c2 +
			t * u * c4 +
			u * (1.0 - t) * c3;

		    outbuf[col] = new;
		}
	    }
	    G_put_d_raster_row(outfile, outbuf);
	}
    }

    G_free(inbuf1);
    G_free(inbuf2);
    G_free(outbuf);

    G_close_cell(infile);
    G_close_cell(outfile);
    
    return(EXIT_SUCCESS);

}
