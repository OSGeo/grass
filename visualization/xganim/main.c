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
#include <grass/gis.h>
#include <grass/glocale.h>
#include "gui.h"
#define MAIN
#include "local_proto.h"

#define COLOR_OFFSET 0
#define MAXIMAGES 400
#define DEF_MAX 900
#define DEF_MIN 600
#define MAXVIEWS    4 
#define BORDER_W    2


/* function prototypes */
static int load_files();
static Boolean do_run(XtPointer);
static char **gee_wildfiles(char *wildarg, char *element, int *num);
static void change_label(Widget wid, char *str);
static void parse_command(int argc, char **argv,
                  char *vfiles[MAXVIEWS][MAXIMAGES],
                  int *numviews, int *numframes);


/* global variables */
Widget 	     canvas, flabel; 
Display      *theDisplay;
XImage       *pic_array[MAXIMAGES];
GC           invertGC, drawGC;
unsigned int nrows, ncols;
int          numviews;
int          Top=0, Left=0;
char         frame[MAXIMAGES][4];
char         *vfiles[MAXVIEWS][MAXIMAGES];
int          LabelPos[MAXVIEWS][2];

float        vscale, scale;  /* resampling scale factors */
int          irows, icols, vrows, vcols;
int          frames;

unsigned int depth;


int main (int argc, char **argv)
{
    Widget        toplevel, mainwin, trc;
    int           scrn;
    Display       *dpy;
    Window        grwin;
    Colormap      fixedcmap;

    int	     	  i, j;
    unsigned int  *sdimp;
    int           longdim;
    unsigned long blackPix, whitePix;

    struct gui_data cd;

    XtAppContext  AppC;
    Arg           wargs[15];
    unsigned int  n;


    toplevel = XtAppInitialize(&AppC, "xganimate", NULL, 0,
			      &argc, argv, NULL, wargs, 0);

    theDisplay = XtDisplay(toplevel);
   

    G_gisinit (argv[0]);
    parse_command(argc, argv, vfiles, &numviews, &frames);

    /* debug */
    if (G_verbose() > G_verbose_std() ) {
	for(i=0; i<numviews; i++){
	    fprintf(stderr,"\nVIEW %d: ", i+1);
	    for(j=0; j< frames; j++){
		fprintf(stderr,"%s ", vfiles[i][j]);
	    }
	}
    }
    fprintf(stderr,"\n");

    vrows = G_window_rows();
    vcols = G_window_cols();
    nrows = vrows;
    ncols = vcols;

    /* short dimension */
    sdimp = nrows>ncols? &ncols: &nrows;

    /* these proportions should work fine for 1 or 4 views, but for
    2 views, want to double the narrow dim & for 3 views triple it */
    if(numviews == 2)
	*sdimp *= 2;
    else if(numviews == 3)
	*sdimp *= 3;

    longdim = nrows>ncols? nrows: ncols;

    scale = 1.0;

    { /* find animation image size */
    int max, min;
    char *p;

    max = DEF_MAX;
    min = DEF_MIN;

    if ((p = getenv ("XGANIM_SIZE")))
	max = min = atoi(p);

    if(longdim > max)      /* scale down */
	scale = (float)max/longdim;
    else if(longdim < min) /* scale up */
	scale = (float)min/longdim;
    }
    
    vscale = scale;
    if(numviews == 4)
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
    nrows += (1 + (nrows/vrows)) * BORDER_W;
    ncols += (1 + (ncols/vcols)) * BORDER_W;


    n = 0;
    if(ncols>nrows){
	XtSetArg(wargs[n], XmNwidth, ncols); n++;
	XtSetArg(wargs[n], XmNheight, nrows+60); n++;
    }
    else{
	XtSetArg(wargs[n], XmNwidth, ncols+80); n++;
	XtSetArg(wargs[n], XmNheight, nrows); n++;
    }
    mainwin = XtCreateManagedWidget("GRASS Animate", xmFormWidgetClass,
	    toplevel, wargs, n);

    cd.speed = 100;
    cd.direction = 1;
    cd.shownames = 1;

    n=0;
    XtSetArg(wargs[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(wargs[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(wargs[n],XmNwidth,ncols); n++;
    XtSetArg(wargs[n],XmNheight,nrows); n++;
    canvas = XtCreateManagedWidget("canvas", xmDrawingAreaWidgetClass,
		  mainwin, wargs, n);

    n=0;
    if(ncols > nrows){
	XtSetArg(wargs[n],XmNorientation,XmHORIZONTAL); n++;
	XtSetArg(wargs[n],XmNleftAttachment,XmATTACH_FORM); n++;
	XtSetArg(wargs[n],XmNrightAttachment,XmATTACH_FORM); n++;
	XtSetArg(wargs[n],XmNbottomAttachment,XmATTACH_FORM); n++;
	XtSetArg(wargs[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
	XtSetArg(wargs[n],XmNtopWidget,canvas); n++;
    }
    else{
	XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
	XtSetArg(wargs[n],XmNleftAttachment,XmATTACH_WIDGET); n++;
	XtSetArg(wargs[n],XmNleftWidget,canvas); n++;
	XtSetArg(wargs[n],XmNrightAttachment,XmATTACH_FORM); n++;
	XtSetArg(wargs[n],XmNbottomAttachment,XmATTACH_FORM); n++;
	XtSetArg(wargs[n],XmNtopAttachment,XmATTACH_FORM); n++;
    }
    XtSetArg(wargs[n],XmNbackground,WhitePixelOfScreen(XtScreen(toplevel)));n++;
    XtSetArg(wargs[n],XmNadjustMargin,False); n++;
    trc = XtCreateManagedWidget("controls_rc",
		xmRowColumnWidgetClass, mainwin, wargs, n);

    make_buttons(&cd, trc, XtScreen(toplevel));

    n = 0;
    XtSetArg(wargs[n],XmNalignment, XmALIGNMENT_END); n++;
    flabel = XtCreateManagedWidget("cfr", xmLabelWidgetClass, 
		trc, wargs,n);

    XtRealizeWidget(toplevel);
    set_buttons_pixmap(theDisplay, XtWindow(canvas));

    /**************************************************************/

    dpy = XtDisplay(canvas);
    grwin = XtWindow(canvas);
    scrn = DefaultScreen(dpy);
    use_visual = DefaultVisual(dpy, scrn);
#if 1
    fixedcmap = XCreateColormap(dpy, grwin,
				use_visual, AllocNone);
#else
    fixedcmap = DefaultColormap(dpy, scrn);
#endif
    fixedcmap = InitColorTableFixed(fixedcmap);

    XtVaGetValues(canvas, XmNdepth, &depth, NULL);

    XtVaSetValues(toplevel, XmNcolormap, fixedcmap, NULL);
    XtSetWMColormapWindows(toplevel, &canvas, 1);

    /**************************************************************/

    blackPix = _get_lookup_for_color(0, 0, 0);
    whitePix = _get_lookup_for_color(255, 255, 255);

    drawGC = XCreateGC(XtDisplay(canvas), XtWindow(canvas), (unsigned long)0, NULL);
    XSetFunction(theDisplay, drawGC, GXcopy);
    XSetForeground(theDisplay, drawGC, blackPix);
    XSetBackground(theDisplay, drawGC, whitePix);

    invertGC = XCreateGC(XtDisplay(canvas), XtWindow(canvas), (unsigned long)0, NULL);
    XSetFunction(theDisplay, invertGC, GXcopy);
    XSetForeground(theDisplay, invertGC, whitePix);
    XSetBackground(theDisplay, invertGC, blackPix);


    for(j=0; j<MAXIMAGES; j++)
	sprintf(frame[j],"%2d",j+1);
   
    while(1) { /* wait for window */
	XEvent      xev;

	XNextEvent(theDisplay, &xev);
	if(xev.type == MapNotify && xev.xmap.event == XtWindow(mainwin))
	    break;
    }

    XtAppAddWorkProc(AppC, do_run, &cd);
    XtAppMainLoop(AppC);

    return 0;
}


static int load_files()
{
    CELL  *cell;
    FCELL *fcell;
    DCELL *dcell;
    void  *voidc = NULL;
    unsigned char *tr, *tg, *tb, *tset;
    int tsiz, coff;
    int rowoff, row, col, vxoff, vyoff;
    int cnt, ret, fd;
    int	vnum;
    XImage  *pa;
    char *mapset, name[BUFSIZ];
    struct Colors colors;
    int rtype;

    cell = G_allocate_c_raster_buf();
    fcell = G_allocate_f_raster_buf();
    dcell = G_allocate_d_raster_buf();
     
    tsiz = G_window_cols();

    /* allocate memory */
    tr = G_malloc(tsiz * sizeof(char));
    tg = G_malloc(tsiz * sizeof(char));
    tb = G_malloc(tsiz * sizeof(char));
    tset = G_malloc(tsiz * sizeof(char));

    for (cnt = 0; cnt < frames; cnt++)
    {
        if (cnt > MAXIMAGES)
	{
	    cnt--;
	    break;
	}

	pa = XCreateImage(theDisplay, use_visual, depth, ZPixmap,
			  0, NULL,  ncols, nrows, 8, 0);
	pa->data = G_malloc((size_t)nrows * pa->bytes_per_line);
	pic_array[cnt] = pa;

	for(vnum = 0; vnum < numviews; vnum++){
	    if(icols == vcols){
		vxoff =  BORDER_W;
		vyoff = (irows == vrows)? BORDER_W : 
			    BORDER_W + vnum*(BORDER_W+vrows);
	    }
	    else if (irows == vrows){
		vxoff = (icols == vcols)? BORDER_W : 
			    BORDER_W + vnum*(BORDER_W+vcols);
		vyoff =  BORDER_W;
	    }
	    else{ /* 4 views */
		/* assumes we want :
		    view1	view2

		    view3	view4   
		*/
		vxoff = vnum%2? BORDER_W: vcols+2*BORDER_W;
		vyoff = vnum>1? vrows+2*BORDER_W: BORDER_W; 
	    }
	    if(!cnt){
		LabelPos[vnum][0] = vxoff;
		LabelPos[vnum][1] = vyoff+vrows-1;
	    }

	    strcpy(name,vfiles[vnum][cnt]);
	    G_message(_("Reading file [%s]..."), name);

	    mapset = G_find_cell2 (name, "");
	    if (mapset == NULL)
                G_fatal_error(_("Raster map <%s> not found"), name);

	    fd = G_open_cell_old (name, mapset);
	    if (fd < 0)
                G_fatal_error(_("Unable to open raster map <%s>"), name);
    /*
	    strcpy(title[cnt],G_get_cell_title(name, mapset));
    */

	    rtype = G_get_raster_map_type(fd);
	    if (rtype == CELL_TYPE)
		voidc = (CELL *)cell;
	    else if (rtype == FCELL_TYPE)
		voidc = (FCELL *)fcell;
	    else if (rtype == DCELL_TYPE)
		voidc = (DCELL *)dcell;
	    else
                /* should not reach here */
                G_fatal_error(_("Unable to determine raster cell type"));

	    ret = G_read_colors(name, mapset, &colors);
	    if (ret < 0)
                G_fatal_error(_("Unable to read color file"));

	    for (row = 0; row < vrows; row++){
		if (G_get_raster_row (fd, (void *)voidc, (int)(row/vscale), rtype) < 0)
                    G_fatal_error(_("Unable to read raster row"));

		rowoff = (vyoff+row)*ncols;
		G_lookup_raster_colors((void *)voidc, tr, tg, tb, tset, tsiz,
		    &colors, rtype);

		for (col = 0; col < vcols; col++){
		    coff= (int)(col/vscale);

		    if(!tset[coff])
			tr[coff] = tg[coff] = tb[coff] = 255;

		    XPutPixel(pa, vxoff+col, vyoff+row,
			      _get_lookup_for_color(tr[coff],
						    tg[coff], tb[coff]));
		}
	    }

	    G_close_cell(fd);
	}

	XPutImage(theDisplay, XtWindow(canvas), drawGC, pa, 0, 0, 
		  Left, Top, ncols, nrows);
	change_label(flabel, frame[cnt]);

    }
    G_free(cell);
    G_free(fcell);
    G_free(dcell);
    G_free(tr);
    G_free(tg);
    G_free(tb);
    G_free(tset);

    return(cnt);
}


/* ###################################################### */
static Boolean do_run(XtPointer p)
{
    static int first = 1;
    struct gui_data *cd = p;
    int i, cnt;
    Drawable dr;

    if(first){
	first=0;
	cnt = load_files();
	cd->curframe = cd->direction > 0? 0: cnt-1;
	cd->prevframe = cd->curframe;
	cd->step = cd->stop = 0;
	cd->loop = cd->swing = 0;
	cd->nframes = cnt;

    }

    if(cd->rewind){
	cd->rewind = 0;
	cd->curframe = 0;
	cd->direction = 1;
	cd->step = 1;
    }

    if(cd->swing){
	if(cd->curframe==cd->nframes || cd->curframe<0){
	     cd->direction = -cd->direction;
	     cd->curframe += cd->direction;
	}
    }
    else if(cd->loop){
	if(cd->curframe==cd->nframes)
	     cd->curframe = 0;
	else if(cd->curframe<0)
	     cd->curframe = cd->nframes-1;
    }
    else if(cd->curframe == cd->nframes || cd->curframe < 0)
	cd->stop = 1;

    if(cd->stop && !cd->step)
	return (False);

    if(cd->curframe < cd->nframes && cd->curframe >= 0){
	/* little pause */
	{
	float tf;
	for(tf=0.0; tf < cd->speed; tf += .01);
	}

	dr = XtWindow(canvas);
	XPutImage(theDisplay, dr, drawGC, pic_array[cd->curframe], 0, 0, 
		    Left, Top, ncols, nrows);

	/* draw labels */
	if(cd->shownames == 1)
	    for(i=0; i < numviews; i++){
		XDrawString(theDisplay, dr, drawGC,
			LabelPos[i][0]+5, LabelPos[i][1]-5,
			vfiles[i][cd->curframe],
			(int)strlen(vfiles[i][cd->curframe]));
	    }
	else if(cd->shownames == 2)
	    for(i=0; i < numviews; i++){
		XDrawString(theDisplay, dr, invertGC,
			LabelPos[i][0]+5, LabelPos[i][1]-5,
			vfiles[i][cd->curframe],
			(int)strlen(vfiles[i][cd->curframe]));
	    }
	change_label(flabel, frame[cd->curframe]);

	cd->prevframe = cd->curframe;
    }

    cd->curframe += cd->direction;

    if (cd->step){
	cd->step = 0;
	cd->stop = 1;
    }

    return False; /* to keep it running */
}


/* ###################################################### */
static char **gee_wildfiles(char *wildarg, char *element, int *num)
{
    int n, cnt=0;
    char path[1000], *mapset, cmd[1000], buf[512];
    char *p, *tfile;
    static char *newfiles[MAXIMAGES];
    FILE *tf;

    *num = 0;
    tfile = G_tempfile();

    /* build list of filenames */
    for(n=0; (mapset = G__mapset_name (n)); n++){
	if (strcmp (mapset,".") == 0)
	    mapset = G_mapset();

	G__file_name (path, element, "", mapset);
	if(access(path, 0) == 0) {
	    sprintf(cmd, "cd %s; \\ls %s >> %s 2> /dev/null", path, wildarg, tfile);
	    system(cmd);
	}
    }

    if (NULL == (tf = fopen(tfile, "r")))
        G_warning(_("Error reading wildcard"));
    else{
	while(NULL != fgets(buf,512,tf)){
	    /* replace newline with null */
	    if ((p = strchr(buf, '\n')))
		*p = '\0';
	    /* replace first space with null */
	    else if ((p = strchr(buf, ' ')))
		*p = '\0';

	    if(strlen(buf) > 1){
		newfiles[cnt++] = G_store (buf);
	    }
	}
	fclose(tf);
    }
    *num = cnt;
    G_free(tfile);

    return(newfiles);
}


/********************************************************************/
/* to change label in label widget */

static void change_label(Widget wid, char *str)
{
    Arg wargs[1];
    XmString xmstr;

    xmstr = XmStringCreateSimple(str);
    XtSetArg (wargs[0], XmNlabelString,  xmstr);
    XtSetValues (wid, wargs, 1);
    XmStringFree (xmstr);
}


/********************************************************************/
static void parse_command(int argc, char **argv,
                  char *vfiles[MAXVIEWS][MAXIMAGES],
                  int *numviews, int *numframes)
{
    struct Option *viewopts[MAXVIEWS]; 
    char buf[BUFSIZ], **wildfiles;
    int i,j,k, numi, wildnum;

    *numviews = *numframes = 0;
    for(i=0; i<MAXVIEWS; i++){
	viewopts[i] = G_define_option();
	sprintf(buf,"view%d", i+1);
	viewopts[i]->key		= G_store(buf);
	viewopts[i]->type 		= TYPE_STRING;
	viewopts[i]->required 		= (i? NO: YES);
	viewopts[i]->multiple 		= YES;
	viewopts[i]->gisprompt 		= "old,cell,Raster";;
	sprintf(buf, _("Raster file(s) for View%d"), i+1);
	viewopts[i]->description 	= G_store(buf);
    }

    if (G_parser (argc, argv))
	    exit(EXIT_FAILURE);

    for(i=0; i<MAXVIEWS; i++){
	if(viewopts[i]->answers){
	    (*numviews)++;

	    for (j = 0, numi=0 ; viewopts[i]->answers[j] ; j++){
		if((NULL != strchr(viewopts[i]->answers[j], '*')) || 
		   (NULL != strchr(viewopts[i]->answers[j], '?')) || 
		   (NULL != strchr(viewopts[i]->answers[j], '['))){
		    wildfiles = gee_wildfiles(viewopts[i]->answers[j],
				"cell", &wildnum);

		    for (k = 0; k < wildnum; k++)
			vfiles[i][numi++] = wildfiles[k];
		}
		else
		    vfiles[i][numi++] = G_store(viewopts[i]->answers[j]);
	    }
	    /* keep track of smallest number of frames */
	    *numframes = *numframes? *numframes > numi? numi: *numframes: numi;
	}
    }
}



