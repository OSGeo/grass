/* Functions: symbol_save, symbol_draw 
 **
 ** Author: Radim Blazek
 */

#include <grass/vector.h>
#include <grass/symbol.h>
#include "clr.h"
#include "local_proto.h"
#include "vector.h"

/* draw chain */
int draw_chain(SYMBCHAIN * chain, double s)
{
    int k, l;
    char *mvcmd;
    SYMBEL *elem;

    for (k = 0; k < chain->count; k++) {
	elem = chain->elem[k];
	switch (elem->type) {
	case S_LINE:
	    for (l = 0; l < elem->coor.line.count; l++) {
		if (k == 0 && l == 0)
		    mvcmd = "M";
		else
		    mvcmd = "LN";
		fprintf(PS.fp, "%.4f %.4f %s\n",
			s * elem->coor.line.x[l],
			s * elem->coor.line.y[l], mvcmd);
	    }
	    break;
	case S_ARC:
	    if (elem->coor.arc.clock)
		mvcmd = "arcn";
	    else
		mvcmd = "arc";
	    fprintf(PS.fp, "%.4f %.4f %.4f %.4f %.4f %s\n",
		    s * elem->coor.arc.x,
		    s * elem->coor.arc.y,
		    s * elem->coor.arc.r,
		    elem->coor.arc.a1, elem->coor.arc.a2, mvcmd);
	    break;
	}
    }

    return 0;
}

int
symbol_draw(char *name, double x, double y, double size, double rotate,
	    double width)
{
    fprintf(PS.fp, "gsave\n");
    fprintf(PS.fp, "%.5f %.5f translate\n", x, y);
    fprintf(PS.fp, "%.5f %.5f scale\n", size, size);
    fprintf(PS.fp, "%.5f rotate\n", rotate);
    fprintf(PS.fp, "%.8f W\n", width / size);
    fprintf(PS.fp, "%s\n", name);
    fprintf(PS.fp, "grestore\n");

    return 0;
}

/* store symbol in PS file, scaled to final size and drawn with final colors */
int symbol_save(SYMBOL * Symb, PSCOLOR * color, PSCOLOR * fcolor, char *name)
{
    SYMBPART *part;
    SYMBCHAIN *chain;
    int points;
    int i, j;
    double s, xo[4], yo[4];

    points = 4;
    xo[0] = 0.0;
    yo[0] = 0.5;
    xo[1] = -0.5;
    yo[1] = 0.0;
    xo[2] = 0.0;
    yo[2] = -0.5;
    xo[3] = 0.5;
    yo[3] = 0.0;

    s = 1;

    fprintf(PS.fp, "\n/%s {\n", name);
    if (Symb != NULL) {
	s *= Symb->scale;
	for (i = 0; i < Symb->count; i++) {
	    part = Symb->part[i];
	    switch (part->type) {
	    case S_POLYGON:
		fprintf(PS.fp, "NP\n");	/* Start ring */
		for (j = 0; j < part->count; j++) {	/* RINGS */
		    chain = part->chain[j];
		    draw_chain(chain, s);
		    fprintf(PS.fp, "CP\n");	/* Close one ring */
		}
		/* Fill */
		if (part->fcolor.color == S_COL_DEFAULT &&
		    !color_none(fcolor)) {
		    set_ps_color(fcolor);
		    fprintf(PS.fp, "F\n");	/* Fill polygon */
		}
		else if (part->fcolor.color == S_COL_DEFINED) {
		    fprintf(PS.fp, "%.3f %.3f %.3f C\n", part->fcolor.fr,
			    part->fcolor.fg, part->fcolor.fb);
		    fprintf(PS.fp, "F\n");
		}
		/* Outline */
		if (part->color.color == S_COL_DEFAULT && !color_none(color)) {
		    set_ps_color(color);
		    fprintf(PS.fp, "D\n");	/* Draw boundary */
		}
		else if (part->color.color == S_COL_DEFINED) {
		    fprintf(PS.fp, "%.3f %.3f %.3f C\n", part->color.fr,
			    part->color.fg, part->color.fb);
		    fprintf(PS.fp, "D\n");
		}
		break;
	    case S_STRING:	/* string has 1 chain */
		if (part->color.color != S_COL_NONE) {
		    fprintf(PS.fp, "NP\n");
		    chain = part->chain[0];
		    draw_chain(chain, s);
		    /* Color */
		    if (part->color.color == S_COL_DEFAULT &&
			!color_none(color)) {
			set_ps_color(color);
			fprintf(PS.fp, "D\n");
		    }
		    else {
			fprintf(PS.fp, "%.3f %.3f %.3f C\n", part->color.fr,
				part->color.fg, part->color.fb);
			fprintf(PS.fp, "D\n");
		    }
		}
		break;
	    }
	}
    }
    else {
	fprintf(PS.fp, "%.4f %.4f NM\n", s * xo[0], s * yo[0]);
	for (j = 1; j < points; j++)
	    fprintf(PS.fp, "%.4f %.4f LN\n", s * xo[j], s * yo[j]);
	fprintf(PS.fp, "CP\n");

	set_ps_color(fcolor);
	fprintf(PS.fp, "F\n");
	set_ps_color(color);
	fprintf(PS.fp, "D\n");
    }
    fprintf(PS.fp, "} def\n");

    return 0;
}
