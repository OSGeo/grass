/* draw.c:
 *    Compute position of legend, title, labels and ticks
 *    Extracted from original d.legend/main.c for background purpose
 *    Moving to separate file: Adam Laza, GSoC 2016
 *
 *    Copyright (C) 2014 by Hamish Bowman, and the GRASS Development Team*
 *    This program is free software under the GPL (>=v2)
 *    Read the COPYING file that comes with GRASS for details.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"

void draw(const char *map_name, int maptype, int color, int thin, int lines,
          int steps, int fp, int label_indent, int hide_catnum,
          int hide_catstr, int show_ticks, int hide_nodata, int do_smooth,
          struct Categories cats, struct Colors colors, double X0, double X1,
          double Y0, double Y1, int flip, int UserRange, double UserRangeMin,
          double UserRangeMax, double *catlist, int catlistCount,
          int use_catlist, int ticksCount, double fontsize,
          double tit_fontsize, const char *title, double *tick_values,
          double t_step, int colorb, int colorbg, struct Option *opt_use,
          struct Option *opt_at, struct Option *opt_fontsize,
          struct Option *opt_tstep, struct Option *opt_range, struct Flag *histo,
          struct Flag *hidestr, int log_sc, int draw, int digits, char *units)
{
    char buff[512];
    int black, white;
    int cats_num;
    int cur_dot_row;
    int do_cats;
    int dots_per_line;
    int i, j, k;
    double t, b, l, r;
    char *cstr;
    double x_box[5], y_box[5];
    struct Range range;
    struct FPRange fprange, render_range;
    CELL min_ind, max_ind;
    DCELL dmin, dmax, val;
    CELL min_colr, max_colr;
    DCELL min_dcolr, max_dcolr;
    int x0, x1, y0, y1, xyTemp;
    int SigDigits;
    unsigned int MaxLabelLen;
    char DispFormat[5];         /*  %.Xf\0  */
    double maxCat;
    int horiz;
    char *units_bottom;
    double t_start;
    double max_hist;
    double txsiz, titsiz;
    int tcell;
    float ScaleFactor = 1.0;
    double x_tit, y_tit, x1_tit;
    double x0bg, y0bg, x1bg, y1bg;
    double wleg, lleg;
    int true_l, true_r;
    int dx, dy;
    double coef;
    double ppl;
    double bb,bt,bl,br;
    char MaxLabel[512];
    double num;
    int MaxLabelW, LabelW;


    if (draw) {
        /* init colors */
        black = D_translate_color(DEFAULT_FG_COLOR);
        white = D_translate_color(DEFAULT_BG_COLOR);
    }

    /* Figure out where to put text */
    D_setup_unity(0);
    D_get_src(&t, &b, &l, &r);

    x0 = l + (int)((r - l) * X0 / 100.);
    x1 = l + (int)((r - l) * X1 / 100.);
    y0 = t + (int)((b - t) * (100. - Y0) / 100.);       /* make lower left the origin */
    y1 = t + (int)((b - t) * (100. - Y1) / 100.);

    if (y0 > y1) {              /* allow for variety in order of corner */
        flip = !flip;           /*   selection without broken output    */
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
        x1++;                   /* avoid 0 width boxes */
    if (y0 == y1)
        y1++;

    if (draw) {
        if ((x0 < l) || (x1 > r) || (y0 < t) || (y1 > b))       /* for mouse or at= 0- or 100+; needs to be after order check */
            G_warning(_("Legend box lies outside of frame. Text may not display properly."));
    }

    horiz = (x1 - x0 > y1 - y0);
    if (horiz && draw)
        G_message(_("Drawing horizontal legend as box width exceeds height"));

    if (!fp && horiz)           /* better than nothing */
        do_smooth = TRUE;

    MaxLabelLen = 0;            /* init variable */
    MaxLabelW = 0;

    /* How many categories to show */
    /* not fp */
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
                if (draw)
                    G_warning(_("Requested range exceeds lower limit of actual data"));
            }
            if (max_ind < UserRangeMax) {
                max_ind =
                    UserRangeMax >
                    max_colr ? max_colr : (int)floor(UserRangeMax);
                if (draw)
                    G_warning(_("Requested range exceeds upper limit of actual data"));
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

            if (!cstr[0]) {     /* no cat label found, skip str output */
                if (hide_nodata)
                    continue;
            }
            else {              /* ie has a label */
                if (!hide_catstr && (MaxLabelLen < strlen(cstr))) {
                    MaxLabelLen = strlen(cstr);
                    sprintf(MaxLabel, "%s", cstr);
            }
            }

            if (!hide_catnum)
                if (i > maxCat)
                    maxCat = (double)i;
            k++;                /* count of actual boxes drawn (hide_nodata option invaidates using j-1) */
        }
        lines = k;

        /* figure out how long the category + label will be */
        if (use_catlist) {
            MaxLabelLen = 0;
            maxCat = 0;         /* reset */
            for (i = 0, k = 0; i < catlistCount; i++) {
                if ((catlist[i] < min_ind) || (catlist[i] > max_ind)) {
                    G_fatal_error(_("use=%s out of range [%d,%d] (extend with range= ?)"),
                                  opt_use->answers[i], min_ind, max_ind);
                }

                cstr = Rast_get_d_cat(&catlist[i], &cats);
                if (!cstr[0]) { /* no cat label found, skip str output */
                    if (hide_nodata)
                        continue;
                }
                else {          /* ie has a label */
                    if (!hide_catstr && (MaxLabelLen < strlen(cstr)))
                        MaxLabelLen = strlen(cstr);
                }
                if (!hide_catnum)
                    if (catlist[i] > maxCat)
                        maxCat = catlist[i];
                k++;
            }
            if (0 == k)         /* nothing to draw */
                lines = 0;
        }

        /* following covers both the above if(do_cats == cats_num) and k++ loop */
        if (lines < 1) {
            lines = 1;          /* ward off the dpl floating point exception */
            G_fatal_error(_("Nothing to draw! (no categories with labels? out of range?)"));
        }

        /* Figure number of lines, number of pixles per line and text size */
        dots_per_line = ((y1 - y0) / lines);

        /* switch to a smooth legend for CELL maps with too many cats */
        /*  an alternate solution is to set   dots_per_line=1         */
        if ((dots_per_line == 0) && (do_smooth == FALSE)) {
            if (!use_catlist) {
                if (draw)
                    G_message(_("Forcing a smooth legend: too many categories for current window height"));
                do_smooth = TRUE;
            }
        }

        /* center really tiny legends */
        if (opt_at->answer == NULL) {   /* if defualt scaling */
            if (!do_smooth && (dots_per_line < 4))      /* if so small that there's no box */
                if ((b - (dots_per_line * lines)) / (b * 1.0) > 0.15)   /* if there's more than a 15% blank at the bottom */
                    y0 = ((b - t) - (dots_per_line * lines)) / 2;
        }

        /* D_text_size(dots_per_line*4/5., dots_per_line*4/5.);    redundant */
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
    }                           /* end of if(!fp) */

    else {                      /* is fp */
        if (maptype == MAP_TYPE_RASTER2D) {
            if (Rast_read_fp_range(map_name, "", &fprange) == -1)
                G_fatal_error(_("Range information for <%s> not available"),
                              map_name);
        }
        else {
            if (Rast3d_read_range(map_name, "", &fprange) == -1)
                G_fatal_error(_("Range information for <%s> not available"),
                              map_name);
        }

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

        /* In case of log. scale raster doesn't contain negative or zero values */
        if (log_sc)
            if ((dmin<=0) || (dmax<=0))
                G_fatal_error(_("Range [%.3f, %.3f] out of the logarithm domain."),
                              dmin, dmax);


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
        do_cats = 0;            /* if only to get rid of the compiler warning  */
        cats_num = 0;           /* if only to get rid of the compiler warning  */
        /* determine how many significant digits to display based on range */
        if (digits != -1) /* number of digits given by user */
            sprintf(DispFormat, "%%.%df", digits);
        else {/* automatic calculation */
        if (0 == (dmax - dmin)) /* trap divide by 0 for single value rasters */
            sprintf(DispFormat, "%%f");
        else {
            SigDigits = (int)ceil(log10(fabs(25 / (dmax - dmin))));
            if (SigDigits < 0)
                SigDigits = 0;
            if (SigDigits < 7)
                sprintf(DispFormat, "%%.%df", SigDigits);
            else
                sprintf(DispFormat, "%%.2g");   /* eg 4.2e-9  */
        }
        }
    }                           /* end of is fp */

    if (use_catlist) {
        cats_num = catlistCount;
        do_cats = catlistCount;
        lines = catlistCount;
        do_smooth = FALSE;
    }


    if (do_smooth) {
        if (horiz) {
            if (draw) {
                lleg = x1 - x0;
                dx = 0;
                dy = y1 - y0;
            }
            if (fp)
                flip = !flip;   /* horiz floats look better not flipped by default */
        }
        else {
            if (draw) {
                lleg = y1 - y0;
                dy = 0;
                dx = x1 - x0;
            }
        }

        /* Draw colors */
        /* Draw the legend bar */
        if (draw) {
            for (k = 0; k < lleg; k++) {
                if (log_sc) { /* logarithmic scale */
                    num = k / lleg;
                    val = dmin * pow(dmax/dmin, num);
                    D_d_color(val, &colors);
                    if (!flip) {
                        if (horiz)
                            D_box_abs(x0 + k, y0, x0 + k + 1, y0 + dy);
                        else
                            D_box_abs(x0, y0 + k, x0 + dx, y0 + k + 1);
                        }
                    else {
                        if (horiz)
                            D_box_abs(x1 - k, y0, x1 - k - 1, y0 + dy);
                        else
                            D_box_abs(x0, y1 - k, x0 + dx, y1 - k - 1);
                    }

                } /* linear scale */
                else{
                if (!fp) {
                    if (!flip)
                        tcell = min_ind + k * (double)(1 + max_ind - min_ind) / lleg;
                    else
                        tcell = (max_ind + 1) - k * (double)(1 + max_ind - min_ind) / lleg;
                    D_color((CELL) tcell, &colors);
                }
                else {
                    if (!flip)
                            val = dmin + k / lleg * (dmax - dmin);
                    else
                            val = dmax - k / lleg * (dmax - dmin);
                    D_d_color(val, &colors);
                }

                if (dx < dy)
                    D_box_abs(x0 + k, y0, x0 + k + (dx ? -dx : 1),
                              y0 - (dy ? -dy : 1));
                else
                    D_box_abs(x0, y0 + k, x0 - (dx ? -dx : 1),
                              y0 + k + (dy ? -dy : 1));
            }
        }
        }

        /* Format text */
        if (!fp) {              /* cut down labelnum so they don't repeat */
            if (do_cats < steps)
                steps = do_cats;
        }

        /* Draw text and ticks */
        if (!horiz)
            txsiz = (y1 - y0) / 20;
        else
            txsiz = (x1 - x0) / 20;

        wleg = x1 - x0;
        lleg = y1 - y0;

        /* scale text to fit in window if position not manually set */
        /* usually not needed, except when frame is really narrow   */
        if (opt_at->answer == NULL) {   /* ie default scaling */
            ScaleFactor = ((r - x1) / ((MaxLabelLen + 1) * txsiz * 0.81));      /* ?? txsiz*.81=actual text width. */
            if (ScaleFactor < 1.0) {
                txsiz = txsiz * ScaleFactor;
            }
        }

        if (opt_fontsize->answer != NULL)
            txsiz = fontsize;

        if (txsiz < 0)
            txsiz = 0;          /* keep it sane */

        if (tit_fontsize == 0)
            titsiz = txsiz;
        else
            titsiz = tit_fontsize;

        if (draw) {
            D_text_size(txsiz, txsiz);
            D_use_color(color);
        }

        /* Draw labels and ticks */
        /* LABELNUM OPTION */
        if (steps >= 2) {
            for (k = 0; k < steps; k++) {
                if (!fp) {
                    if (!flip)
                        tcell =
                            min_ind + k * (double)(max_ind -
                                                   min_ind) / (steps - 1);
                    else
                        tcell =
                            max_ind - k * (double)(max_ind -
                                                   min_ind) / (steps - 1);

                    if (!cstr[0])       /* no cats found, disable str output */
                        hide_catstr = 1;
                    else
                        hide_catstr = hidestr->answer;

                    buff[0] = 0;        /* blank string */

                    if (!hide_catnum) { /* num */
                        sprintf(buff, DispFormat, tcell);
                        if (!hide_catstr)       /* both */
                            strcat(buff, ")");
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                    }
                    }
                    if (!hide_catstr) {  /* str */
                        sprintf(buff + strlen(buff), " %s", cstr);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                }
                    }
                }
                else {          /* ie FP map */
                    if (hide_catnum)
                        buff[0] = 0;    /* no text */
                    else {
                        if (log_sc) {
                            num = log10(dmax) - k * ((log10(dmax) - log10(dmin)) / (steps - 1));
                            val = pow(10,num);
                        }
                        else{
                        if (!flip)
                            val = dmin + k * (dmax - dmin) / (steps - 1);
                        else
                            val = dmax - k * (dmax - dmin) / (steps - 1);
                        }
                        sprintf(buff, DispFormat, val);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                    }
                }
                }

                if (draw) {
                    if (!hide_catnum) {
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff, &bb, &bt, &bl, &br);
                        if (!horiz) {
                            if (log_sc) {
                                coef = (log10(val) - log10(dmin)) / (log10(dmax) - log10(dmin));
                                if (flip)
                            D_pos_abs(x1 + label_indent,
                                              y1 - coef * lleg + (bb - bt) / 2);
                                else
                                    D_pos_abs(x1 + label_indent,
                                              y0 + coef * lleg + (bb - bt) / 2);
                        }
                        else {
                                ppl = (lleg) / (steps * 1.0 - 1);
                                D_pos_abs(x1 + label_indent,
                                          y0 + ppl * k + (bb - bt) / 2);
                            }
                            if (show_ticks) {
                                D_use_color(black);
                                if (log_sc)
                                    if (flip)
                                        D_line_abs(x1, y1 - coef * lleg,
                                                   x1 + 6, y1 - coef * lleg);
                                    else
                                        D_line_abs(x1, y0 + coef * lleg,
                                                   x1 + 6, y0 + coef * lleg);
                                else
                                    D_line_abs(x1, y0 + ppl * k,
                                               x1 + 6, y0 + ppl * k);
                            }
                        }
                        else {
                            if (log_sc) {
                                coef = (log10(val) - log10(dmin)) / (log10(dmax) - log10(dmin));
                                if (flip)
                                    D_pos_abs(x1 - coef * wleg - ((br - bl) / 2),
                                      y1 + label_indent + txsiz);
                                else
                                    D_pos_abs(x0 + coef * wleg - ((br - bl) / 2),
                                              y1 + label_indent + txsiz);
                        }
                            else {
                                ppl = (wleg) / (steps * 1.0 - 1);
                                D_pos_abs(x0 + ppl * k - ((br - bl) / 2),
                                          y1 + label_indent + txsiz);
                            }
                            if (show_ticks) {
                                D_use_color(black);
                                if (log_sc)
                                    if (flip)
                                        D_line_abs(x1 - coef * wleg, y1,
                                                   x1 - coef * wleg, y1 + 6);
                                    else
                                        D_line_abs(x0 + coef * wleg, y1,
                                                   x0 + coef * wleg, y1 + 6);
                                else
                                    D_line_abs(x0 + ppl * k, y1,
                                               x0 + ppl * k, y1 + 6);
                            }
                        }
                        if (color)
                            D_use_color(color);
                            D_text(buff);
                    }
                }
            }                   /* for */
        }

        if (!fp) {
            dmin = min_ind;
            dmax = max_ind;
            sprintf(DispFormat, "%s", "%.0f");
        }

        /* LABEL_VALUE OPTION */
        if (ticksCount > 0) {
            for (i = 0; i < ticksCount; i++) {
                if ((tick_values[i] < dmin) || (tick_values[i] > dmax)) {
                    G_fatal_error(_("tick_value=%.3f out of range [%.3f, %.3f]"),
                                  tick_values[i], dmin, dmax);
                }
                sprintf(buff, DispFormat, tick_values[i]);
                if (strlen(units)>0)
                    strcat(buff, units);
                D_text_size(txsiz, txsiz);
                D_get_text_box(buff,&bb,&bt, &bl, &br);
                LabelW = br - bl;
                if (LabelW > MaxLabelW) {
                    MaxLabelW = LabelW;
                    sprintf(MaxLabel, "%s", buff);
                }

                if (log_sc) {
                    coef = (log10(tick_values[i]) - log10(dmin)) / (log10(dmax) - log10(dmin));
                }
                else
                coef = (tick_values[i] - dmin) / ((dmax - dmin) * 1.0);

                if (draw) {
                    if (!flip) {
                        if (!horiz) {
                            if (show_ticks) {
                                D_use_color(black);
                                D_line_abs(x1, y0 + coef * lleg,
                                           x1 + 6, y0 + coef * lleg);
                            }
                            D_pos_abs(x1 + label_indent,
                                      y0 + coef * lleg + txsiz / 2);
                        }
                        else {
                            if (show_ticks) {
                                D_use_color(black);
                                D_line_abs(x0 + coef * wleg, y1,
                                           x0 + coef * wleg, y1 + 6);
                            }
                            D_pos_abs(x0 + coef * wleg -
                                      (strlen(buff) * txsiz * .81 / 2),
                                      y1 + label_indent + txsiz);
                        }
                    }
                    else {
                        if (!horiz) {
                            if (show_ticks) {
                                D_use_color(black);
                                D_line_abs(x1, y1 - coef * lleg,
                                           x1 + 6, y1 - coef * lleg);
                            }
                            D_pos_abs(x1 + label_indent,
                                      y1 - coef * lleg + txsiz / 2);
                        }
                        else {
                            if (show_ticks) {
                                D_use_color(black);
                                D_line_abs(x1 - coef * wleg, y1,
                                           x1 - coef * wleg, y1 + 6);
                            }
                            D_pos_abs(x1 - coef * wleg -
                                      (strlen(buff) * txsiz * .81 / 2),
                                      y1 + label_indent + txsiz);
                        }
                    }
                    D_use_color(color);
                    D_text(buff);
                }
            }
        }

        /* LABEL_STEP OPTION */
        if (opt_tstep->answer) {
            if (log_sc) { /* logarithmic */
                t_start=0;
                while (log10(dmin) + t_start < log10(dmax)){
                    num = ceil(log10(dmin)) + t_start;
                    val = pow(10,num);
                    sprintf(buff, DispFormat, val);
                    if (strlen(units)>0)
                        strcat(buff, units);
                    D_text_size(txsiz, txsiz);
                    D_get_text_box(buff,&bb,&bt, &bl, &br);
                    LabelW = br - bl;
                    if (LabelW > MaxLabelW) {
                        MaxLabelW = LabelW;
                        sprintf(MaxLabel, "%s", buff);
                    }
                    coef = (log10(val) - log10(dmin)) / (log10(dmax) - log10(dmin));
                    if (draw){
                        if (!flip){
                            if (!horiz){
                                if (show_ticks) {
                                    D_use_color(black);
                                    D_line_abs(x1, y0 + coef * lleg,
                                               x1 + 6, y0 + coef * lleg);
                                }
                                D_pos_abs(x1 + label_indent,
                                          y0 + coef * lleg + txsiz / 2);
                            }
                            else{
                                if (show_ticks) {
                                    D_use_color(black);
                                    D_line_abs(x0 + coef * wleg, y1,
                                               x0 + coef * wleg, y1 + 6);
                                }
                                D_pos_abs(x0 + coef * wleg -
                                          (strlen(buff) * txsiz * .81 / 2),
                                          y1 + label_indent + txsiz);
                            }
                        }
                        else{
                            if (!horiz){
                                if (show_ticks) {
                                    D_use_color(black);
                                    D_line_abs(x1, y1 - coef * lleg,
                                               x1 + 6, y1 - coef * lleg);
                                }
                                D_pos_abs(x1 + label_indent,
                                          y1 - coef * lleg + txsiz / 2);
                            }
                            else{
                                if (show_ticks){
                                    D_use_color(black);
                                    D_line_abs(x1 - coef * wleg, y1,
                                               x1 - coef * wleg, y1 + 6);
                                }
                                D_pos_abs(x1 - coef * wleg -
                                          (strlen(buff) * txsiz * .81 / 2),
                                          y1 + label_indent + txsiz);
                            }
                        }
                        D_use_color(color);
                        D_text(buff);

                    }
                t_start += t_step;
                }
            }
            else { /* linear */
            t_start = ceil(dmin / t_step) * t_step;
            if (t_start == -0)
                t_start = 0;

            if (!flip) {
                if (!horiz)
                    while (t_start <= dmax) {
                        sprintf(buff, DispFormat, t_start);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                        }
                        if (draw) {
                            coef = (t_start - dmin) / ((dmax - dmin) * 1.0);
                            if (show_ticks) {
                                D_use_color(black);
                                D_line_abs(x1, y0 + coef * lleg,
                                           x1 + 6, y0 + coef * lleg);
                            }
                            D_pos_abs(x1 + label_indent,
                                      y0 + coef * lleg + txsiz / 2);
                            D_use_color(color);
                            D_text(buff);
                        }
                        t_start += t_step;
                    }
                else
                    while (t_start <= dmax) {
                        sprintf(buff, DispFormat, t_start);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                        }

                        if (draw) {
                            coef = (t_start - dmin) / ((dmax - dmin) * 1.0);
                            if (show_ticks) {
                                D_use_color(black);
                                D_line_abs(x0 + coef * wleg, y1,
                                           x0 + coef * wleg, y1 + 6);
                            }
                            D_pos_abs(x0 + coef * wleg -
                                      (strlen(buff) * txsiz * .81 / 2),
                                      y1 + label_indent + txsiz);
                            D_use_color(color);
                            D_text(buff);
                        }
                        t_start += t_step;
                    }
            }
            else {
                if (!horiz)
                    while (t_start <= dmax) {
                        sprintf(buff, DispFormat, t_start);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                        }

                        if (draw) {
                            coef = (t_start - dmin) / ((dmax - dmin) * 1.0);
                            if (show_ticks) {
                                D_use_color(black);
                                D_line_abs(x1, y1 - coef * lleg,
                                           x1 + 6, y1 - coef * lleg);
                            }
                            D_pos_abs(x1 + label_indent,
                                      y1 - coef * lleg + txsiz / 2);
                            D_use_color(color);
                            D_text(buff);
                        }
                        t_start += t_step;
                    }
                else
                    while (t_start <= dmax) {
                        sprintf(buff, DispFormat, t_start);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                        }

                        if (draw) {
                            coef = (t_start - dmin) / ((dmax - dmin) * 1.0);
                            if (show_ticks){
                                D_use_color(black);
                                D_line_abs(x1 - coef * wleg, y1,
                                           x1 - coef * wleg, y1 + 6);
                            }
                            D_pos_abs(x1 - coef * wleg -
                                      (strlen(buff) * txsiz * .81 / 2),
                                      y1 + label_indent + txsiz);
                            D_use_color(color);
                            D_text(buff);
                        }
                        t_start += t_step;
                    }
            }
        }
        }

        if (draw) {
            /* Draw boxes outside of legend bar */
            /* White box */
            D_use_color(white);
            D_begin();
            D_move_abs(x0 + 1, y0 + 1);
            D_cont_rel(0, lleg - 2);
            D_cont_rel(wleg - 2, 0);
            D_cont_rel(0, -lleg + 2);
            D_close();
            D_end();
            D_stroke();

            /* Black box */
            D_use_color(black);
            D_begin();
            D_move_abs(x0, y0);
            D_cont_rel(0, lleg);
            D_cont_rel(wleg, 0);
            D_cont_rel(0, -lleg);
            D_close();
            D_end();
            D_stroke();
        }

        /* Display sidebar histogram, if requested.
           /  In case of horizontal legend, maximum of histogram - max_hist
           will affect the title position */
        max_hist = 0;
        if (histo->answer) {
            render_range.min = (DCELL) (fp ? dmin : min_ind);
            render_range.max = (DCELL) (fp ? dmax : max_ind);
            /* reuse flag to indicate if user-specified or default ranging */
            render_range.first_time = opt_range->answer ? TRUE : FALSE;

            if (draw)
                max_hist =
                    histogram(map_name, x0, y0, wleg, lleg, color, flip,
                              horiz, maptype, fp, render_range, 1);
            else
                max_hist =
                    histogram(map_name, x0, y0, wleg, lleg, color, flip,
                              horiz, maptype, fp, render_range, 0);
        }


        /* display title or units */
        if (strlen(title) > 0) {
            D_text_size(titsiz, titsiz);
            D_get_text_box(title, &bb, &bt, &bl, &br);
            /* title */
            if (horiz) {
                x_tit = (x0 + x1) / 2. - (br - bl) / 2;
                y_tit = y0 - (titsiz) - max_hist;
            }
            else {
                x_tit = x0;
                y_tit = y0 - txsiz;
            }

            x1_tit = x_tit + (br - bl);

            if (draw) {
                D_use_color(color);
                /* use tit_fontsize */
                D_text_size(titsiz, titsiz);

                D_pos_abs(x_tit, y_tit);
                D_text(title);
                /* restart fontsize */
                D_text_size(txsiz, txsiz);
            }
        }
        else {
            /* units */
            /* print units label, if present */
            if (maptype == MAP_TYPE_RASTER2D)
                units_bottom = Rast_read_units(map_name, "");
            else
                units = "";
            /* FIXME: does the raster3d really need to be opened to read the units?
               units_bottom = Rast3d_get_unit(map_fid); */

            if (!units_bottom)
                units_bottom = "";

            if (strlen(units_bottom)) {
                D_text_size(titsiz, titsiz);
                D_get_text_box(title, &bb, &bt, &bl, &br);
                if (horiz) {
                    x_tit =
                        (x0 + x1) / 2. - (br - bl) / 2;
                    y_tit = y1 + (txsiz * 2.75);
                }
                else {
                    x_tit = x0;
                }
                x1_tit = x_tit + (br - bl);

                if (draw) {
                    D_use_color(color);
                    D_pos_abs(x_tit, y_tit);
                    D_text(units_bottom);
                }
            }
        }                       /* end of display units) */

        if (!draw) {
            /* Draw background */
            D_text_size(txsiz, txsiz);
            D_get_text_box(MaxLabel, &bb, &bt, &bl, &br);
            if (!horiz) {
                x0bg = x0 - max_hist - txsiz;
                x1bg = x0 + wleg + label_indent + (br - bl) + txsiz;
                if (x1bg < x1_tit)
                    x1bg = x1_tit + txsiz;
                y1bg = y0 + lleg + txsiz;
                if (strlen(title) > 0)
                    y0bg = y0 - titsiz -2 * txsiz;
                else
                    y0bg = y0 - txsiz;
            }
            else {
                x0bg = x0 - (br - bl) / 2 - txsiz;
                x1bg = x0 + wleg + (br - bl) / 2 + txsiz;
                if (x1bg < x1_tit) {
                    x0bg = x_tit - txsiz;
                    x1bg = x1_tit + txsiz;
                }
                y1bg = y0 + lleg + label_indent + 1.5 * txsiz;
                if (strlen(title) > 0)
                    y0bg = y0 - (2.5 * titsiz) - max_hist;
                else
                    y0bg = y0 - titsiz - max_hist;
            }

            if (colorbg != 0) {
                D_use_color(colorbg);
                D_box_abs(x0bg, y0bg, x1bg, y1bg);
            }

            D_use_color(colorb);
            D_begin();
            D_move_abs(x0bg, y0bg);
            D_cont_abs(x0bg, y1bg);
            D_cont_abs(x1bg, y1bg);
            D_cont_abs(x1bg, y0bg);
            D_close();
            D_end();
            D_stroke();
        }

    }                           /* end of if(do_smooth) */

    else {                      /* non FP, no smoothing */
        ScaleFactor = 1.0;

        if (histo->answer)
            G_warning(_("Histogram plotting not implemented for categorical legends. "
                       "Use the '-s' flag"));

        /* set legend box bounds */
        true_l = l;
        true_r = r;             /* preserve window width */
        l = x0;
        t = y0;
        r = x1;
        b = y1;

        /* figure out box height  */
        if (do_cats == cats_num)
            dots_per_line = (b - t) / (lines + 1);      /* +1 line for the two 1/2s at top and bottom */
        else
            dots_per_line = (b - t) / (lines + 2);      /* + another line for 'x of y categories' text */

        /* adjust text size */
        /*  txsiz = (int)((y1-y0)/(1.5*(lines+5))); */
        txsiz = (y1 - y0) / (2.0 * lines);

        if (tit_fontsize == 0)
            titsiz = txsiz;
        else
            titsiz = tit_fontsize;

        /* scale text to fit in window if position not manually set */
        if (opt_at->answer == NULL) {   /* ie defualt scaling */
            ScaleFactor = ((true_r - true_l) / ((MaxLabelLen + 3) * txsiz * 0.81));     /* ?? txsiz*.81=actual text width. */
            if (ScaleFactor < 1.0) {
                txsiz = txsiz * ScaleFactor;
                dots_per_line = (int)floor(dots_per_line * ScaleFactor);
            }
        }

        if (dots_per_line < txsiz)
            txsiz = dots_per_line;

        if (opt_fontsize->answer != NULL)
            txsiz = fontsize;

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

        /* if(ScaleFactor < 1.0)   */
        /*    cur_dot_row = ((b-t) - (dots_per_line*lines))/2; *//* this will center the legend */
        /* else    */
        cur_dot_row = t + dots_per_line / 2;

        /*  j = (do_cats == cats_num ? 1 : 2 ); */

        if (draw) {
            D_pos_abs(x0, y0);
            D_text_size(txsiz, txsiz);
        }

        for (i = 0, k = 0; i < catlistCount; i++)
            /* for(i=min_ind, j=1, k=0; j<=do_cats && i<=max_ind; j++, i+=thin) */
        {
            if (!flip)
                cstr = Rast_get_d_cat(&catlist[i], &cats);
            else
                cstr = Rast_get_d_cat(&catlist[catlistCount - i - 1], &cats);


            if (!cstr[0]) {     /* no cat label found, skip str output */
                hide_catstr = 1;
                if (hide_nodata)
                    continue;
            }
            else
                hide_catstr = hidestr->answer;

            k++;                /* count of actual boxes drawn (hide_nodata option invaidates using j-1) */

            cur_dot_row += dots_per_line;

            if (draw) {
                /* Black box */
                D_use_color(black);
                D_begin();
                D_move_abs(l + 2, (cur_dot_row - 1));
                D_cont_rel(0, (3 - dots_per_line));
                D_cont_rel((dots_per_line - 3), 0);
                D_cont_rel(0, (dots_per_line - 3));
                D_close();
                D_end();
                D_stroke();

                /* White box */
                D_use_color(white);
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
            }


            if (!fp) {
                /* nothing, box only */
                buff[0] = 0;
                if (!hide_catnum) {     /* num */
                    sprintf(buff, DispFormat, (int)catlist[i]);
                    if (strlen(units)>0)
                        strcat(buff, units);
                    D_text_size(txsiz, txsiz);
                    D_get_text_box(buff,&bb,&bt, &bl, &br);
                    LabelW = br - bl;
                    if (LabelW > MaxLabelW) {
                        MaxLabelW = LabelW;
                        sprintf(MaxLabel, "%s", buff);
                    }
                    if (!flip) {
                        sprintf(buff, DispFormat, (int)catlist[i]);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                        }
                    }
                    else {
                        sprintf(buff, DispFormat,
                                (int)catlist[catlistCount - i - 1]);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                        }
                    }
                    if (!hide_catstr)   /* both */
                        strcat(buff, ")");
                }
                if (!hide_catstr) {       /* str */
                    sprintf(buff + strlen(buff), " %s", cstr);
                    if (strlen(units)>0)
                        strcat(buff, units);
                    D_text_size(txsiz, txsiz);
                    D_get_text_box(buff,&bb,&bt, &bl, &br);
                    LabelW = br - bl;
                    if (LabelW > MaxLabelW) {
                        MaxLabelW = LabelW;
                        sprintf(MaxLabel, " %s", buff);
            }
                }
            }
            else {              /* is fp */
                if (!flip) {
                    if (use_catlist) {
                        /* pass through format exactly as given by the user in
                           the use= command line parameter (helps with log scale) */
                        sprintf(buff, "%s", opt_use->answers[i]);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, " %s", buff);
                        }
                    }
                    else {
                        /* automatically generated/tuned decimal precision format */
                        sprintf(buff, DispFormat, catlist[i]);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, "%s", buff);
                }
                    }
                }
                else {
                    if (use_catlist){
                        sprintf(buff, "%s", opt_use->answers[catlistCount - i - 1]);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, " %s", buff);
                }
            }
                    else {
                        sprintf(buff, DispFormat, catlist[catlistCount - i - 1]);
                        if (strlen(units)>0)
                            strcat(buff, units);
                        D_text_size(txsiz, txsiz);
                        D_get_text_box(buff,&bb,&bt, &bl, &br);
                        LabelW = br - bl;
                        if (LabelW > MaxLabelW) {
                            MaxLabelW = LabelW;
                            sprintf(MaxLabel, " %s", buff);
            }
                    }
                }
            }

            if (draw) {
                D_pos_abs((l + 3 + dots_per_line), (cur_dot_row) - 3);
                if (color)
                    D_text(buff);
            }
        }

        if (0 == k)
            G_fatal_error(_("Nothing to draw! (no categories with labels?)"));  /* "(..., out of range?)" */

        /* display title */
        if (strlen(title) > 0) {
            x_tit = x0;
            y_tit = y0 - txsiz;

            D_text_size(titsiz, titsiz);
            D_get_text_box(title, &bb, &bt, &bl, &br);
            x1_tit = x_tit + (br - bl);

            if (draw) {
                D_use_color(color);
                /* use tit_fontsize */
                D_text_size(titsiz, titsiz);
                D_pos_abs(x_tit, y_tit);
                D_text(title);
                /* restart fontsize */
                D_text_size(txsiz, txsiz);
            }
        }

        /* Display info line about numbers of categories */
        if (do_cats != cats_num) {
            cur_dot_row += dots_per_line;
            /* sprintf(buff, "%d of %d categories\n", (j-1), cats_num); */

            sprintf(buff, "%d of %d categories\n", k, cats_num);
            if (strlen(buff) > MaxLabelLen) {
                MaxLabelLen = strlen(buff);
                sprintf(MaxLabel, "%d of %d categories\n", k, cats_num);
            }

            if (draw) {
                if (opt_fontsize->answer != NULL)
                    txsiz = fontsize;
                D_text_size(txsiz, txsiz);
                D_use_color(black);
                D_pos_abs((l + 3 + dots_per_line), (cur_dot_row));
                if (color)
                    D_text(buff);
            }
        }

        if (!draw) {
            /* Draw background */
            D_text_size(txsiz, txsiz);
            D_get_text_box(MaxLabel, &bb, &bt, &bl, &br);
            x0bg = x0 - txsiz;
            x1bg =
                x0 + dots_per_line + 3 + (br - bl) + txsiz;
            if (x1bg < x1_tit)
                x1bg = x1_tit + txsiz;
            y1bg = cur_dot_row + txsiz;
            if (strlen(title) > 0)
                y0bg = y0 - 2 * txsiz - titsiz;
            else
                y0bg = y0 - txsiz;


            if (colorbg != 0) {
                D_use_color(colorbg);
                D_box_abs(x0bg, y0bg, x1bg, y1bg);
            }

            D_use_color(colorb);
            D_begin();
            D_move_abs(x0bg, y0bg);
            D_cont_abs(x0bg, y1bg);
            D_cont_abs(x1bg, y1bg);
            D_cont_abs(x1bg, y0bg);
            D_close();
            D_end();
            D_stroke();
        }

    }
    D_save_command(G_recreate_command());
}
