/* Function: vect_legend
 **
 ** Author: Paul W. Carlson     April 1992
 ** Modified by: Radim Blazek Jan 2000 area, label added 
 ** Modified by: Morten Hulden Mar 2004, support for legend in columns added 
 */

#include "vector.h"
#include "local_proto.h"

int PS_vlegend(void)
{
    int h, i, j, k, l, lc, st, lcount, nopos;
    double x, y, fontsize, dx, dy, xo, yo, xs, ys, margin, width;
    double llx, lly, urx, ury, sc;
    char pat[50];
    int **vec, *nvec;

    G_debug(2, "vect_legend(): count = %d", vector.count);

    vec = (int **)G_malloc(vector.count * sizeof(int *));
    for (i = 0; i < vector.count; i++)
	vec[i] = (int *)G_malloc(vector.count * sizeof(int));
    nvec = (int *)G_malloc(vector.count * sizeof(int));

    /* vec[][] is array of vectors on each row of legend, first to be drawn is first in array,
     *         label is stored in last in row (last plotted, first in the script)
     * nvec[] is number of vector on each row 
     *        index for both start at 0, lpos in script starts from 1
     * If position is not used by any vector it is used for next not used vector without
     * lpos or lpos > vector.count */

    nopos = -1;		/* last used without position in script or position > count */

    for (l = 0; l < vector.count; l++) {
	nvec[l] = 0;
	for (i = vector.count - 1; i >= 0; i--) {	/* last in script plot first */
	    if (vector.layer[i].lpos == l + 1) {
		vec[l][nvec[l]] = i;
		nvec[l]++;
	    }
	}
	if (nvec[l] == 0) {	/* no vector with this lpos */
	    for (i = nopos + 1; i < vector.count; i++) {
		/* first in script (most important) in script plot first */
		if (vector.layer[i].lpos == -1 ||
		    vector.layer[i].lpos > vector.count) {
		    vec[l][0] = i;
		    nvec[l]++;
		    nopos = i;
		    break;
		}
	    }
	}
    }

    /* find last used row */
    lcount = 0;		/* number of used rows in legend */
    for (i = vector.count - 1; i >= 0; i--) {
	if (nvec[i] > 0) {
	    lcount = i + 1;
	    break;
	}
    }

    /* set font */
    fontsize = (double)vector.fontsize;
    fprintf(PS.fp, "(%s) FN %.1f SF\n", vector.font, fontsize);

    /* get text location */
    dy = 1.5 * fontsize;

    if (vector.x > 0.0)
	x = 72.0 * vector.x;
    else
	x = PS.map_left;

    if (vector.y > 0.0)
	y = 72.0 * (PS.page_height - vector.y);
    else if (vector.x <= 0.0)
	y = PS.min_y;
    else
	y = PS.map_bot;

    margin = 0.4 * fontsize;

    if (x < PS.left_marg*72 + margin)
	x = PS.left_marg*72 + margin;

    if (lcount < vector.cols)
	vector.cols = lcount;

    if(vector.span < 0)
	dx = (PS.map_right - x) / vector.cols;
	/* dx = ((PS.page_width-PS.right_marg)*72 - x) / vector.cols; */
    else
	dx = vector.span*72;

    xs = x;			/*save x and y */
    ys = y;
    lc = (int)lcount / vector.cols;	/* lines per column */

    if (lcount % vector.cols)
	lc++;

    for (h = 0; h < vector.cols; h++) {

	y = ys;
	if (h)
	    x += dx;
	st = ((h + 1) * lc < lcount) ? ((h + 1) * lc) : lcount;

	/* make PostScript array "a" of name-mapset strings */
	fprintf(PS.fp, "/a [\n");

	for (i = h * lc; i < st; i++) {
	    G_debug(4, "  row = %d", i);
	    if (nvec[i] > 0) {	/* used position */
		j = vec[i][nvec[i] - 1];	/* vector with label */
		if (vector.layer[j].label == NULL)
		    fprintf(PS.fp, "( %s (%s))\n", vector.layer[j].name,
			    vector.layer[j].mapset);
		else
		    fprintf(PS.fp, "( %s)\n", vector.layer[j].label);
	    }
	    else {		/* not used -> empty string */
		fprintf(PS.fp, "( )\n");
	    }
	}
	fprintf(PS.fp, "] def\n");

	width = 72.0 * vector.width;
	if (width <= 0.0)
	    width = 2.4 * fontsize;

	/* HB 2006: always figure width & draw so the border will always work. */
	/* if vector legend is on map... */
	/*      if (y > PS.map_bot && y <= PS.map_top && x < PS.map_right) {   */
	fprintf(PS.fp, "/mg %.1f def\n", margin);

	/* get width of widest string in PostScript variable "w" */
	fprintf(PS.fp, "/w 0 def 0 1 a length 1 sub { /i XD\n");
	fprintf(PS.fp, "a i get SW pop /t XD t w gt {/w t def} if } for\n");
	fprintf(PS.fp, "/w w %.1f add mg add %.1f add def\n", x, width);

	/* make white background for text */
	fprintf(PS.fp, "1 1 1 C ");
	fprintf(PS.fp, "%.1f %.1f w %.1f B fill \n",
		x - margin, y - lc * dy - margin, y);
	/*      } */

	/* draw the border, if set */
	if (! color_none(&vector.border)) {
	    set_ps_color(&vector.border);

	    fprintf(PS.fp, "%.1f %.1f w %.1f B\n",
		    x - margin, y - lc * dy - margin, y);
	    fprintf(PS.fp, "D\n");
	}

	/* make the legend */
	for (j = h * lc; j < st; j++) {	/* each row */
	    G_debug(4, "  row = %d", j);
	    y -= dy;		/* set position of next row */
	    for (k = 0; k < nvec[j]; k++) {
		i = vec[j][k];

		/* make a grey box if needed */
		/* TODO */
		/*
		   if ((vector.layer[i].hwidth > 0. && vector.layer[i].hcolor == WHITE) ||
		   (vector.layer[i].hwidth < 1. && vector.layer[i].colors[0]  == WHITE))
		   {
		   fprintf(PS.fp, "0.5 setgray ");
		   fprintf(PS.fp, "%.1f %.1f %.1f %.1f B fill \n", 
		   x, y, x + 72.0, y + fontsize);
		   }
		 */

		if (vector.layer[i].type == VAREAS) {	/* added for areas */
		    /* plot rectangle */
		    yo = y - 0.1 * fontsize;

		    if (vector.layer[i].pat != NULL ||
			!(color_none(&vector.layer[i].fcolor))) {

			if (vector.layer[i].pat != NULL) {	/* use pattern */
			    sc = 0.5 * vector.layer[i].scale;	/* half scale */

			    /* load pattern */
			    eps_bbox(vector.layer[i].pat, &llx, &lly, &urx,
				     &ury);
			    sprintf(pat, "APATTEPS%d", i);
			    pat_save(PS.fp, vector.layer[i].pat, pat);

			    fprintf(PS.fp,
				    "<<  /PatternType 1\n    /PaintType 1\n    /TilingType 1\n");
			    fprintf(PS.fp, "    /BBox [%f %f %f %f]\n",
				    llx * sc, lly * sc, urx * sc, ury * sc);
			    fprintf(PS.fp, "    /XStep %f\n    /YStep %f\n",
				    (urx - llx) * sc, (ury - lly) * sc);
			    fprintf(PS.fp, "    /PaintProc\n      { begin\n");
			    fprintf(PS.fp, "        %f %f scale\n", sc, sc);
			    set_ps_color(&(vector.layer[i].fcolor));
			    fprintf(PS.fp, "        %.8f W\n",
				    vector.layer[i].pwidth);
			    fprintf(PS.fp, "        %s\n", pat);
			    fprintf(PS.fp, "        end\n");
			    fprintf(PS.fp, "      } bind\n>>\n");
			    sprintf(pat, "APATT%d", i);
			    fprintf(PS.fp,
				    " matrix\n makepattern /%s exch def\n",
				    pat);
			    fprintf(PS.fp,
				    "/Pattern setcolorspace\n %s setcolor\n",
				    pat);
			}
			else	/* solid fill color */
			    set_ps_color(&(vector.layer[i].fcolor));

			fprintf(PS.fp, "%.1f %.1f %.1f %.1f rectfill\n",
				x + width / 5, yo, 3 * width / 5,
				0.8 * fontsize);
		    }
		    if (!color_none(&vector.layer[i].color) &&
			vector.layer[i].width > 0) {
			fprintf(PS.fp, "%.8f W\n", vector.layer[i].width);
			set_ps_color(&(vector.layer[i].color));
			fprintf(PS.fp, "[] 0 setdash\n");
			fprintf(PS.fp, "%.1f %.1f %.1f %.1f rectstroke\n",
				x + width / 5, yo, 3 * width / 5,
				0.8 * fontsize);
		    }
		}
		else if (vector.layer[i].type == VLINES) {
		    yo = y + 0.35 * fontsize - vector.layer[i].offset;
		    /* do highlight, if any */
		    if (vector.layer[i].hwidth) {
			set_ps_color(&(vector.layer[i].hcolor));
			fprintf(PS.fp, "%.8f W\n",
				vector.layer[i].width +
				2 * vector.layer[i].hwidth);
			fprintf(PS.fp, "[] 0 setdash\n");
			fprintf(PS.fp, "%.1f %.1f %.1f %.1f L\n", x + width,
				yo, x, yo);
		    }

		    /* plot the primary color line */
		    set_ps_color(&(vector.layer[i].color));
		    fprintf(PS.fp, "%.8f W\n", vector.layer[i].width);
		    fprintf(PS.fp, "%s setdash\n", vector.layer[i].setdash);
		    fprintf(PS.fp, "%.1f %.1f %.1f %.1f L\n", x + width, yo,
			    x, yo);
		}
		else if (vector.layer[i].type == VPOINTS) {
		    /* TODO */
		    /* yo = y + 0.5 * fontsize - vector.layer[i].offset; */
		    yo = y + 0.5 * fontsize;
		    xo = x + width / 2;

		    symbol_draw(vector.layer[i].symbol_ps, xo, yo,
				vector.layer[i].size, vector.layer[i].rotate,
				vector.layer[i].width);
		}

	    }

	    /* plot the text */
	    set_rgb_color(BLACK);
	    fprintf(PS.fp, "a %d get %.1f %.1f MS\n",
			j - h * lc, x + width, y);
	}
    }	/* h */

    x = xs;
    y = ys - lc * dy;

    fprintf(PS.fp, "[] 0 setdash\n");

    if (PS.min_y > y)
	PS.min_y = y;

    return 0;
}
