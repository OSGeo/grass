
/****************************************************************************
 *
 * MODULE:       xganim
 * AUTHOR(S):    Bill Brown <brown gis.uiuc.edu> CERL (original contributor),
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      a tool for animating a series of GRASS raster files
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>

#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/event.h>

extern "C" {
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/spawn.h>
#include <grass/glocale.h>
}

#include "gui.h"

#define DEF_MAX 900
#define DEF_MIN 600
#define BORDER_W    2

static char **gee_wildfiles(const char *wildarg, const char *element, int *num);
static void parse_command(
    struct Option **viewopts,
    char *vfiles[MAXVIEWS][MAXIMAGES],
    int *numviews, int *numframes);

struct Option *viewopts[MAXVIEWS];

unsigned int nrows, ncols;
char *vfiles[MAXVIEWS][MAXIMAGES];
int numviews;
int frames;
int Top = 0, Left = 0;
char frame[MAXIMAGES][4];
int LabelPos[MAXVIEWS][2];

float vscale, scale;		/* resampling scale factors */
int irows, icols, vrows, vcols;

BEGIN_EVENT_TABLE(MyApp, wxApp)
EVT_IDLE(MyApp::do_run)
END_EVENT_TABLE()

int main(int argc, char **argv)
{
    int i;

    G_gisinit(argv[0]);


    for (i = 0; i < MAXVIEWS; i++) {
	char buf[BUFSIZ];
	viewopts[i] = G_define_option();
	sprintf(buf, "view%d", i + 1);
	viewopts[i]->key = G_store(buf);
	viewopts[i]->type = TYPE_STRING;
	viewopts[i]->required = (i ? NO : YES);
	viewopts[i]->multiple = YES;
	viewopts[i]->gisprompt = "old,cell,Raster";;
	sprintf(buf, _("Raster file(s) for View%d"), i + 1);
	viewopts[i]->description = G_store(buf);
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parse_command(viewopts, vfiles, &numviews, &frames);

    return wxEntry(argc, argv);
}

bool MyApp::OnInit()
{
    int i, j;
    unsigned int *sdimp;
    int longdim;

    /* debug */
    if (G_verbose() > G_verbose_std()) {
	for (i = 0; i < numviews; i++) {
	    fprintf(stderr, "\nVIEW %d: ", i + 1);
	    for (j = 0; j < frames; j++) {
		fprintf(stderr, "%s ", vfiles[i][j]);
	    }
	}
    }
    fprintf(stderr, "\n");

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

    {				/* find animation image size */
	int max, min;
	char *p;

	max = DEF_MAX;
	min = DEF_MIN;

	if ((p = getenv("XGANIM_SIZE")))
	    max = min = atoi(p);

	if (longdim > max)	/* scale down */
	    scale = (float)max / longdim;
	else if (longdim < min)	/* scale up */
	    scale = (float)min / longdim;
    }

    vscale = scale;
    if (numviews == 4)
	vscale = scale / 2.;

    nrows = (unsigned int) (nrows * scale);
    ncols = (unsigned int) (ncols * scale);
    /* now nrows & ncols are the size of the combined - views image */
    vrows = (int) (vrows * vscale);
    vcols = (int) (vcols * vscale);
    /* now vrows & vcols are the size for each sub-image */

    /* add to nrows & ncols for borders */
    /* irows, icols used for vert/horizontal determination in loop below */
    irows = nrows;
    icols = ncols;
    nrows += (1 + (nrows / vrows)) * BORDER_W;
    ncols += (1 + (ncols / vcols)) * BORDER_W;

    gd.speed = 100;
    gd.direction = 1;
    gd.shownames = 1;

    mainwin = new MyFrame(wxString("GRASS Animate", wxConvISO8859_1), ncols, nrows, &gd);
    mainwin->Show();
    SetTopWindow(mainwin);

    for (j = 0; j < MAXIMAGES; j++)
	sprintf(frame[j], "%2d", j + 1);

    return true;
}


int MyApp::load_files(void)
{
    DCELL *dcell;
    unsigned char *tr, *tg, *tb, *tset;
    int tsiz, coff;
    int rowoff, row, col, vxoff, vyoff;
    int cnt, ret, fd;
    int vnum;
    const char *name;
    struct Colors colors;

    dcell = Rast_allocate_d_buf();

    tsiz = Rast_window_cols();

    /* allocate memory */
    tr   = (unsigned char *) G_malloc(tsiz * sizeof(char));
    tg   = (unsigned char *) G_malloc(tsiz * sizeof(char));
    tb   = (unsigned char *) G_malloc(tsiz * sizeof(char));
    tset = (unsigned char *) G_malloc(tsiz * sizeof(char));

    wxImage img(ncols, nrows);

    for (cnt = 0; cnt < frames; cnt++) {
	if (cnt > MAXIMAGES) {
	    cnt--;
	    break;
	}

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
		/* assumes we want :
		   view1        view2

		   view3        view4   
		 */
		vxoff = vnum % 2 ? BORDER_W : vcols + 2 * BORDER_W;
		vyoff = vnum > 1 ? vrows + 2 * BORDER_W : BORDER_W;
	    }
	    if (!cnt) {
		LabelPos[vnum][0] = vxoff;
		LabelPos[vnum][1] = vyoff + vrows - 1;
	    }

	    name = vfiles[vnum][cnt];
	    G_message(_("Reading file [%s]..."), name);

	    fd = Rast_open_old(name, "");
	    if (fd < 0)
		G_fatal_error(_("Unable to open raster map <%s>"), name);
	    /*
	       strcpy(title[cnt],G_get_cell_title(name, mapset));
	     */

	    ret = Rast_read_colors(name, "", &colors);
	    if (ret < 0)
		G_fatal_error(_("Unable to read color file"));

	    for (row = 0; row < vrows; row++) {
		Rast_get_d_row(fd, dcell, (int)(row / vscale));

		rowoff = (vyoff + row) * ncols;
		Rast_lookup_d_colors(dcell, tr, tg, tb, tset, tsiz, &colors);

		for (col = 0; col < vcols; col++) {
		    coff = (int)(col / vscale);

		    if (!tset[coff])
			img.SetRGB(vxoff + col, vyoff + row, 255, 255, 255);
		    else
			img.SetRGB(vxoff + col, vyoff + row, tr[coff], tg[coff], tb[coff]);
		}
	    }

	    Rast_close(fd);
	}

	wxBitmap *bmp = new wxBitmap(img);
	pic_array[cnt] = bmp;

	mainwin->canvas->draw_image(bmp);
	mainwin->change_label(frame[cnt]);
    }

    G_free(dcell);
    G_free(tr);
    G_free(tg);
    G_free(tb);
    G_free(tset);

    return cnt;
}


/* ###################################################### */

void MyApp::do_run(wxIdleEvent &ev)
{
    static int first = 1;
    struct gui_data *cd = &gd;
    int i, cnt;

    if (first) {
	first = 0;
	cnt = load_files();
	cd->curframe = cd->direction > 0 ? 0 : cnt - 1;
	cd->prevframe = cd->curframe;
	cd->step = cd->stop = 0;
	cd->loop = cd->swing = 0;
	cd->nframes = cnt;

    }

    if (cd->rewind) {
	cd->rewind = 0;
	cd->curframe = 0;
	cd->direction = 1;
	cd->step = 1;
    }

    if (cd->swing) {
	if (cd->curframe == cd->nframes || cd->curframe < 0) {
	    cd->direction = -cd->direction;
	    cd->curframe += cd->direction;
	}
    }
    else if (cd->loop) {
	if (cd->curframe == cd->nframes)
	    cd->curframe = 0;
	else if (cd->curframe < 0)
	    cd->curframe = cd->nframes - 1;
    }
    else if (cd->curframe == cd->nframes || cd->curframe < 0)
	cd->stop = 1;

    if (cd->stop && !cd->step)
	return;

    if (cd->curframe < cd->nframes && cd->curframe >= 0) {
	/* little pause */
	{
	    float tf;

	    for (tf = 0.0; tf < cd->speed; tf += .01) ;
	}

	mainwin->canvas->draw_image(pic_array[cd->curframe]);

	/* draw labels */
	for (i = 0; i < numviews; i++) {
	    mainwin->canvas->draw_text(
		cd->shownames,
		LabelPos[i][0] + 5, LabelPos[i][1] - 5,
		vfiles[i][cd->curframe]);
	}

	mainwin->change_label(frame[cd->curframe]);

	cd->prevframe = cd->curframe;
    }

    cd->curframe += cd->direction;

    if (cd->step) {
	cd->step = 0;
	cd->stop = 1;
    }
}

/* ###################################################### */

static void mlist(const char *element, const char *wildarg, const char *outfile)
{
    int n;
    const char *mapset;

    for (n = 0; (mapset = G__mapset_name(n)); n++) {
	char type_arg[GNAME_MAX];
	char pattern_arg[GNAME_MAX];
	char mapset_arg[GMAPSET_MAX];

	if (strcmp(mapset, ".") == 0)
	    mapset = G_mapset();

	sprintf(type_arg, "type=%s", element);
	sprintf(pattern_arg, "pattern=%s", wildarg);
	sprintf(mapset_arg, "mapset=%s", mapset);

	G_spawn_ex("g.mlist", "g.mlist",
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

/********************************************************************/

IMPLEMENT_APP_NO_MAIN(MyApp)

