/* Function: do_plt
 **
 ** Author: Paul W. Carlson     March 1992
 */

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/symbol.h>
#include "clr.h"
#include "local_proto.h"

int do_plt(int after_masking)
{
    FILE *fp;
    char buf[1024], symb[1024], sname[100];
    char name[1024] /*, prev_name[50] */;
    double e1, n1, e2, n2, llx, lly, urx, ury;
    int color_R, color_G, color_B;
    int fcolor_R, fcolor_G, fcolor_B;
    int masked;
    double size, scale, rotate;
    int i;
    int x_int, y_int;
    double width, x, y, x_off, y_off;
    PSCOLOR pcolor, pfcolor;
    SYMBOL *Symb;
    static int snum = 0;

    if (PS.plfile == NULL)
	return 1;
    fp = fopen(PS.plfile, "r");
    if (fp == NULL) {
	error("point/line file", "", "can't open");
	return 1;
    }

    G_message(_("Reading point/line file ..."));

    while (fgets(buf, sizeof buf, fp))
	switch (*buf) {
	case 'L':
	    if (sscanf(buf, "L %d %lf %lf %lf %lf %d %d %d %lf", &masked, &e1,
		       &n1, &e2, &n2, &color_R, &color_G, &color_B,
		       &width) == 9) {
		if (masked && after_masking)
		    continue;
		if (!masked && !after_masking)
		    continue;

		if (color_R == -1)
		    continue;	/* ie color was set to "none" */

		set_color(&pcolor, color_R, color_G, color_B);
		set_ps_color(&pcolor);
		set_line_width(width);

		if (G_projection() == PROJECTION_LL) {
		/* use the same method as the vlines instruction,
		for completely out of region lines just move, don't put
		the pen down. but the thing this method has going for it
		is that for lat/lon (AFAIU) it draws the line twice,
		so that at least one of them will go the desired way
		around the sphere, and the other will be discarded.
		(I think). But it means your self-defined lines need to
		keep at least one little toe in the map box. --HB Feb2012 */
		    start_line(e1, n1);
		    sec_draw = 0;
		    G_plot_line(e1, n1, e2, n2);
		}
		else {
		    G_plot_where_xy(e1, n1, &x_int, &y_int);
		    llx = (double)x_int / 10.;
		    lly = (double)y_int / 10.;

		    G_plot_where_xy(e2, n2, &x_int, &y_int);
		    urx = (double)x_int / 10.;
		    ury = (double)y_int / 10.;

		    fprintf(PS.fp, " %.1f %.1f NM %.1f %.1f LN",
			    llx, lly, urx, ury);
		}
		fprintf(PS.fp, " stroke\n");

	    }
	    break;

	case 'R':
	    if (sscanf(buf, "R %d %lf %lf %lf %lf %d %d %d %d %d %d %lf",
		       &masked, &e1, &n1, &e2, &n2, &color_R, &color_G,
		       &color_B, &fcolor_R, &fcolor_G, &fcolor_B,
		       &width) == 12) {
		if (masked && after_masking)
		    continue;
		if (!masked && !after_masking)
		    continue;

		fprintf(PS.fp, " NP\n");
		G_plot_where_xy(e1, n1, &x_int, &y_int);
		llx = (double)x_int / 10.;
		lly = (double)y_int / 10.;
		G_plot_where_xy(e2, n2, &x_int, &y_int);
		urx = (double)x_int / 10.;
		ury = (double)y_int / 10.;

		fprintf(PS.fp, " %.1f %.1f M %.1f %.1f LN\n", llx, lly, urx,
			lly);
		fprintf(PS.fp, " %.1f %.1f LN %.1f %.1f LN\n", urx, ury, llx,
			ury);
		fprintf(PS.fp, " CP\n");

		if (fcolor_R != -1) {	/* ie color is not set to "none" */
		    set_color(&pfcolor, fcolor_R, fcolor_G, fcolor_B);
		    set_ps_color(&pfcolor);
		    fprintf(PS.fp, " F\n");
		}

		if (color_R != -1) {	/* ie color is not set to "none" */
		    set_color(&pcolor, color_R, color_G, color_B);
		    set_ps_color(&pcolor);
		    set_line_width(width);
		    fprintf(PS.fp, " D\n");
		}
	    }
	    break;

	case 'P':
	    i = sscanf(buf, "P %d %lf %lf %d %d %d %d %d %d %lf %lf %s %lf",
		       &masked, &e1, &n1, &color_R, &color_G, &color_B,
		       &fcolor_R, &fcolor_G, &fcolor_B, &size, &rotate, symb,
		       &width);
	    if (i == 13) {
		if (masked && after_masking)
		    continue;
		if (!masked && !after_masking)
		    continue;

		if (size <= 0.0)
		    size = 10;

		/* HB 7/2005: allow all points through as only way to generate
		   one is explicitly with the ps.map "point" instruction. */
		/*
		   if (n1 > PS.w.north || n1 < PS.w.south) continue;
		   if (e1 > PS.w.east  || e1 < PS.w.west ) continue;
		 */
		G_plot_where_xy(e1, n1, &x_int, &y_int);
		x = (double)x_int / 10.;
		y = (double)y_int / 10.;


		if (color_R == -1)
		    unset_color(&pcolor);
		else
		    set_color(&pcolor, color_R, color_G, color_B);

		if (fcolor_R == -1)
		    unset_color(&pfcolor);
		else
		    set_color(&pfcolor, fcolor_R, fcolor_G, fcolor_B);

		if (width < 0) /* default: autoscale relative to size */
		    width = 0.05 * size;

		/* Read symbol */
		sprintf(sname, "POINTSYMBOL%d", snum);
		Symb = S_read(symb);
		if (Symb == NULL)
		    G_warning(_("Cannot read symbol, using default icon"));
		symbol_save(Symb, &pcolor, &pfcolor, sname);
		symbol_draw(sname, x, y, size, rotate, width);

		snum++;
	    }
	    break;

	case 'E':		/* EPS file */
	    if (sscanf(buf, "E %d %lf %lf %lf %lf %[^\n]s",
		       &masked, &e1, &n1, &scale, &rotate, name) == 6) {
		if (masked && after_masking)
		    continue;
		if (!masked && !after_masking)
		    continue;

		/* find eps bbox */
		if (!eps_bbox(name, &llx, &lly, &urx, &ury))
		    continue;

		G_plot_where_xy(e1, n1, &x_int, &y_int);
		x = (double)x_int / 10.;
		y = (double)y_int / 10.;

		/* calculate translation */
		eps_trans(llx, lly, urx, ury, x, y, scale, rotate, &x_off,
			  &y_off);

		/* write eps to PS */
		eps_draw(PS.fp, name, x_off, y_off, scale, rotate);
	    }
	    break;
	}

    fclose(fp);

    return 0;
}
