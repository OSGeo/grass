/* Function: ps_colortable
**
** Author: Paul W. Carlson	April 1992
*/

#include <grass/glocale.h>

#include "ps_info.h"
#include "colortable.h"

#define NSTEPS 5 /* number of steps to divide color box when showing color for
		    category data range */

int PS_colortable (void)
{
    char *label;
    int num_cats;
    int i, j, k, jj;
    int R, G, B;
    int center_cols;
    DCELL dmin, dmax, val;
    struct Colors colors;
    double t, l, r;
    double x1, x2, y, dy, fontsize, tl;
    double col_width;
    int do_color;
    double grey_color_val;

    /* let user know what's happenning */
    G_message (_("Creating color table for <%s in %s>..."),
	       ct.name, ct.mapset); 

    if (G_read_cats(ct.name, ct.mapset, &PS.cats) == -1)
    {
	G_warning(_("Category file for <%s> not available"), ct.name);
	return 1;
    }

    if (G_read_colors(ct.name, ct.mapset, &colors) == -1)
	G_warning(_("Unable to read colors for colorbar"));

    do_color = (PS.grey == 0 && PS.level == 2);

    /* How many categories to show */
    num_cats = G_number_of_raster_cats(&PS.cats);
    G_debug(3, "clrtbl: %d categories", num_cats);
    if(!num_cats) {
	G_warning(_("Your cats/ file is invalid. A cats/ file with "
	  "categories and labels is required for 'colortable' when using "
	  "CELL rasters. No colortable will be assigned to this output "
	  "postscript file."));
	return 1;
    }

    /* set font */
    fontsize = (double)ct.fontsize;
    fprintf(PS.fp, "(%s) FN %.1f SF\n", ct.font, fontsize);

    /* set colortable location */
    dy = 1.5 * fontsize;

    if (ct.y < PS.top_marg) {
	G_warning(_("Colorbar y location beyond page margins. Adjusting."));
	ct.y = PS.top_marg;
    }
    t = 72.0 * ( PS.page_height - ct.y);

    if (ct.x < PS.left_marg) {
	G_warning(_("Colorbar x location beyond page margins. Adjusting."));
	ct.x = PS.left_marg + 0.1;
    }
    l = 72.0 * ct.x + 0.5;

    if (ct.width <= 0.0 || ct.width > PS.page_width  - PS.right_marg - ct.x)
        ct.width = PS.page_width  - PS.right_marg - ct.x;

    r  = l + 72.0 * ct.width;
    col_width = ct.width / (double)ct.cols;

    G_debug(3, "clrtbl: adjusted ct.x=[%.3f]  ct.y=[%.3f]  ct.width=[%.3f]",
	ct.x, ct.y, ct.width);

    /* read cats into PostScript array "a" */
    fprintf(PS.fp, "/a [\n");
    for(i = 0; i <= num_cats; i++)
    {
	if ( !i && !ct.nodata )  i++; /* step over 'no data' */
	if(!i)
	    fprintf(PS.fp, "(%s)\n", "no data");
        else
            fprintf(PS.fp, "(%s)\n", 
		  G_get_ith_d_raster_cat (&PS.cats, i-1, &dmin, &dmax));
    }		  
    fprintf(PS.fp, "] def\n");

    /* get width of widest string in PostScript variable "mw" */
    fprintf(PS.fp, "/mw 0 def 0 1 a length 1 sub { /i XD\n");
    fprintf(PS.fp, "a i get SW pop /t XD t mw gt {/mw t def} if } for\n");

    /* shrink font size to fit in width */
    if(ct.cols == 1) tl = 72.0 * col_width - 2.0 * fontsize; 
    else tl = 72.0 * col_width - 4.0 * fontsize; 
    fprintf(PS.fp, "/s %.1f def\n", fontsize);
    fprintf(PS.fp, "mw %.1f gt {/s s %.1f mul mw div def } if\n", tl, tl);
    fprintf(PS.fp, "(%s) FN s SF\n", ct.font);

    /* make proc to center multiple columns */
    center_cols = (ct.cols > 1);
    if (center_cols)
    {
    	fprintf(PS.fp, "/k %d def\n", ct.cols - 1);
    	fprintf(PS.fp, "/mlw 0 def 0 k a length 1 sub { /i XD\n");
    	fprintf(PS.fp, "a i get SW pop /t XD t mlw gt {/mlw t def} if } for\n");
	fprintf(PS.fp, "/xo mw mlw sub D2 s mul %1.0f div %1.0f add def\n", 
		fontsize, fontsize);
	fprintf(PS.fp, "/mvx {xo add} BD\n");
    }

    y = t - fontsize;
    k = 0;
    for(i = 0; i <= num_cats;)
    {
	if ( !i && !ct.nodata ) i++; /* step over 'no data' */

	/* test for bottom of page */
	y -= dy;
	if (y < 72.0 * PS.bot_marg) 
	{
	    y = 72.0 * (PS.page_height - PS.top_marg) - 0.5 * fontsize;
	    fprintf(PS.fp, "showpage\n");
	}

	for (j = 0; j < ct.cols; j++)
	{
	    /* get the data range */

	    /* fill box and outline in black */
	    if(i)
	       label = G_get_ith_d_raster_cat (&PS.cats, i-1, &dmin, &dmax);

	    x1 = l + (double)j * 72.0 * col_width;
	    x2 = x1 + fontsize;

	    if(!i || dmax==dmin)
	    /* draw a 1-color rectangle */
	    {
	       /* set box fill color */
	       if(!i)
		  G_get_null_value_color(&R, &G, &B, &colors);
               else
	          G_get_d_raster_color(&dmin, &R, &G, &B, &colors);

		if(do_color)
		    fprintf(PS.fp, "%.3f %.3f %.3f C\n", 
			(double)R/255., (double)G/255., (double)B/255.);
		else {
		    grey_color_val = (.3 * (double)R + .59 * (double)G + .11 * (double)B)/255.;
		    fprintf(PS.fp, "%.3f setgray\n", grey_color_val);
		}

               fprintf(PS.fp, "%.1f ", x1);
	       if (center_cols) fprintf(PS.fp, "mvx ");
	       fprintf(PS.fp, "%.1f ", y);
	       fprintf(PS.fp, "%.1f ", x2);
	       if (center_cols) fprintf(PS.fp, "mvx ");
	       fprintf(PS.fp, "%.1f ", y + fontsize);
	       fprintf(PS.fp, "B F BW stroke\n");
            }
	    else
	    /* split the rectangle into NSTEPS horisontal strips and
	       draw each with the corresponding value's color */
	    {
	       for(jj=0; jj<NSTEPS; jj++)
	       {
   	          /* set box fill color */
	          val = dmin + (double) jj * (dmax-dmin)/NSTEPS;
	          G_get_d_raster_color(&val, &R, &G, &B, &colors);
		  fprintf(PS.fp, "%.3f %.3f %.3f C\n",
			  (double)R/255., (double)G/255., (double)B/255.);
    	          fprintf(PS.fp, "%.1f ", x1);
    	          if (center_cols) fprintf(PS.fp, "mvx ");
    	          fprintf(PS.fp, "%.1f ", 
			 y + (fontsize * (double) jj) / NSTEPS);
    	          fprintf(PS.fp, "%.1f ", x2);
    	          if (center_cols) fprintf(PS.fp, "mvx ");
    	          fprintf(PS.fp, "%.1f ", 
			 y + (fontsize * (double) (jj+1)) / NSTEPS);
    	          fprintf(PS.fp, "B CF stroke\n");
	       } /* done filling the box */

	    /* outline the box in black */
               fprintf(PS.fp, "%.1f ", x1);
	       if (center_cols) fprintf(PS.fp, "mvx ");
	       fprintf(PS.fp, "%.1f ", y);
	       fprintf(PS.fp, "%.1f ", x2);
	       if (center_cols) fprintf(PS.fp, "mvx ");
	       fprintf(PS.fp, "%.1f ", y + fontsize);
	       fprintf(PS.fp, "B BW stroke\n");
	    } /* done drawing the box */

	    /* do the text */
            fprintf(PS.fp, "a %d get %.1f ", k++, x1 + 2.0 * fontsize);
    	    if (center_cols) fprintf(PS.fp, "mvx ");
            fprintf(PS.fp, "%.1f MS\n", y);
            i++;
	    if (i >= num_cats) j = ct.cols + 1;
	}
    }
    y -= dy;
    if (PS.min_y > y) PS.min_y = y;

    G_free_colors(&colors);

    return 0;
}
