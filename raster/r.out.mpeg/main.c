
/****************************************************************************
 *
 * MODULE:       r.out.mpeg
 * AUTHOR(S):    Bill Brown, CERL (original contributor)
 *               Brad Douglas <rez touchofmadness.com>, Markus Neteler <neteler itc.it>,
 *               Glynn Clements <glynn gclements.plus.com>, Hamish Bowman <hamish_b yahoo.com>,
 *               Jan-Oliver Wagner <jan intevation.de>, Paul Kelly <paul-grass stjohnspoint.co.uk>
 *               Paolo Zatelli <paolo.zatelli unitn.it>
 *
 * PURPOSE:      combines a series of GRASS raster maps into a single MPEG-1
 * COPYRIGHT:    (C) 1999-2006, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* Written by Bill Brown, USACERL (brown@zorro.cecer.army.mil)
 * May, 1994
 *
 * This code is in the public domain. Specifically, we give to the public
 * domain all rights for future licensing of the source code, all resale
 * rights, and all publishing rights.
 * 
 * We ask, but do not require, that the following message be included in
 * all derived works:
 *     "Portions developed at the US Army Construction Engineering 
 *     Research Laboratories, Champaign, Illinois."
 * 
 * USACERL GIVES NO WARRANTY, EXPRESSED OR IMPLIED,
 * FOR THE SOFTWARE AND/OR DOCUMENTATION PROVIDED, INCLUDING, WITHOUT
 * LIMITATION, WARRANTY OF MERCHANTABILITY AND WARRANTY OF FITNESS FOR A
 * PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/spawn.h>
#include <grass/glocale.h>

#include "rom_proto.h"

#define MAXIMAGES 400
#define DEF_MAX 500
#define DEF_MIN 200
#define MAXVIEWS    4
#define BORDER_W    2


/* global variables */
int nrows, ncols, numviews, quality;
char *vfiles[MAXVIEWS][MAXIMAGES];
char outfile[GPATH_MAX];
const char *encoder;

float vscale, scale;		/* resampling scale factors */
int irows, icols, vrows, vcols;
int frames;


/* function prototypes */
static int load_files(void);
static int use_r_out(void);
static char **gee_wildfiles(const char *wildarg, const char *element, int *num);
static void parse_command(struct Option **viewopts,
			  char *vfiles[MAXVIEWS][MAXIMAGES],
			  int *numviews, int *numframes);

static int check_encoder(const char *encoder)
{
    int status, prev;

    prev = G_suppress_warnings(1);

    status = G_spawn_ex(
	encoder, encoder,
	SF_REDIRECT_FILE, SF_STDERR, SF_MODE_OUT, G_DEV_NULL,
	NULL);

    G_suppress_warnings(prev);

    return status >= 0 && status != 127;
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *viewopts[MAXVIEWS], *out, *qual;
    struct Flag *conv;
    int i;
    int *sdimp, longdim, r_out;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    G_add_keyword(_("animation"));

    module->description =
	_("Converts raster map series to MPEG movie.");

    for (i = 0; i < MAXVIEWS; i++) {
	char *buf = NULL;
	viewopts[i] = G_define_standard_option(G_OPT_R_INPUTS);
	G_asprintf(&buf, "view%d", i + 1);
	viewopts[i]->key = G_store(buf);
	viewopts[i]->required = (i ? NO : YES);
	G_asprintf(&buf, _("Name of input raster map(s) for view no.%d"), i + 1);
	viewopts[i]->description = G_store(buf);
        viewopts[i]->guisection = _("Views");
	G_free(buf);
    }

    out = G_define_standard_option(G_OPT_F_OUTPUT);
    
    qual = G_define_option();
    qual->key = "quality";
    qual->type = TYPE_INTEGER;
    qual->required = NO;
    qual->multiple = NO;
    qual->answer = "3";
    qual->options = "1-5";
    qual->description =
	_("Quality factor (1 = highest quality, lowest compression)");
    qual->guisection = _("Settings");
    
    conv = G_define_flag();
    conv->key = 'c';
    conv->label = _("Convert on the fly, uses less disk space");
    conv->description =	_("Requires r.out.ppm with stdout option");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parse_command(viewopts, vfiles, &numviews, &frames);

    /* output file */
    strcpy(outfile, out->answer);

    r_out = 0;
    if (conv->answer)
	r_out = 1;

    quality = 3;
    if (qual->answer != NULL)
	sscanf(qual->answer, "%d", &quality);
    if (quality > 5 || quality < 1)
	quality = 3;

    /* find a working encoder */
    if (check_encoder("ppmtompeg"))
	encoder = "ppmtompeg";
    else if (check_encoder("mpeg_encode"))
	encoder = "mpeg_encode";
    else
	G_fatal_error(_("Either mpeg_encode or ppmtompeg must be installed"));

    G_debug(1, "encoder = [%s]", encoder);

    vrows = Rast_window_rows();
    vcols = Rast_window_cols();
    nrows = vrows;
    ncols = vcols;

    /* short dimension */
    sdimp = nrows > ncols ? &ncols : &nrows;

    /* these proportions should work fine for 1 or 4 views, but for
       2 views, want to double the narrow dim & for 3 views triple it */
    if (numviews == 2)
	*sdimp *= 2;
    else if (numviews == 3)
	*sdimp *= 3;

    longdim = nrows > ncols ? nrows : ncols;

    scale = 1.0;

    {	/* find animation image size */
	int max, min;
	char *p;

	max = DEF_MAX;
	min = DEF_MIN;

	if ((p = getenv("GMPEG_SIZE")))
	    max = min = atoi(p);

	if (longdim > max)	/* scale down */
	    scale = (float)max / longdim;
	else if (longdim < min)	/* scale up */
	    scale = (float)min / longdim;
    }
    /* TODO: align image size to 16 pixel width & height */

    vscale = scale;
    if (numviews == 4)
	vscale = scale / 2.;

    nrows *= scale;
    ncols *= scale;
    /* now nrows & ncols are the size of the combined - views image */
    vrows *= vscale;
    vcols *= vscale;
    /* now vrows & vcols are the size for each sub-image */

    /* add to nrows & ncols for borders */
    /* irows, icols used for vert/horizontal determination in loop below */
    irows = nrows;
    icols = ncols;
    nrows += (1 + (nrows / vrows)) * BORDER_W;
    ncols += (1 + (ncols / vcols)) * BORDER_W;

    if (numviews == 1 && r_out)
	use_r_out();
    else
	load_files();

    return (EXIT_SUCCESS);
}


static int load_files(void)
{
    void *voidc;
    int rtype;
    register int i, rowoff, row, col, vxoff, vyoff, offset;
    int cnt, fd, size, tsiz, coff;
    int vnum;
    int y_rows, y_cols;
    char *pr, *pg, *pb;
    unsigned char *tr, *tg, *tb, *tset;
    char *mpfilename, *name;
    char *yfiles[MAXIMAGES];
    struct Colors colors;
    int ret;

    size = nrows * ncols;

    pr = G_malloc(size);
    pg = G_malloc(size);
    pb = G_malloc(size);

    tsiz = Rast_window_cols();

    tr = (unsigned char *)G_malloc(tsiz);
    tg = (unsigned char *)G_malloc(tsiz);
    tb = (unsigned char *)G_malloc(tsiz);
    tset = (unsigned char *)G_malloc(tsiz);

    for (cnt = 0; cnt < frames; cnt++) {
	if (cnt > MAXIMAGES) {
	    cnt--;
	    break;
	}

	for (i = 0; i < size; i++)
	    pr[i] = pg[i] = pb[i] = 0;

	for (vnum = 0; vnum < numviews; vnum++) {
	    if (icols == vcols) {
		vxoff = BORDER_W;
		vyoff = (irows == vrows) ? BORDER_W :
		    BORDER_W + vnum * (BORDER_W + vrows);
	    }
	    else if (irows == vrows) {
		vxoff = (icols == vcols) ? BORDER_W :
		    BORDER_W + vnum * (BORDER_W + vcols);
		vyoff = BORDER_W;
	    }
	    else {		/* 4 views */
		/* assumes we want:
		   view1        view2
		   view3        view4   
		 */
		vxoff = vnum % 2 ? BORDER_W : vcols + 2 * BORDER_W;
		vyoff = vnum > 1 ? vrows + 2 * BORDER_W : BORDER_W;
	    }

	    name = vfiles[vnum][cnt];

	    G_message(_("Reading raster map <%s>..."), name);

	    fd = Rast_open_old(name, "");

	    if (Rast_read_colors(name, "", &colors) < 0)
		G_fatal_error(_("Unable to read color table for <%s>"), name);

	    rtype = Rast_get_map_type(fd);
	    voidc = Rast_allocate_buf(rtype);

	    for (row = 0; row < vrows; row++) {
		Rast_get_row(fd, voidc, (int)(row / vscale), rtype);

		rowoff = (vyoff + row) * ncols;
		Rast_lookup_colors(voidc, tr, tg, tb,
				       tset, tsiz, &colors, rtype);

		for (col = 0; col < vcols; col++) {
		    coff = (int)(col / vscale);
		    offset = rowoff + col + vxoff;

		    if (!tset[coff])
			pr[offset] = pg[offset] = pb[offset] = (char)255;
		    else {
			pr[offset] = (char)tr[coff];
			pg[offset] = (char)tg[coff];
			pb[offset] = (char)tb[coff];
		    }
		}
	    }

	    Rast_close(fd);
	}

	yfiles[cnt] = G_tempfile();

#ifdef USE_PPM
	write_ppm(pr, pg, pb, nrows, ncols, &y_rows, &y_cols, yfiles[cnt]);
#else
	write_ycc(pr, pg, pb, nrows, ncols, &y_rows, &y_cols, yfiles[cnt]);
#endif
    }

    mpfilename = G_tempfile();
    write_params(mpfilename, yfiles, outfile, cnt, quality, y_rows, y_cols, 0);

    if (G_verbose() <= G_verbose_min())
	ret = G_spawn(encoder, encoder, mpfilename,
		      SF_REDIRECT_FILE, SF_STDOUT, SF_MODE_OUT, G_DEV_NULL,
		      SF_REDIRECT_FILE, SF_STDERR, SF_MODE_OUT, G_DEV_NULL,
		      NULL);
    else
	ret = G_spawn(encoder, encoder, mpfilename, NULL);

    if (ret != 0)
	G_warning(_("mpeg_encode ERROR"));

    clean_files(mpfilename, yfiles, cnt);

    G_free(voidc);
    G_free(tset);
    G_free(tr);
    G_free(tg);
    G_free(tb);
    G_free(pr);
    G_free(pg);
    G_free(pb);

    return (cnt);
}

static int use_r_out(void)
{
    char *mpfilename;
    int ret;

    mpfilename = G_tempfile();
    write_params(mpfilename, vfiles[0], outfile, frames, quality, 0, 0, 1);

    if (G_verbose() <= G_verbose_min())
	ret = G_spawn(encoder, encoder, mpfilename,
		      SF_REDIRECT_FILE, SF_STDOUT, SF_MODE_OUT, G_DEV_NULL,
		      SF_REDIRECT_FILE, SF_STDERR, SF_MODE_OUT, G_DEV_NULL,
		      NULL);
    else
	ret = G_spawn(encoder, encoder, mpfilename, NULL);

    if (ret != 0)
	G_warning(_("mpeg_encode ERROR"));

    clean_files(mpfilename, NULL, 0);

    return (1);
}

/* ###################################################### */

static void mlist(const char *element, const char *wildarg, const char *outfile)
{
    int n;
    const char *mapset;

    for (n = 0; (mapset = G_get_mapset_name(n)); n++) {
	char type_arg[GNAME_MAX];
	char pattern_arg[GNAME_MAX];
	char mapset_arg[GMAPSET_MAX];

	if (strcmp(mapset, ".") == 0)
	    mapset = G_mapset();

	sprintf(type_arg, "type=%s", element);
	sprintf(pattern_arg, "pattern=%s", wildarg);
	sprintf(mapset_arg, "mapset=%s", mapset);

	G_spawn_ex("g.list", "g.list",
		   type_arg, pattern_arg, mapset_arg,
		   SF_REDIRECT_FILE, SF_STDOUT, SF_MODE_APPEND, outfile,
		   NULL);
    }
}

static char **parse(const char *filename, int *num)
{
    char buf[GNAME_MAX];
    char **files = NULL;
    int max_files = 0;
    int num_files = 0;
    FILE *fp;

    fp = fopen(filename, "r");
    if (!fp)
	G_fatal_error(_("Error reading wildcard"));

    while (fgets(buf, sizeof(buf), fp)) {
	char *p = strchr(buf, '\n');
	if (p)
	    *p = '\0';

	if (!*buf)
	    continue;

	if (num_files >= max_files) {
	    max_files += 50;
	    files = (char **) G_realloc((void *) files,
					max_files * sizeof(char *));
	}

	files[num_files++] = G_store(buf);
    }

    fclose(fp);

    *num = num_files;

    return files;
}

static char **gee_wildfiles(const char *wildarg, const char *element, int *num)
{
    char *tfile;
    char **files;

    tfile = G_tempfile();
    
    mlist(element, wildarg, tfile);
    files = parse(tfile, num);

    remove(tfile);
    G_free(tfile);

    return files;
}

/********************************************************************/
static void parse_command(struct Option **viewopts,
			  char *vfiles[MAXVIEWS][MAXIMAGES],
			  int *numviews, int *numframes)
{
    int i, j, k;

    *numviews = *numframes = 0;

    for (i = 0; i < MAXVIEWS; i++) {
	if (viewopts[i]->answers) {
	    int numi, wildnum;

	    (*numviews)++;

	    for (j = 0, numi = 0; viewopts[i]->answers[j]; j++) {
		if ((NULL != strchr(viewopts[i]->answers[j], '*')) ||
		    (NULL != strchr(viewopts[i]->answers[j], '?')) ||
		    (NULL != strchr(viewopts[i]->answers[j], '['))) {
		    char **wildfiles = gee_wildfiles(viewopts[i]->answers[j],
						     "rast", &wildnum);

		    for (k = 0; k < wildnum; k++)
			vfiles[i][numi++] = wildfiles[k];
		}
		else
		    vfiles[i][numi++] = G_store(viewopts[i]->answers[j]);
	    }

	    /* keep track of smallest number of frames */
	    *numframes =
		*numframes ? *numframes > numi ? numi : *numframes : numi;
	}
    }
}
