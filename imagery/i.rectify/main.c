
/****************************************************************************
 *
 * MODULE:       i.rectify
 * AUTHOR(S):    William R. Enslin, Michigan State U. (original contributor)
 *               Luca Palmeri <palmeri ux1.unipd.it>
 *               Bill Hughes,
 *               Pierre de Mouveaux <pmx audiovu.com>,
 *               Bob Covill (original CMD version), 
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Markus Metz
 * PURPOSE:      calculate a transformation matrix and then convert x,y cell 
 *               coordinates to standard map coordinates for each pixel in the 
 *               image (control points can come from i.points or i.vpoints)
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/raster.h>
#include <grass/glocale.h>

#include "global.h"
#include "crs.h"

char *seg_mb;

int temp_fd;
RASTER_MAP_TYPE map_type;
char *temp_name;
int *ref_list;
char **new_name;
struct Ref ref;

func interpolate;

/* georef coefficients */

double E12[10], N12[10];
double E21[10], N21[10];

/* DELETED WITH CRS MODIFICATIONS
   double E12a, E12b, E12c, N12a, N12b, N12c;
   double E21a, E21b, E21c, N21a, N21b, N21c;
 */
struct Cell_head target_window;

#define NFILES 15   /* ??? */

void err_exit(char *, char *);

/* modify this table to add new methods */
struct menu menu[] = {
    {p_nearest, "nearest", "nearest neighbor"},
    {p_bilinear, "bilinear", "bilinear"},
    {p_cubic, "cubic", "cubic convolution"},
    {p_lanczos, "lanczos", "lanczos filter"},
    {p_bilinear_f, "bilinear_f", "bilinear with fallback"},
    {p_cubic_f, "cubic_f", "cubic convolution with fallback"},
    {p_lanczos_f, "lanczos_f", "lanczos filter with fallback"},
    {NULL, NULL, NULL}
};

static char *make_ipol_list(void);

int main(int argc, char *argv[])
{
    char group[INAME_LEN], extension[INAME_LEN];
    char result[NFILES][15];
    int order;			/* ADDED WITH CRS MODIFICATIONS */
    char *ipolname;		/* name of interpolation method */
    int method;
    int n, i, m, k = 0;
    int got_file = 0;

    struct Option *grp,         /* imagery group */
     *val,                      /* transformation order */
     *ifile,			/* input files */
     *ext,			/* extension */
     *tres,			/* target resolution */
     *mem,			/* amount of memory for cache */
     *interpol;			/* interpolation method:
				   nearest neighbor, bilinear, cubic */
    struct Flag *c, *a;
    struct GModule *module;

    struct Cell_head cellhd;

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("rectify"));
    module->description =
	_("Rectifies an image by computing a coordinate "
	  "transformation for each pixel in the image based on the "
	  "control points.");

    grp = G_define_standard_option(G_OPT_I_GROUP);

    ifile = G_define_standard_option(G_OPT_R_INPUTS);
    ifile->required = NO;

    ext = G_define_option();
    ext->key = "extension";
    ext->type = TYPE_STRING;
    ext->required = YES;
    ext->multiple = NO;
    ext->description = _("Output raster map(s) suffix");

    val = G_define_option();
    val->key = "order";
    val->type = TYPE_INTEGER;
    val->required = YES;
    val->description = _("Rectification polynom order (1-3)");

    tres = G_define_option();
    tres->key = "res";
    tres->type = TYPE_DOUBLE;
    tres->required = NO;
    tres->description = _("Target resolution (ignored if -c flag used)");

    mem = G_define_option();
    mem->key = "memory";
    mem->type = TYPE_DOUBLE;
    mem->key_desc = "memory in MB";
    mem->required = NO;
    mem->answer = "300";
    mem->description = _("Amount of memory to use in MB");

    ipolname = make_ipol_list();

    interpol = G_define_option();
    interpol->key = "method";
    interpol->type = TYPE_STRING;
    interpol->required = NO;
    interpol->answer = "nearest";
    interpol->options = ipolname;
    interpol->description = _("Interpolation method to use");

    c = G_define_flag();
    c->key = 'c';
    c->description =
	_("Use current region settings in target location (def.=calculate smallest area)");

    a = G_define_flag();
    a->key = 'a';
    a->description = _("Rectify all raster maps in group");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* get the method */
    for (method = 0; (ipolname = menu[method].name); method++)
	if (strcmp(ipolname, interpol->answer) == 0)
	    break;

    if (!ipolname)
	G_fatal_error(_("<%s=%s> unknown %s"),
		      interpol->key, interpol->answer, interpol->key);
    interpolate = menu[method].method;

    G_strip(grp->answer);
    strcpy(group, grp->answer);
    strcpy(extension, ext->answer);
    order = atoi(val->answer);

    seg_mb = NULL;
    if (mem->answer) {
	if (atoi(mem->answer) > 0)
	    seg_mb = mem->answer;
    }

    if (!ifile->answers)
	a->answer = 1;		/* force all */

    /* Find out how files on command line */
    if (!a->answer) {
	for (m = 0; ifile->answers[m]; m++) {
	    k = m;
	}
	k++;
    }

    if (order < 1 || order > MAXORDER)
	G_fatal_error(_("Invalid order (%d); please enter 1 to %d"), order,
		      MAXORDER);

    if (!(seg_mb > 0))
	G_fatal_error(_("Amount of memory to use in MB must be > 0"));

    /* determine the number of files in this group */
    if (I_get_group_ref(group, &ref) <= 0) {
	G_warning(_("Location: %s"), G_location());
	G_warning(_("Mapset: %s"), G_mapset());
	G_fatal_error(_("Group <%s> does not exist"), grp->answer);
    }

    if (ref.nfiles <= 0) {
	G_important_message(_("Group <%s> contains no raster maps; run i.group"),
			    grp->answer);
	exit(EXIT_SUCCESS);
    }

    for (i = 0; i < NFILES; i++)
	result[i][0] = 0;

    ref_list = (int *)G_malloc(ref.nfiles * sizeof(int));
    new_name = (char **)G_malloc(ref.nfiles * sizeof(char *));

    if (a->answer) {
	for (n = 0; n < ref.nfiles; n++) {
	    ref_list[n] = -1;
	}
    }
    else {
	char xname[GNAME_MAX], xmapset[GMAPSET_MAX], *name;
	for (m = 0; m < k; m++) {
	    got_file = 0;
	    if (G_name_is_fully_qualified(ifile->answers[m], xname, xmapset))
		name = xname;
	    else
		name = ifile->answers[m];

	    for (n = 0; n < ref.nfiles; n++) {
		ref_list[n] = 1;
		if (strcmp(name, ref.file[n].name) == 0) {
		    got_file = 1;
		    ref_list[n] = -1;
		    break;
		}
	    }
	    if (got_file == 0)
		err_exit(ifile->answers[m], group);
	}
    }

    /* read the control points for the group */
    get_control_points(group, order);

    /* get the target */
    get_target(group);

    if (!c->answer) {
	double res = -1;
	
	if (tres->answer) {
	    if (!((res = atof(tres->answer)) > 0))
		G_warning(_("Target resolution must be > 0, ignored"));
	}
	/* Calculate smallest region */
	if (a->answer)
	    Rast_get_cellhd(ref.file[0].name, ref.file[0].mapset, &cellhd);
	else
	    Rast_get_cellhd(ifile->answers[0], ref.file[0].mapset, &cellhd);

	georef_window(&cellhd, &target_window, order, res);
    }

    G_verbose_message(_("Using region: N=%f S=%f, E=%f W=%f"), target_window.north,
	      target_window.south, target_window.east, target_window.west);

    exec_rectify(order, extension, interpol->answer);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}


void err_exit(char *file, char *grp)
{
    int n;

    G_warning(_("Input raster map <%s> does not exist in group <%s>."),
	    file, grp);
    G_message(_("Try:"));

    for (n = 0; n < ref.nfiles; n++)
	G_message("%s", ref.file[n].name);

    G_fatal_error(_("Exit!"));
}

static char *make_ipol_list(void)
{
    int size = 0;
    int i;
    char *buf;

    for (i = 0; menu[i].name; i++)
	size += strlen(menu[i].name) + 1;

    buf = G_malloc(size);
    *buf = '\0';

    for (i = 0; menu[i].name; i++) {
	if (i)
	    strcat(buf, ",");
	strcat(buf, menu[i].name);
    }

    return buf;
}
