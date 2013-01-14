/* d.legend a.k.a d.leg.thin
 *
 * MODULE:      d.legend
 *
 *              Based on the old d.leg.thin, which replaced an even older
 *              module called "d.legend".
 *
 * PURPOSE:     Draw a graphical legend for a raster on the display mon
 *
 * AUTHORS:
 *      Original version:
 *         Bill Brown, U.S. Army Construction Engineering Research Laboratories
 *      FP Support:
 *         Radim Blazek
 *      Merge of original "d.legend" code into d.leg.thin (now this module):
 *         Markus Neteler
 *      Late 2002: Rewrite of much of the code:
 *         Hamish Bowman, Otago University, New Zealand
 *
 * COPYRIGHT:   (c) 2006 The GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main(int argc, char **argv)
{
    char buff[512];
    char *map_name;
    int black;
    int cats_num;
    int color;
    int cur_dot_row;
    int do_cats;
    int dots_per_line;
    int thin;
    int i, j, k;
    int lines, steps;
    int fp;
    double t, b, l, r;
    int hide_catnum, hide_catstr, hide_nodata, do_smooth;
    char *cstr;
    int white;
    double x_box[5];
    double y_box[5];
    struct Categories cats;
    struct Colors colors;
    struct GModule *module;
    struct Option *opt_input, *opt_color, *opt_lines, *opt_thin, *opt_labelnum, *opt_at, *opt_use, *opt_range, *opt_font, *opt_path, *opt_charset, *opt_fontscale;
    struct Flag *hidestr, *hidenum, *hidenodata, *smooth, *flipit, *size;
    struct Range range;
    struct FPRange fprange;
    CELL min_ind, max_ind, null_cell;
    DCELL dmin, dmax, val;
    CELL min_colr, max_colr;
    DCELL min_dcolr, max_dcolr;
    int x0, x1, y0, y1, xyTemp;
    double X0, X1, Y0, Y1;
    int SigDigits;
    unsigned int MaxLabelLen;
    char DispFormat[5];		/*  %.Xf\0  */
    int flip, horiz, UserRange;
    double UserRangeMin, UserRangeMax, UserRangeTemp;
    double *catlist, maxCat;
    int catlistCount, use_catlist;
    double fontscale;


    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    module->description =
	_("Displays a legend for a raster map in the active frame "
	  "of the graphics monitor.");

    opt_input = G_define_standard_option(G_OPT_R_MAP);
    opt_input->description = _("Name of raster map");

    opt_color = G_define_standard_option(G_OPT_C_FG);
    opt_color->label = _("Text color");
    opt_color->guisection = _("Font settings");

    opt_lines = G_define_option();
    opt_lines->key = "lines";
    opt_lines->type = TYPE_INTEGER;
    opt_lines->answer = "0";
    opt_lines->options = "0-1000";
    opt_lines->description =
	_("Number of text lines (useful for truncating long legends)");
    opt_lines->guisection = _("Advanced");

    opt_thin = G_define_option();
    opt_thin->key = "thin";
    opt_thin->type = TYPE_INTEGER;
    opt_thin->required = NO;
    opt_thin->answer = "1";
    opt_thin->options = "1-1000";
    opt_thin->description = _("Thinning factor (thin=10 gives cats 0,10,20...)");
    opt_thin->guisection = _("Advanced");

    opt_labelnum = G_define_option();
    opt_labelnum->key = "labelnum";
    opt_labelnum->type = TYPE_INTEGER;
    opt_labelnum->answer = "5";
    opt_labelnum->options = "2-100";
    opt_labelnum->description = _("Number of text labels for smooth gradient legend");
    opt_labelnum->guisection = _("Gradient");

    opt_at = G_define_option();
    opt_at->key = "at";
    opt_at->key_desc = "bottom,top,left,right";
    opt_at->type = TYPE_DOUBLE;	/* needs to be TYPE_DOUBLE to get past options check */
    opt_at->required = NO;
    opt_at->options = "0-100";
    opt_at->label =
	_("Size and placement as percentage of screen coordinates (0,0 is lower left)");
    opt_at->description = opt_at->key_desc;
    opt_at->answer = NULL;

    opt_use = G_define_option();
    opt_use->key = "use";
    opt_use->type = TYPE_DOUBLE;	/* string as it is fed through the parser? */
    opt_use->required = NO;
    opt_use->description =
	_("List of discrete category numbers/values for legend");
    opt_use->multiple = YES;
    opt_use->guisection = _("Subset");

    opt_range = G_define_option();
    opt_range->key = "range";
    opt_range->key_desc = "min,max";
    opt_range->type = TYPE_DOUBLE;	/* should it be type_double or _string ?? */
    opt_range->required = NO;
    opt_range->description =
	_("Use a subset of the map range for the legend (min,max)");
    opt_range->guisection = _("Subset");

    opt_font = G_define_option();
    opt_font->key = "font";
    opt_font->type = TYPE_STRING;
    opt_font->required = NO;
    opt_font->description = _("Font name");
    opt_font->guisection = _("Font settings");

    /* HB: this option should become redundant as soon as the bug in trunk's D_text_size() is fixed. */
    opt_fontscale = G_define_option();
    opt_fontscale->key = "fontscale";
    opt_fontscale->type = TYPE_DOUBLE;
    opt_fontscale->required = NO;
    opt_fontscale->answer = "5";
    opt_fontscale->options = "0-100";
    opt_fontscale->description = _("Font scaling factor for floating point legends");
    opt_fontscale->guisection = _("Font settings");

    opt_path = G_define_option();
    opt_path->key = "path";
    opt_path->type = TYPE_STRING;
    opt_path->required = NO;
    opt_path->description = _("Path to font file");
    opt_path->gisprompt = "old_file,file,font";
    opt_path->guisection = _("Font settings");

    opt_charset = G_define_option();
    opt_charset->key = "charset";
    opt_charset->type = TYPE_STRING;
    opt_charset->required = NO;
    opt_charset->description =
	_("Text encoding (only applicable to TrueType fonts)");
    opt_charset->guisection = _("Font settings");

    hidestr = G_define_flag();
    hidestr->key = 'v';
    hidestr->description = _("Do not show category labels");
    hidestr->guisection = _("Advanced");

    hidenum = G_define_flag();
    hidenum->key = 'c';
    hidenum->description = _("Do not show category numbers");
    hidenum->guisection = _("Advanced");

    hidenodata = G_define_flag();
    hidenodata->key = 'n';
    hidenodata->description = _("Skip categories with no label");
    hidenodata->guisection = _("Advanced");

    smooth = G_define_flag();
    smooth->key = 's';
    smooth->description = _("Draw smooth gradient");
    smooth->guisection = _("Gradient");

    flipit = G_define_flag();
    flipit->key = 'f';
    flipit->description = _("Flip legend");
    flipit->guisection = _("Advanced");

    size = G_define_flag();
    size->key = 's';
    size->description = _("Font size is height in pixels");
    size->guisection = _("Font settings");

    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    map_name = opt_input->answer;

    hide_catstr = hidestr->answer;	/* note hide_catstr gets changed and re-read below */
    hide_catnum = hidenum->answer;
    hide_nodata = hidenodata->answer;
    do_smooth = smooth->answer;
    flip = flipit->answer;

    /* Create the fontscale factor for floating point legends */
    fontscale = atof(opt_fontscale->answer);
    fontscale = 1.0 / (100.0/fontscale);

    color = D_parse_color(opt_color->answer, TRUE);

    if (opt_lines->answer != NULL)
	sscanf(opt_lines->answer, "%d", &lines);

    thin = 1;
    if (opt_thin->answer != NULL)
	sscanf(opt_thin->answer, "%d", &thin);
    if (!thin)
	thin = 1;

    if (opt_labelnum->answer != NULL)
	sscanf(opt_labelnum->answer, "%d", &steps);

    catlistCount = 0;
    if (opt_use->answer != NULL) {	/* should this be answerS ? */
	use_catlist = TRUE;

	catlist = (double *)G_calloc(100 + 1, sizeof(double));
	for (i = 0; i < 100; i++)	/* fill with dummy values */
	    catlist[i] = 1.0 * (i + 1);
	catlist[i] = 0;

	for (i = 0; (opt_use->answers[i] != NULL) && i < 100; i++)
	    catlist[i] = atof(opt_use->answers[i]);

	catlistCount = i;
    }
    else
	use_catlist = FALSE;


    UserRange = FALSE;
    if (opt_range->answer != NULL) {	/* should this be answerS ? */
	sscanf(opt_range->answers[0], "%lf", &UserRangeMin);
	sscanf(opt_range->answers[1], "%lf", &UserRangeMax);
	UserRange = TRUE;
	if (UserRangeMin > UserRangeMax) {
	    UserRangeTemp = UserRangeMax;
	    UserRangeMax = UserRangeMin;
	    UserRangeMin = UserRangeTemp;
	    flip = !flip;
	}
    }


    if (Rast_read_colors(map_name, "", &colors) == -1)
	G_fatal_error(_("Color file for <%s> not available"), map_name);

    fp = Rast_map_is_fp(map_name, "");
    if (fp && !use_catlist) {
	do_smooth = TRUE;
	/* fprintf(stderr, "FP map found - switching gradient legend on\n"); */
	flip = !flip;
    }

    if (Rast_read_cats(map_name, "", &cats) == -1)
	G_warning(_("Category file for <%s> not available"), map_name);

    Rast_set_c_null_value(&null_cell, 1);

    if (D_open_driver() != 0)
      	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    
    white = D_translate_color(DEFAULT_FG_COLOR);
    black = D_translate_color(DEFAULT_BG_COLOR);
    
    if (opt_font->answer)
	D_font(opt_font->answer);
    else if (opt_path->answer)
	D_font(opt_path->answer);

    if (opt_charset->answer)
	D_encoding(opt_charset->answer);

    /* Figure out where to put text */
    D_setup_unity(0);
    D_get_src(&t, &b, &l, &r);

    if (opt_at->answer != NULL) {
	sscanf(opt_at->answers[0], "%lf", &Y1);
	sscanf(opt_at->answers[1], "%lf", &Y0);
	sscanf(opt_at->answers[2], "%lf", &X0);
	sscanf(opt_at->answers[3], "%lf", &X1);
    }
    else {			/* default */
	Y1 = 12;
	Y0 = 88;
	X0 = 3;
	X1 = 7;
    }

    x0 = l + (int)((r - l) * X0 / 100.);
    x1 = l + (int)((r - l) * X1 / 100.);
    y0 = t + (int)((b - t) * (100. - Y0) / 100.);	/* make lower left the origin */
    y1 = t + (int)((b - t) * (100. - Y1) / 100.);

    if (y0 > y1) {		/* allow for variety in order of corner */
	flip = !flip;		/*   selection without broken output    */
	xyTemp = y0;
	y0 = y1;
	y1 = xyTemp;
    }
    if (x0 > x1) {
	xyTemp = x0;
	x0 = x1;
	x1 = xyTemp;
    }

    if (x0 == x1)
	x1++;			/* avoid 0 width boxes */
    if (y0 == y1)
	y1++;

    if ((x0 < l) || (x1 > r) || (y0 < t) || (y1 > b))	/* for mouse or at= 0- or 100+; needs to be after order check */
	G_warning(_("Legend box lies outside of frame. Text may not display properly."));

    horiz = (x1 - x0 > y1 - y0);
    if (horiz)
	G_message(_("Drawing horizontal legend as box width exceeds height"));

    if (!fp && horiz)		/* better than nothing */
	do_smooth = TRUE;


    MaxLabelLen = 0;		/* init variable */

    /* How many categories to show */
    if (!fp) {
	if (Rast_read_range(map_name, "", &range) == -1)
	    G_fatal_error(_("Range information for <%s> not available (run r.support)"),
			  map_name);

	Rast_get_range_min_max(&range, &min_ind, &max_ind);
	if (Rast_is_c_null_value(&min_ind))
	    G_fatal_error(_("Input map contains no data"));

	Rast_get_c_color_range(&min_colr, &max_colr, &colors);

	if (UserRange) {
	    if (min_ind < UserRangeMin)
		min_ind = (int)ceil(UserRangeMin);
	    if (max_ind > UserRangeMax)
		max_ind = (int)floor(UserRangeMax);
	    if (min_ind > UserRangeMin) {
		min_ind =
		    UserRangeMin <
		    min_colr ? min_colr : (int)ceil(UserRangeMin);
		G_warning(_("Color range exceeds lower limit of actual data"));
	    }
	    if (max_ind < UserRangeMax) {
		max_ind =
		    UserRangeMax >
		    max_colr ? max_colr : (int)floor(UserRangeMax);
		G_warning(_("Color range exceeds upper limit of actual data"));
	    }
	}

	/*  cats_num is total number of categories in raster                  */
	/*  do_cats is  total number of categories to be displayed            */
	/*  k is number of cats to be displayed after skipping unlabeled cats */
	/*  lines is number of text lines/legend window                       */

	cats_num = max_ind - min_ind + 1;

	if (lines == 0)
	    lines = cats_num;

	do_cats = cats_num > lines ? lines : cats_num;

	if (do_cats == cats_num)
	    lines = (int)ceil((1.0 * lines) / thin);

	if (!use_catlist) {
	    catlist = (double *)G_calloc(lines + 1, sizeof(double));
	    catlistCount = lines;
	}
	/* see how many boxes there REALLY will be */
	maxCat = 0.0;
	for (i = min_ind, j = 1, k = 0; j <= do_cats && i <= max_ind;
	     j++, i += thin) {
	    if (!flip)
		cstr = Rast_get_c_cat(&i, &cats);
	    else {
		CELL cat = max_ind - (i - min_ind);
		cstr = Rast_get_c_cat(&cat, &cats);
	    }

	    if (!use_catlist)
		catlist[j - 1] = (double)i;

	    if (!cstr[0]) {	/* no cat label found, skip str output */
		if (hide_nodata)
		    continue;
	    }
	    else {		/* ie has a label */
		if (!hide_catstr && (MaxLabelLen < strlen(cstr)))
		    MaxLabelLen = strlen(cstr);
	    }

	    if (!hide_catnum)
		if (i > maxCat)
		    maxCat = (double)i;
	    k++;		/* count of actual boxes drawn (hide_nodata option invaidates using j-1) */
	}
	lines = k;

	/* figure out how long the category + label will be */
	if (use_catlist) {
	    MaxLabelLen = 0;
	    maxCat = 0;		/* reset */
	    for (i = 0, k = 0; i < catlistCount; i++) {
		if ((catlist[i] < min_ind) || (catlist[i] > max_ind)) {
		    G_fatal_error(_("use=%s out of range [%d,%d] (extend with range= ?)"),
				  opt_use->answers[i], min_ind, max_ind);
		}

		cstr = Rast_get_d_cat(&catlist[i], &cats);
		if (!cstr[0]) {	/* no cat label found, skip str output */
		    if (hide_nodata)
			continue;
		}
		else {		/* ie has a label */
		    if (!hide_catstr && (MaxLabelLen < strlen(cstr)))
			MaxLabelLen = strlen(cstr);
		}
		if (!hide_catnum)
		    if (catlist[i] > maxCat)
			maxCat = catlist[i];
		k++;
	    }
	    if (0 == k)		/* nothing to draw */
		lines = 0;
	}

	if (MaxLabelLen > 0) {	/* ie we've picked up at least one label */
	    MaxLabelLen++;	/* compensate for leading space */
	    if (!hide_catnum)
		MaxLabelLen += 3;	/* compensate for "%2d) " */
	}
	else {
	    if (!hide_catnum)
		MaxLabelLen = 1;
	}

	/* compensate for categories >100 */
	if (!hide_catnum) {
	    if (maxCat > 99)
		MaxLabelLen += (int)(log10(maxCat));
	}

	/* following covers both the above if(do_cats == cats_num) and k++ loop */
	if (lines < 1) {
	    lines = 1;		/* ward off the dpl floating point exception */
	    G_fatal_error(_("Nothing to draw! (no categories with labels? out of range?)"));
	}

	/* Figure number of lines, number of pixles per line and text size */
	dots_per_line = ((y1 - y0) / lines);

	/* switch to a smooth legend for CELL maps with too many cats */
	/*  an alternate solution is to set   dots_per_line=1         */
	if ((dots_per_line == 0) && (do_smooth == 0)) {
	    if (!use_catlist) {
		G_message(_("Forcing a smooth legend: too many categories for current window height"));
		do_smooth = 1;
	    }
	}

	/* center really tiny legends */
	if (opt_at->answer == NULL) {	/* if defualt scaling */
	    if (!do_smooth && (dots_per_line < 4))	/* if so small that there's no box */
		if ((b - (dots_per_line * lines)) / (b * 1.0) > 0.15)	/* if there's more than a 15% blank at the bottom */
		    y0 = ((b - t) - (dots_per_line * lines)) / 2;
	}

	/*      D_text_size((int)(dots_per_line*4/5), (int)(dots_per_line*4/5)) ;    redundant */
	/* if(Rast_is_c_null_value(&min_ind) && Rast_is_c_null_value(&max_ind))
	   {
	   min_ind = 1;
	   max_ind = 0;
	   } */

	if (horiz)
	    sprintf(DispFormat, "%%d");
	else {
	    if (maxCat > 0.0)
		sprintf(DispFormat, "%%%dd", (int)(log10(fabs(maxCat))) + 1);
	    else
		sprintf(DispFormat, "%%2d");
	}
    }
    else {			/* is fp */
	if (Rast_read_fp_range(map_name, "", &fprange) == -1)
	    G_fatal_error(_("Range information for <%s> not available"),
			  map_name);

	Rast_get_fp_range_min_max(&fprange, &dmin, &dmax);

	Rast_get_d_color_range(&min_dcolr, &max_dcolr, &colors);

	if (UserRange) {
	    if (dmin < UserRangeMin)
		dmin = UserRangeMin;
	    if (dmax > UserRangeMax)
		dmax = UserRangeMax;
	    if (dmin > UserRangeMin) {
		dmin = UserRangeMin < min_dcolr ? min_dcolr : UserRangeMin;
		G_warning(_("Color range exceeds lower limit of actual data"));
	    }
	    if (dmax < UserRangeMax) {
		dmax = UserRangeMax > max_dcolr ? max_dcolr : UserRangeMax;
		G_warning(_("Color range exceeds upper limit of actual data"));
	    }
	}

	if (use_catlist) {
	    for (i = 0; i < catlistCount; i++) {
		if ((catlist[i] < dmin) || (catlist[i] > dmax)) {
		    G_fatal_error(_("use=%s out of range [%.3f, %.3f] (extend with range= ?)"),
				  opt_use->answers[i], dmin, dmax);
		}
		if (strlen(opt_use->answers[i]) > MaxLabelLen)
		    MaxLabelLen = strlen(opt_use->answers[i]);
	    }
	}
	do_cats = 0;		/* if only to get rid of the compiler warning  */
	cats_num = 0;		/* if only to get rid of the compiler warning  */
	/* determine how many significant digits to display based on range */
	if (0 == (dmax - dmin))	/* trap divide by 0 for single value rasters */
	    sprintf(DispFormat, "%%f");
	else {
	    SigDigits = (int)ceil(log10(fabs(25 / (dmax - dmin))));
	    if (SigDigits < 0)
		SigDigits = 0;
	    if (SigDigits < 7)
		sprintf(DispFormat, "%%.%df", SigDigits);
	    else
		sprintf(DispFormat, "%%.2g");	/* eg 4.2e-9  */
	}
    }

    if (use_catlist) {
	cats_num = catlistCount;
	do_cats = catlistCount;
	lines = catlistCount;
	do_smooth = 0;
    }

    if (do_smooth) {
	int wleg, lleg, dx, dy;
	int txsiz;
	int ppl;
	int tcell;
	float ScaleFactor = 1.0;

	if (horiz) {
	    lleg = x1 - x0;
	    dx = 0;
	    dy = y1 - y0;
	    if (fp)
		flip = !flip;	/* horiz floats look better not flipped by default */
	}
	else {
	    lleg = y1 - y0;
	    dy = 0;
	    dx = x1 - x0;
	}

	/* Draw colors */
	for (k = 0; k < lleg; k++) {
	    if (!fp) {
		if (!flip)
		    tcell =
			min_ind + k * (double)(1 + max_ind - min_ind) / lleg;
		else
		    tcell =
			(max_ind + 1) - k * (double)(1 + max_ind -
						     min_ind) / lleg;
		D_color((CELL) tcell, &colors);
	    }
	    else {
		if (!flip)
		    val = dmin + k * (dmax - dmin) / lleg;
		else
		    val = dmax - k * (dmax - dmin) / lleg;
		D_d_color(val, &colors);
	    }

	    if (dx < dy)
		D_box_abs(x0 + k, y0, x0 + k + (dx ? -dx : 1),
			  y0 - (dy ? -dy : 1));
	    else
		D_box_abs(x0, y0 + k, x0 - (dx ? -dx : 1),
			  y0 + k + (dy ? -dy : 1));
	}

	/* Format text */
	if (!fp) {		/* cut down labelnum so they don't repeat */
	    if (do_cats < steps)
		steps = do_cats;
	    if (1 == steps)
		steps = 2;	/* ward off the ppl floating point exception */
	}

	for (k = 0; k < steps; k++) {
	    if (!fp) {
		if (!flip)
		    tcell =
			min_ind + k * (double)(max_ind - min_ind) / (steps -
								     1);
		else
		    tcell =
			max_ind - k * (double)(max_ind - min_ind) / (steps -
								     1);

		cstr = Rast_get_c_cat(&tcell, &cats);
		if (!cstr[0])	/* no cats found, disable str output */
		    hide_catstr = 1;
		else
		    hide_catstr = hidestr->answer;

		buff[0] = 0;	/* blank string */

		if (!hide_catnum) {	/* num */
		    sprintf(buff, DispFormat, tcell);
		    if (!hide_catstr)	/* both */
			strcat(buff, ")");
		}
		if (!hide_catstr)	/* str */
		    sprintf(buff + strlen(buff), " %s", cstr);
	    }
	    else {		/* ie FP map */
		if (hide_catnum)
		    buff[0] = 0;	/* no text */
		else {
		    if (!flip)
			val = dmin + k * (dmax - dmin) / (steps - 1);
		    else
			val = dmax - k * (dmax - dmin) / (steps - 1);

		    sprintf(buff, DispFormat, val);
		}
	    }

	    /* this probably shouldn't happen mid-loop as text sizes 
	       might not end up being uniform, but it's a start */
	    if (strlen(buff) > MaxLabelLen)
		MaxLabelLen = strlen(buff);

	    /* Draw text */
	    if (!horiz)
		txsiz = (int)((y1 - y0) * fontscale);
	    else
		txsiz = (int)((x1 - x0) * fontscale);

	    /* scale text to fit in window if position not manually set */
	    /* usually not needed, except when frame is really narrow   */
	    if (opt_at->answer == NULL) {	/* ie default scaling */
		ScaleFactor = ((r - x1) / ((MaxLabelLen + 1) * txsiz * 0.81));	/* ?? txsiz*.81=actual text width. */
		if (ScaleFactor < 1.0) {
		    txsiz = (int)(txsiz * ScaleFactor);
		}
	    }

	    if (txsiz < 0)
		txsiz = 0;	/* keep it sane */

	    D_text_size(txsiz, txsiz);
	    D_use_color(color);

	    ppl = (lleg) / (steps - 1);

	    if (!horiz) {
		if (!k)		/* first  */
		    D_pos_abs(x1 + 4, y0 + txsiz);
		else if (k == steps - 1)	/* last */
		    D_pos_abs(x1 + 4, y1);
		else
		    D_pos_abs(x1 + 4, y0 + ppl * k + txsiz / 2);
	    }
	    else {
		/* text width is 0.81 of text height? so even though we set width 
		   to txsiz with D_text_size(), we still have to reduce.. hmmm */
		if (!k)		/* first  */
		    D_pos_abs(x0 - (strlen(buff) * txsiz * .81 / 2),
			       y1 + 4 + txsiz);
		else if (k == steps - 1)	/* last */
		    D_pos_abs(x1 - (strlen(buff) * txsiz * .81 / 2),
			       y1 + 4 + txsiz);
		else
		    D_pos_abs(x0 + ppl * k -
			       (strlen(buff) * txsiz * .81 / 2),
			       y1 + 4 + txsiz);
	    }

	    if(color)
		D_text(buff);

	}			/*for */

	lleg = y1 - y0;
	wleg = x1 - x0;

	/* Black box */
	D_use_color(black);
	D_begin();
	D_move_abs(x0 + 1, y0 + 1);
	D_cont_rel(0, lleg - 2);
	D_cont_rel(wleg - 2, 0);
	D_cont_rel(0, 2 - lleg);
	D_close();
	D_end();
	D_stroke();

	/* White box */
	D_use_color(white);
	D_begin();
	D_move_abs(x0, y0);
	D_cont_rel(0, lleg);
	D_cont_rel(wleg, 0);
	D_cont_rel(0, -lleg);
	D_close();
	D_end();
	D_stroke();

    }
    else {			/* non FP, no smoothing */

	int txsiz, true_l, true_r;
	float ScaleFactor = 1.0;

	/* set legend box bounds */
	true_l = l;
	true_r = r;		/* preserve window width */
	l = x0;
	t = y0;
	r = x1;
	b = y1;

	D_pos_abs(x0, y0);

	/* figure out box height  */
	if (do_cats == cats_num)
	    dots_per_line = (b - t) / (lines + 1);	/* +1 line for the two 1/2s at top and bottom */
	else
	    dots_per_line = (b - t) / (lines + 2);	/* + another line for 'x of y categories' text */

	/* adjust text size */
	/*      txsiz = (int)((y1-y0)/(1.5*(lines+5))); */
	txsiz = (int)((y1 - y0) / (2.0 * lines));

	/* scale text to fit in window if position not manually set */
	if (opt_at->answer == NULL) {	/* ie defualt scaling */
	    ScaleFactor = ((true_r - true_l) / ((MaxLabelLen + 3) * txsiz * 0.81));	/* ?? txsiz*.81=actual text width. */
	    if (ScaleFactor < 1.0) {
		txsiz = (int)floor(txsiz * ScaleFactor);
		dots_per_line = (int)floor(dots_per_line * ScaleFactor);
	    }
	}

	if (dots_per_line < txsiz)
	    txsiz = dots_per_line;

	D_text_size(txsiz, txsiz);


	/* Set up box arrays */
	x_box[0] = 0;
	y_box[0] = 0;
	x_box[1] = 0;
	y_box[1] = (5 - dots_per_line);
	x_box[2] = (dots_per_line - 5);
	y_box[2] = 0;
	x_box[3] = 0;
	y_box[3] = (dots_per_line - 5);
	x_box[4] = (5 - dots_per_line);
	y_box[4] = 0;


	/* Draw away */

	/*              if(ScaleFactor < 1.0)   */
	/*                  cur_dot_row = ((b-t) - (dots_per_line*lines))/2; *//* this will center the legend */
	/*              else    */
	cur_dot_row = t + dots_per_line / 2;

	/*  j = (do_cats == cats_num ? 1 : 2 ); */

	for (i = 0, k = 0; i < catlistCount; i++)
	    /*              for(i=min_ind, j=1, k=0; j<=do_cats && i<=max_ind; j++, i+=thin)        */
	{
	    if (!flip)
		cstr = Rast_get_d_cat(&catlist[i], &cats);
	    else
		cstr = Rast_get_d_cat(&catlist[catlistCount - i - 1], &cats);


	    if (!cstr[0]) {	/* no cat label found, skip str output */
		hide_catstr = 1;
		if (hide_nodata)
		    continue;
	    }
	    else
		hide_catstr = hidestr->answer;

	    k++;		/* count of actual boxes drawn (hide_nodata option invaidates using j-1) */

	    /* White box */
	    cur_dot_row += dots_per_line;
	    D_use_color(white);
	    D_begin();
	    D_move_abs(l + 2, (cur_dot_row - 1));
	    D_cont_rel(0, (3 - dots_per_line));
	    D_cont_rel((dots_per_line - 3), 0);
	    D_cont_rel(0, (dots_per_line - 3));
	    D_close();
	    D_end();
	    D_stroke();

	    /* Black box */
	    D_use_color(black);
	    D_begin();
	    D_move_abs(l + 3, (cur_dot_row - 2));
	    D_cont_rel(0, (5 - dots_per_line));
	    D_cont_rel((dots_per_line - 5), 0);
	    D_cont_rel(0, (dots_per_line - 5));
	    D_close();
	    D_end();
	    D_stroke();

	    /* Color solid box */
	    if (!fp) {
		if (!flip)
		    D_color((CELL) (int)catlist[i], &colors);
		else
		    D_color((CELL) (int)catlist[catlistCount - i - 1],
			    &colors);
	    }
	    else {
		if (!flip)
		    D_d_color(catlist[i], &colors);
		else
		    D_d_color(catlist[catlistCount - i - 1], &colors);
	    }

	    D_pos_abs(l + 3, (cur_dot_row - 2));
	    D_polygon_rel(x_box, y_box, 5);

	    /* Draw text */
	    D_use_color(color);

	    if (!fp) {
		/* nothing, box only */
		buff[0] = 0;
		if (!hide_catnum) {	/* num */
		    sprintf(buff, DispFormat, (int)catlist[i]);
		    if (!flip)
			sprintf(buff, DispFormat, (int)catlist[i]);
		    else
			sprintf(buff, DispFormat,
				(int)catlist[catlistCount - i - 1]);
		    if (!hide_catstr)	/* both */
			strcat(buff, ")");
		}
		if (!hide_catstr)	/* str */
		    sprintf(buff + strlen(buff), " %s", cstr);
	    }
	    else {		/* is fp */
		if (!flip) {
		    if(use_catlist)
			/* pass through format exactly as given by the user in
			   the use= command line parameter (helps with log scale) */
			sprintf(buff, "%s", opt_use->answers[i]);
		    else 
			/* automatically generated/tuned decimal precision format */
			sprintf(buff, DispFormat, catlist[i]);
		}
		else {
		    if(use_catlist)
			sprintf(buff, "%s", opt_use->answers[catlistCount - i - 1]);
		    else
			sprintf(buff, DispFormat, catlist[catlistCount - i - 1]);
		}
	    }

	    D_pos_abs((l + 3 + dots_per_line), (cur_dot_row) - 3);

	    if(color)
		D_text(buff);
	}

	if (0 == k)
	    G_fatal_error(_("Nothing to draw! (no categories with labels?)"));	/* "(..., out of range?)" */


	if (do_cats != cats_num) {
	    cur_dot_row += dots_per_line;
	    /*          sprintf(buff, "%d of %d categories\n", (j-1), cats_num) ;   */

	    sprintf(buff, "%d of %d categories\n", k, cats_num);

	    /* shrink text if it will run off the screen */
	    MaxLabelLen = strlen(buff) + 4;
	    ScaleFactor = ((true_r - true_l) / (MaxLabelLen * txsiz * 0.81));	/* ?? txsiz*.81=actual text width. */
	    if (ScaleFactor < 1.0) {
		txsiz = (int)floor(txsiz * ScaleFactor);
		D_text_size(txsiz, txsiz);
	    }
	    D_use_color(white);
	    D_pos_abs((l + 3 + dots_per_line), (cur_dot_row));
	    if(color)
		D_text(buff);
	}
    }

    D_save_command(G_recreate_command());
    D_close_driver();
    
    exit(EXIT_SUCCESS);
}
