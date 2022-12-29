/*!
   \file lib/ogsf/gsd_legend.c

   \brief OGSF library - legend creation

   GRASS OpenGL gsurf OGSF Library 

   Converted code from legend.c in SG3d
   routines to set viewport, close viewport, and make legend

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/config.h>

#if defined(OPENGL_X11) || defined(OPENGL_WINDOWS)
#include <GL/gl.h>
#include <GL/glu.h>
#elif defined(OPENGL_AQUA)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/ogsf.h>

#include "rgbpack.h"

static float *Listcats;
static int Listnum = 0;

/**** TODO
static int bigger(float *f1, float *f2)
{
    return (*f1 < *f2 ? -1 : (*f1 > *f2));
}
*****/

#define MAX_LEGEND 256

/*!
   \brief ADD

   \param wl
   \param wb
   \param wr
   \param wt
 */
void gsd_bgn_legend_viewport(GLint wl, GLint wb, GLint wr, GLint wt)
{
    /* sets the viewport for the legend and the model matrix */

    gsd_colormode(CM_COLOR);
    glPushAttrib(GL_VIEWPORT);

    glMatrixMode(GL_PROJECTION);

    gsd_pushmatrix();
    GS_set_draw(GSD_FRONT);
    GS_ready_draw();

    gsd_linewidth(1);

    gsd_popmatrix();

    glViewport(wl, wb, (wr - wl), (wt - wb));
    glLoadIdentity();
    gluOrtho2D(-0.5, (wr - wl) + 0.5, -0.5, (wt - wb) + 0.5);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    return;
}

/*!
   \brief ADD
 */
void gsd_end_legend_viewport(void)
{
    /* closes the legend viewport and resets matrix and buffers */

    gsd_popmatrix();
    glMatrixMode(GL_PROJECTION);
    gsd_popmatrix();

    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);
    gsd_popmatrix();

    GS_done_draw();
    GS_set_draw(GSD_BACK);

    return;
}

/*!
   \brief ADD

   \param lownum
   \param highnum
   \param numvals
   \param vals

   \return 0 on failure
   \return range value
 */
int gsd_get_nice_range(float lownum, float highnum, int numvals, float *vals)
{
    /* get a nice range for displaying legend */

    int num = 0;
    float curnum, step, start;

    if (!numvals)
	return (0);

    step = (highnum - lownum) / (float)numvals;
    gsd_make_nice_number(&step);

    /* get a starting point */
    start = step * (int)(1 + lownum / step);
    if (start - lownum < .65 * step)
	start += step;

    for (curnum = start; curnum < (highnum - .65 * step); curnum += step) {
	vals[num++] = curnum;
    }

    return (num);

}

/*!
   \brief ADD

   \param num

   \return 0 on error
   \return 1 on success
 */
int gsd_make_nice_number(float *num)
{
    float newnum, nextnum;

    if (*num < 0)
	return (0);

    if (*num < 1) {
	newnum = 1.;
	while (.5 * newnum > *num) {
	    nextnum = newnum / 10.;
	    newnum /= 2.;
	    if (.5 * newnum > *num)
		newnum /= 2.;
	    if (.5 * newnum > *num)
		newnum = nextnum;
	}
    }
    else {
	newnum = 1.;
	while (2 * newnum <= *num) {
	    nextnum = newnum * 10.;
	    newnum *= 2.5;
	    if (2 * newnum <= *num)
		newnum *= 2.;
	    if (2 * newnum <= *num)
		newnum = nextnum;
	}
	if (newnum == 2.5)
	    newnum = 3;
	/* 2.5 isn't nice, but .25, 25, 250 ... are */
    }
    *num = newnum;
    return (1);
}

/*!
   \brief Put legend

   \param name
   \param fontbase font-base
   \param size
   \param flags
   \param rangef
   \param pt

   \return
 */
GLuint gsd_put_legend(const char *name, GLuint fontbase, int size, int *flags,
		      float *rangef, int *pt)
{
    GLint sl, sr, sb, st;
    GLuint legend_list;
    int cat_labs = 0, cat_vals = 0, do_invert = 0, discrete = 0;
    int is_fp, fprec, iprec;
    struct Categories cats;
    struct Range range;
    struct FPRange fp_range;
    const char *mapset;
    struct Colors colors;
    CELL min, max;
    DCELL fmin, fmax;
    float labvals[12];

    legend_list = gsd_makelist();
    gsd_bgnlist(legend_list, 1);

    /* set coords from pt */
    sl = pt[0];
    sr = pt[1];
    sb = pt[2];
    st = pt[3];

    /* set legend flags */
    if (flags[0])
	cat_vals = 1;
    if (flags[1])
	cat_labs = 1;
    if (flags[3])
	discrete = 1;
    if (flags[2])
	do_invert = 1;

    mapset = G_find_raster2(name, "");
    if (mapset == NULL) {
	G_warning(_("Raster map <%s> not found"), name);
	return (-1);
    }

    is_fp = Rast_map_is_fp(name, mapset);

    if (Rast_read_colors(name, mapset, &colors) == -1) {
	G_warning(_("Unable to read color file of raster map <%s>"), name);
	return (-1);
    }

    if (cat_labs)
	if (Rast_read_cats(name, mapset, &cats) == -1) {
	    G_warning(_("Unable to read category file of raster map <%s>"),
		      name);
	    cat_labs = 0;
	}


    if (flags[4] && rangef[0] != -9999. && rangef[1] != -9999.) {
	fmin = rangef[0];
	fmax = rangef[1];
	if (!is_fp) {
	    min = (int)fmin;
	    max = (int)fmax;
	}
    }
    else {
	if (is_fp) {
	    if (Rast_read_fp_range(name, mapset, &fp_range) != 1) {
		G_warning(_("Unable to read fp range of raster map <%s>"),
			  name);
		return (-1);
	    }
	    Rast_get_fp_range_min_max(&fp_range, &fmin, &fmax);
	    if (flags[4] && rangef[0] != -9999.)
		fmin = rangef[0];
	    if (flags[4] && rangef[1] != -9999.)
		fmax = rangef[1];
	}
	else {
	    if (Rast_read_range(name, mapset, &range) == -1) {
		G_warning(_("Unable to read range of raster map <%s>"), name);
		return (-1);
	    }
	    Rast_get_range_min_max(&range, &min, &max);
	    if (flags[4] && rangef[0] != -9999.)
		min = rangef[0];
	    if (flags[4] && rangef[1] != -9999.)
		max = rangef[1];
	    fmin = min;
	    fmax = max;
	}
    }

    if (fmin == fmax)
	G_warning(_("Range request error for legend"));

    /* set a reasonable precision */
    if (is_fp) {
	float df;

	df = fmax - fmin;
	if (df < .1)
	    fprec = 6;
	else if (df < 1)
	    fprec = 4;
	else if (df < 10)
	    fprec = 3;
	else if (df < 100)
	    fprec = 2;
	else
	    fprec = 1;

    }
    else {
	int tmp, p1, p2;

	iprec = p1 = p2 = 1;
	if (max > 0)
	    for (tmp = 1; tmp < max; tmp *= 10, p1++) ;
	if (min < 0)
	    for (tmp = -1; tmp > min; tmp *= 10, p2++) ;

	iprec = (p1 > p2 ? p1 : p2);
    }

/*********
 * TODO incorp lists

     if(list && (legend_type & LT_LIST)){
	Listcats = list;
	Listnum = nlist;
	qsort(Listcats, Listnum, sizeof(float), bigger);
	discrete = 1;   
    }
    else
	Listnum = 0;

*********/


    /* how many labels? */
    /*
       numlabs can't be = max - min + 1 any more because of floating point
       maybe shouldn't allow discrete legend for floating point maps (unless list)
       or else check number of different values in floating point map
       and use each if "reasonable"
       gs_get_values_in_range(gs, att, low, high, values, &nvals)
       the nvals sent has a max number to return, nvals returned is the actual
       number set in values, return val is 1 on success, -1 if > max vals found

       might need to think about doing histograms first & use same routines here
       could also have a LT_MOST that would limit # to some N most frequent
     */

    /*!
       ???
     */
    {
	int i, k, lleg, horiz;
	int red, green, blue;
	CELL tcell;
	DCELL tdcell, pdcell;
	float vert1[2], vert2[2], vert3[2], vert4[2];
	float *dv1, *dv2;	/* changing vertex coord */
	float *sv1, *sv2;	/* stable vertex coord */
	float stab1, stab2;
	unsigned long colr;
	float *dividers;
	int labw, maxlabw, numlabs;
	float labpos, labpt[3];
	const char *cstr;
	char buff[80];
	GLint wt, wb, wl, wr;	/* Whole legend area, not just box */
	int xoff, yoff;
	int incr;		/* for do_invert */

	horiz = (sr - sl > st - sb);
	dividers = NULL;

	if (discrete) {
	    numlabs = Listnum ? Listnum : max - min + 1;
	    /* watch out for trying to display mega cats */
	    if (is_fp && !Listnum) {
		discrete = 0;	/* maybe later do stats & allow if few #s */
		G_warning(_("Unable to show discrete FP range (use list)"));
		return (-1);
	    }
	    if (numlabs < MAX_LEGEND)
		dividers = (float *)G_malloc(numlabs * sizeof(float));
	}
	else {
	    numlabs = gsd_get_nice_range(fmin, fmax, 4, labvals + 1);
	    labvals[0] = fmin;
	    labvals[numlabs + 1] = fmax;
	    numlabs += 2;
	}

	/* find longest string, reset viewport & saveunder */
	maxlabw = 0;

	if (cat_labs || cat_vals) {
	    for (k = 0; k < numlabs; k++) {
		if (is_fp) {
		    tdcell = discrete ? Listcats[k] : labvals[k];
		    if (cat_labs) {
			cstr = Rast_get_d_cat(&tdcell, &cats);
		    }
		    if (cat_labs && !cat_vals) {
			sprintf(buff, "%s", cstr);
		    }
		    else {
			if (cat_labs && cat_vals) {
			    if (cstr)
				sprintf(buff, "%.*lf) %s",
					fprec, tdcell, cstr);
			    else
				sprintf(buff, "%.*lf", fprec, tdcell);
			}
			else if (cat_vals)
			    sprintf(buff, "%.*lf", fprec, tdcell);
		    }
		}
		else {
		    tcell = discrete ? Listnum ?
			Listcats[k] : min + k : labvals[k];
		    if (cat_labs && !cat_vals)
			sprintf(buff, "%s", Rast_get_c_cat(&tcell, &cats));
		    else {
			if (cat_labs && cat_vals) {
			    cstr = Rast_get_c_cat(&tcell, &cats);
			    if (cstr[0])
				sprintf(buff, "%*d) %s", iprec, tcell, cstr);
			    else
				sprintf(buff, "%d", tcell);
			}
			else if (cat_vals)
			    sprintf(buff, "%d", tcell);
		    }
		}
		labw = gsd_get_txtwidth(buff, size);
		if (labw > maxlabw) {
		    maxlabw = labw;
		}
	    }
	}

	if (horiz) {
	    xoff = maxlabw / 2 + get_txtxoffset();
	    wl = sl - xoff;
	    wr = sr + xoff;
	    yoff = 0;
	    wb = sb;
	    /*
	       wt = st + gsd_get_txtheight() + get_txtdescender() +3;
	     */
	    wt = st + gsd_get_txtheight(size) * 2 + 3;
	}
	else {
	    xoff = 0;
	    wl = sl;
	    wr = sr + maxlabw + get_txtxoffset() + 3;
	    /*
	       yoff = gsd_get_txtheight()/2 + get_txtdescender();
	     */
	    yoff = gsd_get_txtheight(size);
	    wb = sb - yoff;
	    wt = st + yoff;
	}

	/* initialize viewport */
	gsd_bgn_legend_viewport(wl, wb, wr, wt);


	vert1[X] = vert2[X] = xoff;
	vert1[Y] = vert2[Y] = yoff;
	if (horiz) {
	    lleg = sr - sl;
	    dv1 = vert1 + X;
	    dv2 = vert2 + X;
	    sv1 = vert1 + Y;
	    sv2 = vert2 + Y;
	    stab2 = vert2[Y] = st - sb + yoff;
	    stab1 = vert1[Y] = yoff;
	    if (do_invert)
		vert1[X] = vert2[X] = sr - sl + xoff;
	}
	else {
	    lleg = st - sb;
	    dv1 = vert1 + Y;
	    dv2 = vert2 + Y;
	    sv1 = vert1 + X;
	    sv2 = vert2 + X;
	    stab2 = vert2[X] = sr - sl + xoff;
	    stab1 = vert1[X] = xoff;
	    if (do_invert)
		vert1[Y] = vert2[Y] = st - sb + yoff;
	}

	if (discrete) {
	    if (numlabs > lleg / 5)
		G_warning(_("Too many categories to show as discrete!"));
	    else if (numlabs > 1.2 * lleg / gsd_get_txtheight(size))
		G_warning(_("Try using smaller font!"));
	}

	incr = do_invert ? -1 : 1;
	for (k = 0, i = 0; k < lleg; k++) {
	    if (discrete && Listnum)
		tdcell = Listcats[(int)((float)k * numlabs / lleg)];
	    else {
		tcell = min + k * (max - min + 1) / lleg;
		tdcell = fmin + k * (fmax - fmin) / lleg;
		if (!is_fp)
		    tdcell = tcell;
	    }
	    if (k == 0 || tdcell != pdcell) {
		if (is_fp)
		    Rast_get_d_color(&tdcell,
					 &red, &green, &blue, &colors);
		else
		    Rast_get_c_color((CELL *)&tdcell, &red, &green, &blue, &colors);

		RGB_TO_INT(red, green, blue, colr);
		if (discrete) {	/* draw black-white-black separator */
		    if (k > 0) {
			*dv1 -= 2. * incr;
			*dv2 -= 2. * incr;
			gsd_color_func(0x0);
			gsd_bgnline();
			glVertex2fv(vert1);
			glVertex2fv(vert2);
			gsd_endline();

			*dv1 += 1. * incr;
			*dv2 += 1. * incr;
			if (dividers)
			    dividers[i++] = *dv1;

			*dv1 += 1. * incr;
			*dv2 += 1. * incr;
			gsd_color_func(0x0);
			gsd_bgnline();
			glVertex2fv(vert1);
			glVertex2fv(vert2);
			gsd_endline();

			*dv1 += 1. * incr;
			*dv2 += 1. * incr;
			pdcell = tdcell;
			continue;
		    }
		}
	    }

	    gsd_color_func(colr);
	    gsd_bgnline();
	    glVertex2fv(vert1);
	    glVertex2fv(vert2);
	    gsd_endline();
	    glFlush();
	    *dv1 += 1. * incr;
	    *dv2 += 1. * incr;
	    pdcell = tdcell;
	}

	/* Black box */
	vert1[X] = vert2[X] = 1. + xoff;
	vert1[Y] = vert4[Y] = 1. + yoff;
	vert3[X] = vert4[X] = sr - sl - 1. + xoff;
	vert3[Y] = vert2[Y] = st - sb - 1. + yoff;

	gsd_color_func(0x000000);
	gsd_bgnline();
	glVertex2fv(vert1);
	glVertex2fv(vert2);
	glVertex2fv(vert3);
	glVertex2fv(vert4);
	glVertex2fv(vert1);
	gsd_endline();

	/* White box */
	vert1[X] = vert2[X] = xoff;
	vert1[Y] = vert4[Y] = yoff;
	vert3[X] = vert4[X] = sr - sl + xoff;
	vert3[Y] = vert2[Y] = st - sb + yoff;

	gsd_color_func(0xFFFFFF);
	gsd_bgnline();
	glVertex2fv(vert1);
	glVertex2fv(vert2);
	glVertex2fv(vert3);
	glVertex2fv(vert4);
	glVertex2fv(vert1);
	gsd_endline();

	/* draw discrete dividers */
	if (dividers) {
	    gsd_color_func(0xFFFFFFFF);
	    *sv1 = stab1;
	    *sv2 = stab2;
	    for (k = 0; k < i; k++) {
		*dv1 = *dv2 = dividers[k];
		gsd_bgnline();
		glVertex2fv(vert1);
		glVertex2fv(vert2);
		gsd_endline();
	    }
	}

	if (cat_labs || cat_vals) {
	    labpt[Z] = 0;
	    for (k = 0; k < numlabs; k++) {
		if (is_fp) {
		    if (discrete && Listnum) {
			tdcell = Listcats[k];
			labpos = (k + .5) / numlabs;
		    }
		    else {
			/* show_all not supported unless Listnum */
			tdcell = labvals[k];
			labpos = (tdcell - fmin) / (fmax - fmin);
		    }
		}
		else {
		    if (discrete && Listnum) {
			tcell = Listcats[k];
			labpos = (k + .5) / numlabs;
		    }
		    else {
			tcell = discrete ? min + k : labvals[k];
			labpos = (tcell - min + .5) / (max - min + 1);
		    }
		}
		if (do_invert)
		    labpos = 1. - labpos;
		if (cat_labs) {
		    if (!is_fp)
			cstr = Rast_get_c_cat(&tcell, &cats);
		    else
			cstr = Rast_get_d_cat(&tdcell, &cats);
		}
		if (cat_labs && !cat_vals)
		    sprintf(buff, "%s", cstr);
		else {
		    if (cat_labs && cat_vals) {
			if (cstr)
			    if (is_fp)
				sprintf(buff, "%.*lf) %s",
					fprec, tdcell, cstr);
			    else
				sprintf(buff, "%*d) %s", iprec, tcell, cstr);
			else if (is_fp)
			    sprintf(buff, "%.*lf", fprec, tdcell);
			else
			    sprintf(buff, "%d", tcell);
		    }
		    else if (cat_vals) {
			if (is_fp)
			    sprintf(buff, "%.*lf", fprec, tdcell);
			else
			    sprintf(buff, "%d", tcell);
		    }
		}
		if (horiz) {
		    labpt[X] = labpos * (sr - sl) + xoff -
			gsd_get_txtwidth(buff, size) / 2 - get_txtxoffset();
		    labpt[Y] =
			st - sb + yoff + 3 + gsd_get_txtheight(size) / 2;
		}
		else {
		    labpt[X] = sr - sl + xoff + get_txtxoffset() + 3;
		    /*
		       labpt[Y] = labpos * (st - sb) + yoff - 
		       gsd_get_txtheight()/2 + get_txtdescender();
		     */
		    labpt[Y] = labpos * (st - sb) + yoff -
			gsd_get_txtheight(size);
		}
		/* set color for black text -- maybe add option for color 
		 * supplied with font ??
		 */
		gsd_color_func(0x000000);
		do_label_display(fontbase, labpt, buff);
	    }
	}

	if (discrete)
	    G_free(dividers);
    }

    if (cat_labs)
	Rast_free_cats(&cats);

    Rast_free_colors(&colors);

    gsd_end_legend_viewport();

    /*
       gsd_unset_font(fontbase);
     */

    gsd_endlist();

    return (legend_list);
}
