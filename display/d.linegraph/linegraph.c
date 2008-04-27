/****************************************************************************
 *
 * MODULE:       d.linegraph
 * AUTHOR(S):    Chris Rewerts, Agricultural Engineering, Purdue University (original contributor)
 *               Markus Neteler <neteler itc.it>
 *               Roberto Flor <flor itc.it>, Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* Chris Rewerts
   rewerts@ecn.purdue.edu
   Agricultural Engineering, Purdue University
   February 1992
   program: d.linegraph
 
   This program is based on Raghaven Srinivasan's modification  
   of the programs written by Dave Johnson for d.histogram. 
   
   Will read files containing a column of numbers and create line
   graphs. One file can be used for the X axis, up to 10 for the 
   Y axis. Each numerical x,y file should be a single column of
   numbers.    
   */
   
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "linegraph.h"

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

/* the default order of precedence of colors to use for Y lines */
int default_y_colors[] = {
    0,
    RED, GREEN, VIOLET, BLUE, ORANGE,
    GRAY, BROWN, MAGENTA, WHITE, INDIGO
};

int 
main (int argc, char **argv)
{
    int xoffset;	/* offset for x-axis */
    int yoffset;	/* offset for y-axis */
    int text_height;
    int text_width;
    int i;
    int j;
    int c;
    int tic_every;
    int max_tics;
    int title_color;
    int num_y_files;
    int tic_unit;
    int t, b, l, r;
    int tt, tb, tl, tr;
    int prev_x, prev_y[11];
    int new_x, new_y[11];
    int line;
    int x_line[3];
    int y_line[3];
    int err;
    
    struct in_file
    {
       int num_pnts;		/* number of lines in file  */
       int color;		/* color to use for y lines */
       float max;		/* maximum value in file    */
       float min;		/* minimum value in file    */
       float value;		/* current value read in    */
       char name[1024]; 	/* name of file	     */
       char full_name[1024];	/* path/name of file	*/
       FILE *fp;		/* pointer to file	  */
    };
    
    struct in_file in[12];
    struct GModule *module;
    
    float max_y;
    float min_y;
    float height, width;
    float xscale;	
    float yscale;
    
    char txt[1024], xlabel[512];
    char tic_name[1024];
    char *name;
    char color_name[20];
    
    FILE  *fopen();
 
    struct Option *dir_opt, *x_opt, *y_opt;
    struct Option *y_color_opt;
    struct Option *title[3];
    struct Option *t_color_opt;
	  
/* Initialize the GIS calls */
    G_gisinit(argv[0]) ;

    /* Set description */
    module		= G_define_module();
    module->keywords	= _("display");
    module->description =
	_("Generates and displays simple line graphs in the active graphics monitor display frame.");
    
    x_opt	      = G_define_option() ;
    x_opt->key	      = "x_file" ;
    x_opt->description= _("Name of data file for X axis of graph");
    x_opt->type       = TYPE_STRING ;
    x_opt->required   = YES ;

    y_opt	      = G_define_option() ;
    y_opt->key	      = "y_file" ;
    y_opt->description= _("Name of data file(s) for Y axis of graph");
    y_opt->type       = TYPE_STRING ;
    y_opt->required   = YES ;
    y_opt->multiple   = YES ;

    dir_opt		= G_define_option() ;
    dir_opt->key	= "directory" ;
    dir_opt->description= _("Path to file location");
    dir_opt->type       = TYPE_STRING ;
    dir_opt->required   = NO ;
    dir_opt->answer     = ".";
   
    y_color_opt		    = G_define_option() ;
    y_color_opt->key	    = "y_color" ;
    y_color_opt->description= _("Color for Y data");
    y_color_opt->type       = TYPE_STRING ;
    y_color_opt->required   = NO ;
    y_color_opt->multiple   = YES;
    y_color_opt->answers    = NULL;
    y_color_opt->options    = D_COLOR_LIST;
    
    t_color_opt		= G_define_option() ;
    t_color_opt->key	    = "title_color" ;
    t_color_opt->description= _("Color for axis, tics, numbers, and title");
    t_color_opt->type       = TYPE_STRING ;
    t_color_opt->required   = NO ;
    t_color_opt->answer     = DEFAULT_FG_COLOR ;
    t_color_opt->options    = D_COLOR_LIST;

    title[0]		 = G_define_option() ;
    title[0]->key	 = "x_title" ;
    title[0]->description= _("Title for X data");
    title[0]->type       = TYPE_STRING ;
    title[0]->required   = NO ;
    title[0]->answer     = "";

    title[1]		 = G_define_option() ;
    title[1]->key	 = "y_title" ;
    title[1]->description= _("Title for Y data");
    title[1]->type       = TYPE_STRING ;
    title[1]->required   = NO ;
    title[1]->answer     = "";

    title[2]		 = G_define_option() ;
    title[2]->key	 = "title" ;
    title[2]->description= _("Title for Graph");
    title[2]->type       = TYPE_STRING ;
    title[2]->required   = NO ;
    title[2]->answer     = "";


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE) ;
 
    for (i = 0; i < 3; i++)
    {
    for (j = 0; j < strlen(title[i]->answer); j++)
	if(title[i]->answer[j] == '_')
	    title[i]->answer[j] = ' ';
    }

/* build path to X data file and open for reading
   notice that in[0] will be the X file, and in[1-10]
   will be the Y file(s) */

    sprintf(in[0].full_name, "%s/%s", dir_opt->answer, x_opt->answer);
    sprintf(in[0].name, "%s", x_opt->answer);

    if ((in[0].fp = fopen (in[0].full_name, "r")) == NULL)
	G_fatal_error(_("Unable to open input file <%s>"), in[0].full_name);

    num_y_files = 0;
    
/* open all Y data files */

    for (i = 0, j = 1; (name = y_opt->answers[i]); i++, j++)
    {
	 sprintf(in[j].full_name, "%s/%s", dir_opt->answer, name);
	 sprintf(in[j].name, "%s", name);

	if ((in[j].fp = fopen (in[j].full_name, "r")) == NULL)
	    G_fatal_error(_("Unable to open input file <%s>"), in[j].full_name);

	num_y_files++;
	if(num_y_files > 10)
	    G_fatal_error(_("Maximum of 10 Y data files exceeded"));
    }
    
/* set colors  */

    title_color = D_translate_color(t_color_opt->answer) ;

/* I had an argument with the parser, and couldn't get a neat list of
   the input colors as I thought I should. I did a quick hack to get
   my list from the answer var, which gives us the colors input
   separated by commas. at least we know that they have been checked against
   the list of possibles */
    c = 0;
    j = 1;
    if (y_color_opt->answer != NULL)
    {
	for (i =0; i <= (strlen(y_color_opt->answer)); i++)
       {
	   if((y_color_opt->answer[i] == ',') || 
	   (i == (strlen(y_color_opt->answer))))
	   {
	       color_name[c] = '\0';
	       in[j].color = D_translate_color(color_name);
	       j++;
	       c = 0;
	   }
	   else
	   {
	       color_name[c++] = y_color_opt->answer[i];
	   }
	}
/* this is lame. I could come up with a color or prompt for one or something */  
	if(j < num_y_files)
	    G_fatal_error(_("Only <%d> colors given for <%d> lines"), j, num_y_files);
     }
     else
/* no colors given on command line, use default list */
     {
	 for (i = 1; i <= num_y_files; i++)
	 {
	     in[i].color = default_y_colors[i];
	 }
     }
     
/* get coordinates of current screen window, in pixels */
    if (R_open_driver() != 0)
	    G_fatal_error (_("No graphics device selected"));
    D_get_screen_window (&t, &b, &l, &r);
    R_set_window (t, b, l, r);

/* create axis lines, to be drawn later */
    height = b - t;
    width = r - l;
    x_line[0] = x_line[1] = l + (int) (ORIGIN_X * width);
    x_line[2] = l + (int) (XAXIS_END * width);
    y_line[0] = b - (int) (YAXIS_END * height);
    y_line[1] = y_line[2] = b - (int) (ORIGIN_Y * height);
    text_height = (int) (b - t) * TEXT_HEIGHT;
    text_width = (int) (r - l) * TEXT_WIDTH;
    R_text_size (text_width, text_height);

/* read thru each data file in turn, find max and min values for
   each, count lines, find x min and max, find overall y min and
   max */
   
    max_y = -99999.9;
    min_y =  99999.9;

    for (i = 0; i <= num_y_files; i++)
    {
    
	in[i].min = 99999.9;
	in[i].max = -99999.9;
	in[i].value = 0.0;
	in[i].num_pnts = 0;
	    
	while((err = fscanf(in[i].fp, "%f", &in[i].value)) != EOF)
	{
	    in[i].num_pnts++;
	    in[i].max = MAX(in[i].max, in[i].value);
	    in[i].min = MIN(in[i].min, in[i].value);
	    if (i > 0) /* if we have a y file */
	    {
	       min_y = MIN(min_y, in[i].value);
	       max_y = MAX(max_y, in[i].value);
	    }
	}
	if ((i > 0) && (in[0].num_pnts != in[i].num_pnts))
	{
	    G_warning(_("Y input file <%s> contains %s data points than the X input file"),
		in[i].name, ((in[i].num_pnts < in[0].num_pnts) ? "fewer" : "more"));

	    if (in[i].num_pnts > in[0].num_pnts)
		G_message(_("The last %d point(s) will be ignored"),
			(in[i].num_pnts - in[0].num_pnts));
	}
    }

/* close all files */

    for (i = 0; i <= num_y_files; i++)
	fclose(in[i].fp);
    
/* figure scaling factors and offsets */

    xscale = ((double) (x_line[2] - x_line[1]) / 
    (double) (in[0].num_pnts));
    yscale = ((double) (y_line[1] - y_line[0]) / (max_y - min_y));
    yoffset = (double) (y_line[1]);
    xoffset = (double) (x_line[1]);

/* figure tic_every and tic_units for the x-axis of the bar-chart.
   tic_every tells how often to place a tic-number.  tic_unit tells
   the unit to use in expressing tic-numbers. */
    
	if (xscale < XTIC_DIST)
	{
		max_tics = (x_line[2]-x_line[1])/XTIC_DIST;
		i=1;
		while (((in[0].max-in[0].min)/tics[i].every) > max_tics)
			i++;
		tic_every = tics[i].every;
		tic_unit = tics[i].unit;
		strcpy(tic_name,tics[i].name);
	}
	else
	{   
		tic_every = 1;
		tic_unit = 1;
		strcpy(tic_name, "");
	}

 
 /* open all the data files again */

    for (i = 0; i <= num_y_files; i++)
    {
	if ((in[i].fp = fopen (in[i].full_name, "r")) == NULL)
	{
	    sprintf (txt, "Could not open input file <%s>.", in[i].full_name);
	    death (txt);
	}
    }

/* loop through number of lines in x data file, 
   then loop thru for each y file, drawing a piece of each line and a
   legend bar on each iteration evenly divisible, a tic-mark
   on those evenly divisible by tic_unit, and a tic_mark
   number on those evenly divisible by tic_every   */

/* read the info from the inputs */

    for (line = 0; line < in[0].num_pnts; line++)
    {
/* scan in an X value */
    	err = fscanf(in[0].fp, "%f", &in[0].value);
    	
/* didn't find a number or hit EOF before our time */
    	if ((err != 1) || (err == EOF))  
    	{
    	    sprintf(txt, _("Problem reading X data file at line %d"), line);
    	    death(txt);
    	}

/* for each Y data file, get a value and compute where to draw it */
	for (i = 1; i <= num_y_files; i++)
	{
/* check to see that we do indeed have data for this point */    
	   if (line < in[i].num_pnts)
	   {
	    err = fscanf(in[i].fp, "%f", &in[i].value);
	    if ((in[i].num_pnts >= line) && (err != 1))
	    {
    		sprintf(txt, _("Problem reading <%s> data file at line %d"), 
    		in[i].name, line);
    		death(txt);
	    }
	    
/* in case the Y file has fewer lines than the X file, we will skip
   trying to draw when we run out of data */
   
/* draw increment of each Y file's data */

		R_standard_color (in[i].color);

/* find out position of where Y should be drawn. */
/* if our minimum value of y is not negative, this is easy */

		if (min_y >= 0)
		    new_y[i] = (int)(yoffset - yscale * (in[i].value - min_y));
		    
/* if our minimum value of y is negative, then we have two
   cases:  our current value to plot is pos or neg */
   
		else
		{
		    if (in[i].value < 0)
			new_y[i] = (int)(yoffset - yscale * (-1 * 
			(min_y - in[i].value)));
		    else
			new_y[i] = (int)(yoffset - yscale * (in[i].value +
			(min_y * -1)));
		}

		new_x = xoffset + (line * xscale);
		if (line == 0)
		{
		    prev_x = xoffset;
		    prev_y[i] = yoffset;
		}
		R_move_abs (prev_x, prev_y[i]);
		R_cont_abs (new_x, new_y[i]);
		prev_y[i] = new_y[i];
	     }
	    }
	    prev_x = new_x;

/* draw x-axis tic-marks and numbers */

	if (rem ((long int) in[0].value, tic_every) == (float) 0)
	{

/* draw a numbered tic-mark */

	    R_standard_color (title_color);
	    R_move_abs ((int) (xoffset + line * xscale),
		(int) (b - ORIGIN_Y * (b - t)));
	    R_cont_rel ((int) 0, (int) (BIG_TIC * (b - t)));
	    if ((in[0].value >= 1) || (in[0].value <= -1) || (in[0].value == 0))
		sprintf (txt, "%.0f", (in[0].value/tic_unit));
	    else
		sprintf (txt, "%.2f", (in[0].value));
	    text_height = (b - t) * TEXT_HEIGHT;
	    text_width = (r - l) * TEXT_WIDTH;
	    R_text_size (text_width, text_height);
	    R_get_text_box (txt, &tt, &tb, &tl, &tr);
	    while ((tr - tl) > XTIC_DIST)
	    {
		text_width *= 0.75;
		text_height *= 0.75;
		R_text_size (text_width, text_height);
		R_get_text_box (txt, &tt, &tb, &tl, &tr);
	    }
	    R_move_abs ((int) (xoffset + (line * xscale - (tr - tl) / 2)),
		(int) (b - XNUMS_Y * (b - t)));
	    R_text (txt);
	}
	else if (rem (line, tic_unit) == (float) 0)
	{

/* draw a tic-mark */

	    R_standard_color (title_color);
	    R_move_abs ((int) (xoffset + line * xscale),
		(int) (b - ORIGIN_Y * (b - t)));
	    R_cont_rel ((int) 0, (int) (SMALL_TIC * (b - t)));
	}
    }
    
/* close all input files */
    for (i = 0; i <= num_y_files; i++)
    {
	fclose(in[i].fp);
    }

/* draw the x-axis label */
    if ((strcmp(title[0]->answer, "") == 0) && (strcmp(tic_name, "") == 0))
	*xlabel = '\0';
    else
	sprintf (xlabel, "X: %s %s", title[0]->answer, tic_name);
    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH * 1.5;
    R_text_size (text_width, text_height);
    R_get_text_box (xlabel, &tt, &tb, &tl, &tr);
    R_move_abs ((int) (l + (r - l) / 2 - (tr - tl) / 2), 
    (int) (b - LABEL_1 * (b - t)));
    R_standard_color (title_color);
    R_text (xlabel);

/* DRAW Y-AXIS TIC-MARKS AND NUMBERS
   first, figure tic_every and tic_units for the x-axis of the bar-chart.
   tic_every tells how often to place a tic-number.  tic_unit tells
   the unit to use in expressing tic-numbers. */
   
	if (yscale < YTIC_DIST)
	{
		max_tics = (y_line[1]-y_line[0])/YTIC_DIST;
		i=1;
		while (((max_y-min_y)/tics[i].every) > max_tics)
			i++;
		tic_every = tics[i].every;
		tic_unit = tics[i].unit;
		strcpy(tic_name,tics[i].name);
	}
	else
	{   
		tic_every = 1;
		tic_unit = 1;
		strcpy(tic_name,"");
	}

/* Y-AXIS LOOP */

	for (i = (int)min_y; i <= (int)max_y; i += tic_unit)
	{
	    if (rem (i, tic_every) == (float) 0)
	    {
/* draw a tic-mark */

		R_move_abs ((int) x_line[0], (int) (yoffset - yscale * (i - min_y)));
		R_cont_rel ((int) (-(r - l) * BIG_TIC), (int) 0);

/* draw a tic-mark number */

		sprintf (txt, "%d", (i/tic_unit));
		text_height = (b - t) * TEXT_HEIGHT;
		text_width = (r - l) * TEXT_WIDTH;
		R_text_size (text_width, text_height);
		R_get_text_box (txt, &tt, &tb, &tl, &tr);
		while ((tt - tb) > YTIC_DIST)
		{
		    text_width *= 0.75;
		    text_height *= 0.75;
		    R_text_size (text_width, text_height);
		    R_get_text_box (txt, &tt, &tb, &tl, &tr);
		}
		R_move_abs ((int) (l + (r - l) * YNUMS_X - (tr - tl) / 2),
		(int) (yoffset - (yscale * (i - min_y) + 0.5 * (tt - tb))));
		R_text (txt);
	    }
	    else if (rem (i, tic_unit) == (float) 0)
	    {
/* draw a tic-mark */

		R_move_abs ((int) x_line[0], 
		(int) (yoffset - yscale * (i - min_y)));
		R_cont_rel ((int) (-(r - l) * SMALL_TIC), (int) 0);
	    }
	}

/* draw the y-axis label */
    if ((strcmp(title[1]->answer, "") == 0) && (strcmp(tic_name, "") == 0))
	*xlabel = '\0';
    else
	sprintf (xlabel, "Y: %s %s", title[1]->answer, tic_name);
    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH * 1.5;
    R_text_size (text_width, text_height);
    R_get_text_box (xlabel, &tt, &tb, &tl, &tr);
    R_move_abs((int) (l + (r - l) / 2 - (tr - tl) / 2), 
    (int) (b - LABEL_2 * (b - t)));
    R_standard_color (title_color);
    R_text (xlabel);
    
/* top label */
    sprintf (xlabel, title[2]->answer);
    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH * 2.0;
    R_text_size (text_width, text_height);
    R_get_text_box (xlabel, &tt, &tb, &tl, &tr);
/*
    R_move_abs((int)(((r-l)/2)-(tr-tl)/2),
    (int) (t+ (b-t)*.07) );
*/
    R_move_abs((int) (l + (r - l) / 2 - (tr - tl) / 2), 
    (int) (t+ (b-t)*.07) );
    R_standard_color (title_color);
    R_text (xlabel);

/* draw x and y axis lines */
    R_standard_color (title_color);
    R_polyline_abs (x_line, y_line, 3);

    R_flush ();
    R_close_driver ();
    exit(EXIT_SUCCESS);
}

float rem (long int x, long int y)
{
    long int d = x / y;
    return ((float) (x - y * d));
}


/* a function for making an exit after the R_driver is open */
int death (char *gasp)
{
    R_flush ();
    R_close_driver ();
    G_fatal_error("%s", gasp);
}

