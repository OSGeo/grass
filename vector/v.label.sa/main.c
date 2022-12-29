/* ****************************************************************************
 *
 * MODULE:       v.label.sa
 * AUTHOR(S):    Wolf Bergenheim
 * PURPOSE:      Create paint labels, but use a Simulated Annealing
 *               algorithm to avoid overlapping labels.
 *               This file contains the command line parsing and main function.
 *               The paint label file writing function (print_label()) is also
 *               part of this file.
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <time.h>
#include "labels.h"
#define DEFAULT_CHARSET "UTF-8"

/**
 * The main function controls the program flow.
 */
int main(int argc, char *argv[])
{
    struct params p;
    label_t *labels;
    int n_labels, i;
    struct GModule *module;
    FILE *labelf;

    srand((unsigned int)time(NULL));

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("paint labels"));
    module->description =
	_("Create optimally placed labels for vector map(s)");

    /* parse options and flags */
    p.map = G_define_standard_option(G_OPT_V_MAP);

    p.type = G_define_standard_option(G_OPT_V_TYPE);
    p.type->options = "point,line,area";
    p.type->answer = "point,line,area";

    p.layer = G_define_standard_option(G_OPT_V_FIELD);

    p.column = G_define_option();
    p.column->key = "column";
    p.column->type = TYPE_STRING;
    p.column->required = YES;
    p.column->description =
	_("Name of attribute column to be used for labels");

    p.labels = G_define_option();
    p.labels->key = "labels";
    p.labels->description = _("Name for new paint-label file");
    p.labels->type = TYPE_STRING;
    p.labels->required = YES;
    p.labels->key_desc = "name";

    p.font = G_define_option();
    p.font->key = "font";
    p.font->type = TYPE_STRING;
    p.font->required = YES;
    p.font->description =
	_("Name of TrueType font (as listed in the fontcap)");
    p.font->guisection = _("Font");
    p.font->gisprompt = "font";

    p.size = G_define_option();
    p.size->key = "size";
    p.size->description = _("Label size (in map-units)");
    p.size->type = TYPE_DOUBLE;
    p.size->answer = "100";
    p.size->guisection = _("Font");

    p.isize = G_define_option();
    p.isize->key = "isize";
    p.isize->description = _("Icon size of point features (in map-units)");
    p.isize->type = TYPE_DOUBLE;
    p.isize->answer = "10";

    p.charset = G_define_option();
    p.charset->key = "charset";
    p.charset->type = TYPE_STRING;
    p.charset->required = NO;
    p.charset->answer = DEFAULT_CHARSET;
    p.charset->description =
	"Character encoding (default: " DEFAULT_CHARSET ")";

    p.color = G_define_option();
    p.color->key = "color";
    p.color->description = _("Text color");
    p.color->type = TYPE_STRING;
    p.color->answer = "black";
    p.color->options = "aqua,black,blue,brown,cyan,gray,green,grey,indigo,"
	"magenta,orange,purple,red,violet,white,yellow";
    p.color->guisection = _("Colors");

    p.hlcolor = G_define_option();
    p.hlcolor->key = "hcolor";
    p.hlcolor->description = _("Highlight color for text");
    p.hlcolor->type = TYPE_STRING;
    p.hlcolor->answer = "none";
    p.hlcolor->options =
	"none,aqua,black,blue,brown,cyan,gray,green,grey,indigo,"
	"magenta,orange,purple,red,violet,white,yellow";
    p.hlcolor->guisection = _("Colors");

    p.hlwidth = G_define_option();
    p.hlwidth->key = "hwidth";
    p.hlwidth->description = _("Width of highlight coloring");
    p.hlwidth->type = TYPE_DOUBLE;
    p.hlwidth->answer = "0";
    p.hlwidth->guisection = _("Colors");

    p.bgcolor = G_define_option();
    p.bgcolor->key = "background";
    p.bgcolor->description = _("Background color");
    p.bgcolor->type = TYPE_STRING;
    p.bgcolor->answer = "none";
    p.bgcolor->options =
	"none,aqua,black,blue,brown,cyan,gray,green,grey,indigo,"
	"magenta,orange,purple,red,violet,white,yellow";
    p.bgcolor->guisection = _("Colors");

    p.opaque = G_define_option();
    p.opaque->key = "opaque";
    p.opaque->description =
	_("Opaque to vector (only relevant if background color is selected)");
    p.opaque->type = TYPE_STRING;
    p.opaque->answer = "yes";
    p.opaque->options = "yes,no";
    p.opaque->key_desc = "yes|no";
    p.opaque->guisection = _("Colors");

    p.bocolor = G_define_option();
    p.bocolor->key = "border";
    p.bocolor->description = _("Border color");
    p.bocolor->type = TYPE_STRING;
    p.bocolor->answer = "none";
    p.bocolor->options =
	"none,aqua,black,blue,brown,cyan,gray,green,grey,indigo,"
	"magenta,orange,purple,red,violet,white,yellow";
    p.bocolor->guisection = _("Colors");

    p.bowidth = G_define_option();
    p.bowidth->key = "width";
    p.bowidth->description = _("Border width (only for ps.map output)");
    p.bowidth->type = TYPE_DOUBLE;
    p.bowidth->answer = "0";
    p.bowidth->guisection = _("Colors");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* initialize labels (get text from database, and get features) */
    labels = labels_init(&p, &n_labels);
    /* start algorithm */
    /*   1. candidate position generation */
    label_candidates(labels, n_labels);
    /*   2. position evaluation */
    label_candidate_overlap(labels, n_labels);
    /*   3. position selection */
    simulate_annealing(labels, n_labels, &p);
    /* write labels to file */
    fprintf(stderr, "Writing labels to file: ...");
    labelf = G_fopen_new("paint/labels", p.labels->answer);
    for (i = 0; i < n_labels; i++) {
	if (labels[i].n_candidates > 0) {
	    print_label(labelf, &labels[i], &p);
	}
	G_percent(i, (n_labels - 1), 1);
    }
    fclose(labelf);

    return EXIT_SUCCESS;
}

void print_label(FILE * labelf, label_t * label, struct params *p)
{
    int cc, hlwidth;
    double size;

    cc = label->current_candidate;
    size = atof(p->size->answer);
    hlwidth = atoi(p->hlwidth->answer);

    fprintf(labelf, "east: %lf\n", label->candidates[cc].point.x);
    fprintf(labelf, "north: %lf\n", label->candidates[cc].point.y);
    fprintf(labelf, "xoffset: %lf\n", 0.0);	/*  * (size)); */
    fprintf(labelf, "yoffset: %lf\n", 0.0);	/*  * (size)); */
    fprintf(labelf, "ref: %s\n", "bottom left");

    fprintf(labelf, "font: %s\n", p->font->answer);
    fprintf(labelf, "color: %s\n", p->color->answer);

    fprintf(labelf, "size: %lf\n", size);

    fprintf(labelf, "width: %s\n", p->bowidth->answer);
    fprintf(labelf, "hcolor: %s\n", p->hlcolor->answer);
    fprintf(labelf, "hwidth: %d\n", hlwidth);
    fprintf(labelf, "background: %s\n", p->bgcolor->answer);
    fprintf(labelf, "border: %s\n", p->bocolor->answer);
    fprintf(labelf, "opaque: %s\n", p->opaque->answer);
    fprintf(labelf, "rotate: %f\n",
	    label->candidates[cc].rotation * 180.0 / M_PI);
    fprintf(labelf, "text:%s\n\n", label->text);

    return;
}
