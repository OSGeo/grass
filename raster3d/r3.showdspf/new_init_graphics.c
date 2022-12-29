
/*--------------------------------------------------------------------
 new_init_graphics.c

 Programmed by: Kenneth Sakai
 Company      : Lockheed Martin
 Date         : 3-6-00

 Note: The following code is not meant necessarily to be commercial grade
       code.  It is just meant to be better than the original grad-student version.
       K.S.
  --------------------------------------------------------------------
 */
#include <stdio.h>
#include "vizual.h"

#include <grass/config.h>

#include <Xm/Xm.h>
#include <Xm/Form.h>
      /* include file for GL drawing widget */
#ifdef HAVE_GL_GLWMDRAWA_H
#include <GL/GLwMDrawA.h>
#else
#ifdef HAVE_X11_GLW_GLWMDRAWA_H
#include <X11/GLw/GLwMDrawA.h>
#endif
#endif
#include "Ball.h"

BallData Trackball;

#include "kns_defines.h"
#include "kns_globals.h"

#include <X11/Xlib.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define MAXTHRESHOLDS 200
static Widget Threshbutton[MAXTHRESHOLDS];
static Widget Button_plus;
static Widget Button_minus;
static Widget SingleToggle, MultipleToggle;
static Widget ThresholdRadio;
static Widget PlotSelected;
static int MultipleThresholdFlag;
static int ProceedStatus;
static GLuint FontBase;
static int Attributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, GLX_DOUBLEBUFFER, None
};
static int SingleAttributes[] = { GLX_RGBA, GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, GLX_DOUBLEBUFFER, None
};
static int RotationEnabled;

void glinitCB(Widget widget, XtPointer client_data, XtPointer cdata);
void glexposeCB(Widget widget, XtPointer client_data, XtPointer cdata);
void glinputCB(Widget widget, XtPointer client_data, XtPointer cdata);
void glresizeCB(Widget widget, XtPointer client_data, XtPointer cdata);
void glexpose2CB(Widget widget, XtPointer client_data, XtPointer cdata);
void glresize2CB(Widget widget, XtPointer client_data, XtPointer cdata);
void ThresholdCB(Widget widget, XtPointer client_data, XtPointer call_data);
void PlusMinusCB(Widget widget, XtPointer client_data, XtPointer call_data);
void SingleMultipleCB(Widget widget, XtPointer client_data, XtPointer cdata);
void PlotSelectedCB(Widget widget, XtPointer client_data, XtPointer cdata);

void set_threshold_button(int iset);
void draw_ctable();
void rotateLoop();
void draw_multiple();

/*-------------------------------------------------------------------
  get_trackball_rotation_matrix()
  This functions retrieves the current trackball rotation matrix.
  -------------------------------------------------------------------
*/
void get_trackball_rotation_matrix(float mat[4][4])
{
    int i, j;
    HMatrix mNow;

    Ball_Value(&Trackball, mNow);
    for (i = 0; i < QuatLen; i++)
	for (j = 0; j < QuatLen; j++)
	    mat[i][j] = mNow[i][j];
}

/*-------------------------------------------------------------------
  This functions set the trackball state based upon the set of rotation
  angles contained in the dspect struct.
  -------------------------------------------------------------------
*/
void set_trackball_rotations(struct dspec *D_spec)
{
    GLfloat tranmat[4][4];

    glPushMatrix();
    glLoadIdentity();
    glRotatef(.1 * (D_spec->yrot * 10), 0., 1., 0.);
    glRotatef(.1 * (D_spec->zrot * 10), 0., 0., 1.);
    glRotatef(.1 * (D_spec->xrot * 10), 1., 0., 0.);
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) tranmat);
    Ball_SetMatrix(&Trackball, tranmat);
    glPopMatrix();
}

/*-------------------------------------------------------------------
  new_swapbuffers()
  This is an OpenGL version of the IRISGL swapbuffers() function.
  -------------------------------------------------------------------
*/
void new_swapbuffers()
{
    glXSwapBuffers(XtDisplay(MainOGLWindow.widget), MainOGLWindow.window);
}

/*-------------------------------------------------------------------
  enableRotation()
  -------------------------------------------------------------------
*/
void enableRotation()
{
    RotationEnabled = 1;
}

/*-------------------------------------------------------------------
  disableRotation()
  -------------------------------------------------------------------
*/
void disableRotation()
{
    RotationEnabled = 0;
}

/*-------------------------------------------------------------------
  new_swapbuffers()
  This is a substitute for the IRISGL getsize() function.
  -------------------------------------------------------------------
*/
void new_getsize(long *x, long *y)
{
    *x = MainOGLWindow.width;
    *y = MainOGLWindow.height;
}


/*-------------------------------------------------------------------
  initializeFonts()
  This function initializes the OpenGL display lists required for drawing 
  2D text in a 3D window.  FontBase will end up pointing to a sequence
  of display lists in which each individual display character in a font.
  The font chosen in a Courier Bold font chosen to match the font used 
  by the IRISGL charstr() function.
  -------------------------------------------------------------------
*/
void initializeFonts()
{
    XFontStruct *fontInfo;
    Font id;
    GLuint base;
    unsigned int first, last;

    fontInfo = XLoadQueryFont(XtDisplay(MainOGLWindow.widget),
			      "-adobe-courier-bold-r-normal--14-100-100-100-m-90-iso8859-1");
    id = fontInfo->fid;
    first = fontInfo->min_char_or_byte2;
    last = fontInfo->max_char_or_byte2;
    base = glGenLists((GLuint) last + 1);
    glXUseXFont(id, first, last - first + 1, base + first);
    XFreeFont(XtDisplay(MainOGLWindow.widget), fontInfo);
    FontBase = base;
}

/*-------------------------------------------------------------------
  new_charstr()
  This is an OpenGL version of the IRISGL charstr() function. The initializeFonts
  function must have been called prior to calling this function.
  -------------------------------------------------------------------
*/
void new_charstr(char *str)
{
    glPushAttrib(GL_LIST_BIT);
    glListBase(FontBase);
    glCallLists(strlen(str), GL_UNSIGNED_BYTE, (GLubyte *) str);
    glPopAttrib();
}

/*-------------------------------------------------------------------
  winset_main()
  This function is the equivalent of the IRISGL winset command for 
  the main 3D drawing window.
  -------------------------------------------------------------------
*/
void winset_main()
{
    glXMakeCurrent(XtDisplay(MainOGLWindow.widget), MainOGLWindow.window,
		   MainOGLWindow.glx_context);
}

/*-------------------------------------------------------------------
  loadrect()
  This function loads a file created via the loadrect() function into the
  framebuffer ff the main 3D drawing window
  -------------------------------------------------------------------
*/
int loadrect(char *name)
{
    FILE *fp;
    unsigned long *buffer;

    float xd, yd, zd, trd;
    float aspect;
    int xsiz, ysiz;
    long xsiz2, ysiz2;

    new_getsize(&xsiz2, &ysiz2);

    winset_main();
    fp = fopen(name, "r");
    if (!fp) {
	fprintf(stderr, "Unable to open file <%s>\n", name);
	G_free(buffer);
	return 0;
    }
    fread(&xsiz, sizeof(int), 1, fp);
    fread(&ysiz, sizeof(int), 1, fp);
    if (NULL ==
	(buffer = (unsigned long *)G_malloc(xsiz * ysiz * sizeof(long)))) {
	fprintf(stderr, "Out of memory\n");
	return -1;
    }

    if (ysiz != fread(buffer, xsiz * sizeof(long), ysiz, fp))
	fprintf(stderr, "Unable to write file <%s>\n", name);

    clear_screen();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, xsiz2, 0, ysiz2, -100., 10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRasterPos2i(0, 0);
    glDrawPixels(xsiz, ysiz, GL_RGBA, GL_BYTE, buffer);

    glXSwapBuffers(XtDisplay(MainOGLWindow.widget), MainOGLWindow.window);
    fclose(fp);
    G_free(buffer);


    aspect = (float)MainOGLWindow.width / (float)MainOGLWindow.height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45., aspect, .10, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    xd = Headfax.xdim * MainOGLWindow.ptr_D_spec->xscale;
    yd = Headfax.ydim * MainOGLWindow.ptr_D_spec->yscale;
    zd = Headfax.zdim * MainOGLWindow.ptr_D_spec->zscale;

    /* pick greatest dimension to use for translation of viewer from origin */
    if (xd < yd)
	trd = yd;
    else
	trd = xd;
    if (trd < zd)
	trd = zd;

    glTranslatef(0.0, 0.0, -trd * 1.6);




    return (0);
}

/*-------------------------------------------------------------------
  dumprect()
  This function dumps the framebuffer from the main 3D drawing window
  into a file.
  -------------------------------------------------------------------
*/
int dumprect(char *name)
{
    FILE *fp;
    long xsiz, ysiz;
    unsigned long *buffer;

    new_getsize(&xsiz, &ysiz);
    if (NULL ==
	(buffer = (unsigned long *)G_malloc(xsiz * ysiz * sizeof(long)))) {
	fprintf(stderr, "Out of memory\n");
	return -1;
    }
    winset_main();


    fp = fopen(name, "w");
    if (!fp) {
	fprintf(stderr, "Unable to open file <%s>\n", name);
	G_free(buffer);
	return 0;
    }

    /* write dims */
    fwrite(&xsiz, sizeof(int), 1, fp);
    fwrite(&ysiz, sizeof(int), 1, fp);

    glReadBuffer(GL_FRONT);

    glReadPixels(0, 0, xsiz, ysiz, GL_RGBA, GL_BYTE, buffer);
    if (ysiz != fwrite(buffer, xsiz * sizeof(long), ysiz, fp))
	fprintf(stderr, "Unable to write file <%s>\n", name);

    fclose(fp);
    G_free(buffer);


    return (0);
}

/*-------------------------------------------------------------------
  dumpgif()
  This function dumps the framebuffer from the main 3D drawing window
  into a GIF file.
  -------------------------------------------------------------------
*/
#define CPACKTORGBA(l,r,g,b,a)			\
   (r) = ((l)>>0) & 0xff;			\
   (g) = ((l)>>8) & 0xff;			\
   (b) = ((l)>>16) & 0xff;			\
(a) = ((l)>>24) & 0xff;

#include "togif.h"
int dumpgif(char *name)
{
    FILE *fp;
    long xwid, ywid;
    unsigned long *pixels;
    long numpixels;
    vgl_GIFWriter *writer;
    int i;
    unsigned short red, green, blue, alpha;

    new_getsize(&xwid, &ywid);
    if (xwid % 2)
	xwid--;
    if (ywid % 2)
	ywid--;



    if (NULL ==
	(pixels = (unsigned long *)G_malloc(xwid * ywid * sizeof(long)))) {
	fprintf(stderr, "Out of memory\n");
	return -1;
    }

    winset_main();


    fp = fopen(name, "wb");
    if (!fp) {
	fprintf(stderr, "Unable to open file <%s>\n", name);
	G_free(pixels);
	return 0;
    }


    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, xwid, ywid, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    numpixels = xwid * ywid;
    for (i = 0; i < numpixels; i++) {
	CPACKTORGBA(pixels[i], red, green, blue, alpha);
#ifdef WIN32
	pixels[i] = red | (green << 8) | (blue << 16) | (alpha << 24);
#else
	pixels[i] = alpha | (blue << 8) | (green << 16) | (red << 24);
#endif
    }


    writer = vgl_GIFWriterBegin();
    vgl_GIFWriterWriteGIFFile(writer, pixels, xwid, ywid, 0, fp);


    fclose(fp);
    G_free(pixels);


    return (0);
}


/*-------------------------------------------------------------------
  winset_colortable()
  This function is the equivalent of the IRISGL winset command for 
  the colormap window.
  -------------------------------------------------------------------
*/
void winset_colortable()
{
    glXMakeCurrent(XtDisplay(ColormapWindow.widget), ColormapWindow.window,
		   ColormapWindow.glx_context);
}

/*-------------------------------------------------------------------
  init_graphics()
  This function creates both the main 3D drawing window and the 2D
  colormap window.
  -------------------------------------------------------------------
*/
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/ToggleB.h>
#include <Xm/Frame.h>
#include <Xm/SeparatoG.h>
#include <grass/gis.h>
static String Fallback_resources[] = {
    "*.fontList:-adobe-helvetica-bold-r-normal--12-120-75-75-p-70-iso8859-1",
    "*Thresholds.fontList:-adobe-helvetica-bold-r-normal--17-120-100-100-p-92-iso8859-1",
};

void init_graphics(char *name, int argc, char **argv, struct dspec *dspecptr)
{
    int n;
    Arg args[20];		/* array used to set widget resources */
    XVisualInfo *vi;
    XEvent event;
    Widget toplevel, form;
    int i;
    char buffer[20];
    Widget separator;
    Widget gl_shell;
    Widget buttonrowcol, label1, radio;
    Widget frame;

    MainOGLWindow.ptr_D_spec = dspecptr;

    n = 0;
    XtSetArg(args[n], XmNwidth, 660);
    n++;
    XtSetArg(args[n], XmNheight, 600);
    n++;
    XtSetArg(args[n], XmNx, 10);
    n++;
    XtSetArg(args[n], XmNy, 100);
    n++;
    toplevel = XtAppInitialize(&App_context, "Showdspf", NULL,
			       0, &argc, argv, Fallback_resources, args, n);
    /* 
     *     Create the 3D drawing window
     */
    n = 0;
    XtSetArg(args[n], XmNwidth, 660);
    n++;
    XtSetArg(args[n], XmNheight, 600);
    n++;
    XtSetArg(args[n], XmNx, 10);
    n++;
    XtSetArg(args[n], XmNy, 100);
    n++;
    form = XmCreateForm(toplevel, "form", args, n);
    XtManageChild(form);
    vi = glXChooseVisual(XtDisplay(form), DefaultScreen(XtDisplay(form)),
			 Attributes);
    if (!vi) {
	fprintf(stderr, "Fatal error: Couldn't get a visual\n");
	fprintf(stderr, "See program developer.\n");
	exit(1);
    }

    n = 0;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);
    n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);
    n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);
    n++;
    XtSetArg(args[n], XmNshadowThickness, 3);
    n++;
    XtSetArg(args[n], XmNshadowType, XmSHADOW_OUT);
    n++;
    frame = XmCreateFrame(form, "frame_r", args, n);
    XtManageChild(frame);

    n = 0;
    XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_CENTER);
    n++;
    XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);
    n++;
    XtSetArg(args[n], XmNorientation, XmVERTICAL);
    n++;
    XtSetArg(args[n], XmNnumColumns, 1);
    n++;
    XtSetArg(args[n], XmNmarginHeight, 0);
    n++;
    XtSetArg(args[n], XmNmarginWidth, 0);
    n++;
    buttonrowcol = XmCreateRowColumn(frame, "rowcol", args, n);
    XtManageChild(buttonrowcol);
    label1 = XmCreateLabel(buttonrowcol, "Thresholds", args, n);
    XtManageChild(label1);

    n = 0;
    XtSetArg(args[n], XmNshadowThickness, 5);
    n++;
    separator = XmCreateSeparatorGadget(buttonrowcol, "separator1", args, n);

    XtManageChild(separator);
    n = 0;
    radio = XmCreateRadioBox(buttonrowcol, "radio", args, n);
    XtManageChild(radio);
    n = 0;
    SingleToggle =
	XtVaCreateManagedWidget("Single", xmToggleButtonWidgetClass, radio,
				NULL);
    XtAddCallback(SingleToggle, XmNarmCallback, SingleMultipleCB,
		  (XtPointer) 1);
    MultipleToggle =
	XtVaCreateManagedWidget("Multiple", xmToggleButtonWidgetClass, radio,
				NULL);
    XtAddCallback(MultipleToggle, XmNarmCallback, SingleMultipleCB,
		  (XtPointer) 0);
    XmToggleButtonSetState(SingleToggle, True, False);

    n = 0;
    XtSetArg(args[n], XmNshadowThickness, 5);
    n++;
    separator = XmCreateSeparatorGadget(buttonrowcol, "separator1", args, n);
    XtManageChild(separator);


    n = 0;
    Button_plus =
	XtCreateManagedWidget("Increase", xmPushButtonWidgetClass,
			      buttonrowcol, args, n);
    XtAddCallback(Button_plus, XmNactivateCallback, PlusMinusCB,
		  (XtPointer) 1);
    Button_minus =
	XtCreateManagedWidget("Decrease", xmPushButtonWidgetClass,
			      buttonrowcol, args, n);
    XtAddCallback(Button_minus, XmNactivateCallback, PlusMinusCB,
		  (XtPointer) 0);

    n = 0;
    PlotSelected = XmCreatePushButton(buttonrowcol, "Plot Selected", args, n);
    XtAddCallback(PlotSelected, XmNactivateCallback, PlotSelectedCB, NULL);


    n = 0;
    ThresholdRadio = XmCreateRadioBox(buttonrowcol, "radio", args, n);
    XtManageChild(ThresholdRadio);
    if (Headfax.linefax.nthres) {
	for (i = 1; i <= Headfax.linefax.nthres; i++) {
	    n = 0;
	    if (i <= MAXTHRESHOLDS) {
		sprintf(buffer, "Threshold %d", i);
		Threshbutton[i - 1] =
		    XtVaCreateManagedWidget(buffer, xmToggleButtonWidgetClass,
					    ThresholdRadio, NULL);
		XtAddCallback(Threshbutton[i - 1], XmNarmCallback,
			      ThresholdCB, (XtPointer) (i - 1));
	    }
	}
	XmToggleButtonSetState(Threshbutton[0], True, False);
    }


    n = 0;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);
    n++;
    XtSetArg(args[n], XmNrightWidget, frame);
    n++;
    XtSetArg(args[n], GLwNvisualInfo, vi);
    n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);
    n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);
    n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);
    n++;
    MainOGLWindow.widget = GLwCreateMDrawingArea(form, "glwidget", args, n);

    XtManageChild(MainOGLWindow.widget);

    XtAddCallback(MainOGLWindow.widget, GLwNginitCallback, glinitCB, 0);

    XtAddCallback(MainOGLWindow.widget, GLwNexposeCallback, glexposeCB, 0);
    XtAddCallback(MainOGLWindow.widget, GLwNinputCallback, glinputCB, 0);
    XtAddCallback(MainOGLWindow.widget, GLwNresizeCallback, glresizeCB, 0);


    /* 
     *     Create the colormap window
     */
    n = 0;
    XtSetArg(args[n], XmNwidth, 100);
    n++;
    XtSetArg(args[n], XmNheight, 500);
    n++;
    XtSetArg(args[n], XmNx, 5);
    n++;
    XtSetArg(args[n], XmNy, 5);
    n++;
    gl_shell = XtAppCreateShell("Colormap", "GL_window",
				topLevelShellWidgetClass, XtDisplay(toplevel),
				args, n);

    n = 0;
    XtSetArg(args[n], XmNwidth, 100);
    n++;
    XtSetArg(args[n], XmNheight, 500);
    n++;
    XtSetArg(args[n], XmNx, 5);
    n++;
    XtSetArg(args[n], XmNy, 5);
    n++;
    form = XmCreateForm(gl_shell, "form", args, n);
    XtManageChild(form);
    vi = glXChooseVisual(XtDisplay(form), DefaultScreen(XtDisplay(form)),
			 SingleAttributes);
    if (!vi) {
	fprintf(stderr, "Fatal error: Couldn't get a visual\n");
	fprintf(stderr, "See program developer.\n");
	exit(1);
    }



    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);
    n++;
    XtSetArg(args[n], GLwNvisualInfo, vi);
    n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);
    n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);
    n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);
    n++;
    ColormapWindow.widget = GLwCreateMDrawingArea(form, "glwidget", args, n);

    XtManageChild(ColormapWindow.widget);

    XtAddCallback(ColormapWindow.widget, GLwNginitCallback, glinitCB,
		  (void *)1);

    XtAddCallback(ColormapWindow.widget, GLwNexposeCallback, glexpose2CB, 0);

    XtAddCallback(ColormapWindow.widget, GLwNresizeCallback, glresize2CB, 0);


    XtRealizeWidget(toplevel);
    XStoreName(XtDisplay(toplevel), XtWindow(toplevel), name);

    XtPopup(gl_shell, XtGrabNone);

    XMapRaised(XtDisplay(gl_shell), XtWindow(gl_shell));


    while (ProceedStatus < 4) {
	XtAppNextEvent(App_context, &event);
	XtDispatchEvent(&event);
    }

}

/*-------------------------------------------------------------------
  ThresholdCB()
  This is a Motif callback that is called when one of the threshold toggle
  buttons is hit.
  -------------------------------------------------------------------
*/
void ThresholdCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    int num = (int)client_data;

    if (MultipleThresholdFlag)
	return;
    MainOGLWindow.ptr_D_spec->Thresh = num;
    do_draw_immediate_mode();
}

/*-------------------------------------------------------------------
  PlusMinusCB()
  This is a Motif callback that is called when either the "Increase" or
  "Decrease" button is hit in the GUI interface.
  -------------------------------------------------------------------
*/
void PlusMinusCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    int flag = (int)client_data;

    if (flag) {
	MainOGLWindow.ptr_D_spec->Thresh++;
	if (MainOGLWindow.ptr_D_spec->Thresh > Headfax.linefax.nthres - 1)
	    MainOGLWindow.ptr_D_spec->Thresh = 0;
	set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh + 1);
	do_draw_immediate_mode();

    }
    else {
	MainOGLWindow.ptr_D_spec->Thresh--;
	if (MainOGLWindow.ptr_D_spec->Thresh < 0)
	    MainOGLWindow.ptr_D_spec->Thresh = Headfax.linefax.nthres - 1;
	set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh + 1);
	do_draw_immediate_mode();
    }
}

/*-------------------------------------------------------------------
  set_threshold_button(int iset)
  This function sets the GUI toggle button corresponding to a threshold value
  -------------------------------------------------------------------
*/
void set_threshold_button(int iset)
{
    int i;

    for (i = 1; i <= Headfax.linefax.nthres; i++) {
	if (i == iset) {
	    if (i <= MAXTHRESHOLDS)
		XmToggleButtonSetState(Threshbutton[i - 1], True, False);
	}
	else {
	    if ((i <= MAXTHRESHOLDS) && !MultipleThresholdFlag)
		XmToggleButtonSetState(Threshbutton[i - 1], False, False);
	}
    }
}

/*-------------------------------------------------------------------
  unset_threshold_button(int iset)
  This function unsets the GUI toggle button corresponding to a threshold value
  -------------------------------------------------------------------
*/
void unset_threshold_button(int iset)
{
    if (iset <= MAXTHRESHOLDS)
	XmToggleButtonSetState(Threshbutton[iset - 1], False, False);
}

/*-------------------------------------------------------------------
  rotateLoop()
  This function is executed while the user attempts to rotate/translate
  the 3D model in the main 3D drawing window
  -------------------------------------------------------------------
*/

void rotateLoop()
{
    XEvent event;
    XKeyPressedEvent *K;
    KeySym sym;
    int thresh = 1;

    for (;;) {
	XtAppNextEvent(App_context, &event);

	if (event.type == KeyPress) {
	    K = (XKeyPressedEvent *) & event;
	    sym =
		XKeycodeToKeysym(XtDisplay(MainOGLWindow.widget), K->keycode,
				 0);
	    if (sym == XK_Escape) {
		fprintf(stderr, "Escape\n");
		return;
	    }
	    else if (sym == XK_equal) {
		if (K->state & ShiftMask) {
		    MainOGLWindow.ptr_D_spec->Thresh++;
		    if (MainOGLWindow.ptr_D_spec->Thresh >
			Headfax.linefax.nthres - 1)
			MainOGLWindow.ptr_D_spec->Thresh = 0;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_minus) {
		if (!(K->state & ShiftMask)) {
		    MainOGLWindow.ptr_D_spec->Thresh--;
		    if (MainOGLWindow.ptr_D_spec->Thresh < 0)
			MainOGLWindow.ptr_D_spec->Thresh =
			    Headfax.linefax.nthres - 1;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_C) {
		clear_screen();
	    }
	    else if (sym == XK_1) {
		thresh = 0;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_2) {
		thresh = 1;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_3) {
		thresh = 2;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_4) {
		thresh = 3;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_5) {
		thresh = 4;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_6) {
		thresh = 5;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_7) {
		thresh = 6;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_8) {
		thresh = 7;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_9) {
		thresh = 8;
		if (thresh < Headfax.linefax.nthres) {

		    MainOGLWindow.ptr_D_spec->Thresh = thresh;
		    do_draw_immediate_mode();
		    set_threshold_button(MainOGLWindow.ptr_D_spec->Thresh +
					 1);
		}
	    }
	    else if (sym == XK_Escape) {
		fprintf(stderr, "Escape\n");
		return;
	    }

	}
	else if (MainOGLWindow.right_button_hit) {
	    MainOGLWindow.right_button_hit = 0;
	    return;
	}
	else {
	    XtDispatchEvent(&event);
	}
    }
}

/*-------------------------------------------------------------------
  glinputCB()
  This function handles all mouse inputs in the main 3D drawing window
  -------------------------------------------------------------------
*/
HVect vNow;
HVect vNowLast;
void glinputCB(Widget widget, XtPointer client_data, XtPointer cdata)
{
    long xpos, ypos;

    static int tr_first = 0;
    static int rot_first = 0;
    static int tr_oymouse;
    static int rot_oxmouse, rot_oymouse;
    int dymouse;


    GLwDrawingAreaCallbackStruct *call_data =
	(GLwDrawingAreaCallbackStruct *) cdata;

    if (!RotationEnabled)
	return;


    switch (call_data->event->type) {
    case ButtonPress:
	if (call_data->event->xbutton.button == Button1) {
	    if (MainOGLWindow.middle_button_status == 0) {
		MainOGLWindow.left_button_status = 1;
		rot_first = 1;
		xpos = call_data->event->xbutton.x;
		ypos = MainOGLWindow.height - call_data->event->xbutton.y;
		vNow.x = (2 * (float)xpos / MainOGLWindow.width) - 1.;
		vNow.y = (2 * (float)ypos / MainOGLWindow.height) - 1.;
		Ball_Mouse(&Trackball, vNow);
		Ball_Update(&Trackball);



		Ball_BeginDrag(&Trackball);
	    }
	}
	else if (call_data->event->xbutton.button == Button2) {
	    if (MainOGLWindow.left_button_status == 0) {
		tr_first = 1;
		MainOGLWindow.middle_button_status = 1;
	    }
	}
	else if (call_data->event->xbutton.button == Button3) {
	    MainOGLWindow.right_button_hit = 1;
	}
	break;
    case ButtonRelease:
	if (call_data->event->xbutton.button == Button1) {
	    MainOGLWindow.left_button_status = 0;
	    Ball_EndDrag(&Trackball);
	    Ball_Mouse(&Trackball, vNow);
	    Ball_Update(&Trackball);
	    do_draw_with_display_list(MainOGLWindow.ptr_D_spec);
	}
	else if (call_data->event->xbutton.button == Button2) {
	    MainOGLWindow.middle_button_status = 0;
	}
	break;
    case MotionNotify:

	if (MainOGLWindow.left_button_status) {
	    /* Rotate 3D scene */
	    xpos = call_data->event->xmotion.x;
	    ypos = MainOGLWindow.height - call_data->event->xmotion.y;
	    if (rot_first) {
		rot_oxmouse = xpos;
		rot_oymouse = ypos;
		rot_first = 0;
	    }
	    vNow.x = (2 * (float)xpos / MainOGLWindow.width) - 1.;
	    vNow.y = (2 * (float)ypos / MainOGLWindow.height) - 1.;

	    Ball_Mouse(&Trackball, vNow);
	    Ball_Update(&Trackball);
	    if (((vNowLast.x - vNow.x) * (vNowLast.x - vNow.x) +
		 (vNowLast.y - vNow.y) * (vNowLast.y - vNow.y)) > .01) {
		vNowLast = vNow;
		do_draw_with_display_list(MainOGLWindow.ptr_D_spec);
	    }
	}
	else if (MainOGLWindow.middle_button_status) {
	    /* Translate 3D scene */
	    xpos = call_data->event->xmotion.x;
	    ypos = MainOGLWindow.height - call_data->event->xmotion.y;
	    if (tr_first) {
		tr_oymouse = ypos;
		tr_first = 0;
	    }
	    dymouse = ypos - tr_oymouse;
	    if (dymouse > 10 || dymouse < -10) {
		tr_oymouse = ypos;

		if (MainOGLWindow.ptr_D_spec) {
		    MainOGLWindow.ptr_D_spec->ztrans -= (float)dymouse;
		    do_draw_with_display_list(MainOGLWindow.ptr_D_spec);
		}
	    }
	}
	break;
    }
}

/*-------------------------------------------------------------------
  glinitCB()
  This function is called during initialization of both the main 3D 
  drawing window and colormap window
  -------------------------------------------------------------------
*/
void glinitCB(Widget widget, XtPointer client_data, XtPointer cdata)
{
    float xd, yd, zd, trd;
    Arg args[10];
    XVisualInfo *vi;
    int height, width;
    float aspect;
    GLwDrawingAreaCallbackStruct *call_data =
	(GLwDrawingAreaCallbackStruct *) cdata;
    int id = (int)client_data;

    XtSetArg(args[0], GLwNvisualInfo, &vi);
    XtGetValues(widget, args, 1);


    if (id == 0) {
	/* Main 3D window */
	MainOGLWindow.window = XtWindow(widget);
	MainOGLWindow.glx_context =
	    glXCreateContext(XtDisplay(widget), vi, 0, GL_TRUE);
	glXMakeCurrent(XtDisplay(widget), MainOGLWindow.window,
		       MainOGLWindow.glx_context);
	glDepthRange(0x0000, 0x7fffff);


	height = (GLuint) call_data->height - 1;
	width = (GLuint) call_data->width - 1;
	aspect = (float)width / (float)height;

	do_lights();
	clear_screen();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45., aspect, .10, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	xd = Headfax.xdim * MainOGLWindow.ptr_D_spec->xscale;
	yd = Headfax.ydim * MainOGLWindow.ptr_D_spec->yscale;
	zd = Headfax.zdim * MainOGLWindow.ptr_D_spec->zscale;

	/* pick greatest dimension to use for translation of viewer from origin */
	if (xd < yd)
	    trd = yd;
	else
	    trd = xd;
	if (trd < zd)
	    trd = zd;

	glTranslatef(0.0, 0.0, -trd * 1.6);
	glXSwapBuffers(XtDisplay(widget), MainOGLWindow.window);
	initializeFonts();

	MainDlist = glGenLists(1);
	glNewList(MainDlist, GL_COMPILE_AND_EXECUTE);
	glEndList();

	Ball_Init(&Trackball);
	Ball_Place(&Trackball, qOne, .90);


	ProceedStatus++;
    }
    else {
	ColormapWindow.window = XtWindow(widget);
	ColormapWindow.glx_context =
	    glXCreateContext(XtDisplay(widget), vi, 0, GL_TRUE);

	glXMakeCurrent(XtDisplay(widget), ColormapWindow.window,
		       ColormapWindow.glx_context);
	clear_screen();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 100, 0, 1000);
	glMatrixMode(GL_MODELVIEW);
	draw_ctable();
	winset_main();
	ProceedStatus++;
    }
}

/*-------------------------------------------------------------------
  glexposeCB()
  This function is called when the main 3D drawing window is exposed
  -------------------------------------------------------------------
*/
void glexposeCB(Widget widget, XtPointer client_data, XtPointer cdata)
{
    Window wind;
    static int First = 1;
    GLwDrawingAreaCallbackStruct *call_data =
	(GLwDrawingAreaCallbackStruct *) cdata;
    wind = XtWindow(widget);
    glXMakeCurrent(XtDisplay(widget), wind, MainOGLWindow.glx_context);
    MainOGLWindow.height = (GLuint) (call_data->height);
    MainOGLWindow.width = (GLuint) (call_data->width);
    glViewport(0, 0, (GLuint) (call_data->width),
	       (GLuint) (call_data->height));
    glScissor(0, 0, (GLuint) (call_data->width),
	      (GLuint) (call_data->height));
    glEnable(GL_DEPTH_TEST);

    clear_screen();
    if (First) {
	do_draw_immediate_mode();
	First = 0;
    }
    else {
	do_draw_with_display_list(MainOGLWindow.ptr_D_spec);
    }

    ProceedStatus++;
}

/*-------------------------------------------------------------------
  glresizeCB()
  This function is called to clear the colomap window prior to redrawing
  -------------------------------------------------------------------
*/
void glresizeCB(Widget widget, XtPointer client_data, XtPointer cdata)
{
    Window wind;
    GLdouble aspect;
    GLwDrawingAreaCallbackStruct *call_data =
	(GLwDrawingAreaCallbackStruct *) cdata;

    wind = XtWindow(widget);
    glXMakeCurrent(XtDisplay(widget), wind, MainOGLWindow.glx_context);

    glViewport(0, 0, (GLuint) (call_data->width),
	       (GLuint) (call_data->height));
    glScissor(0, 0, (GLuint) (call_data->width),
	      (GLuint) (call_data->height));

    MainOGLWindow.height = (GLuint) (call_data->height);
    MainOGLWindow.width = (GLuint) (call_data->width);

    aspect = (float)MainOGLWindow.width / (float)MainOGLWindow.height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45., aspect, .10, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    do_draw_with_display_list(MainOGLWindow.ptr_D_spec);

}

/*-------------------------------------------------------------------
  clear_screen2()
  This function is called to clear the colomap window prior to redrawing
  -------------------------------------------------------------------
*/
void clear_screen2()
{

    glDisable(GL_LIGHTING);
    glClearColor(1., 1., 1., 1.);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.);
    glClear(GL_DEPTH_BUFFER_BIT);
}

/*-------------------------------------------------------------------
  glexpose2CB()
  This function is called when the colormap window gets an expose event
  -------------------------------------------------------------------
*/
void glexpose2CB(Widget widget, XtPointer client_data, XtPointer cdata)
{
    Window wind;
    GLwDrawingAreaCallbackStruct *call_data =
	(GLwDrawingAreaCallbackStruct *) cdata;

    wind = XtWindow(widget);
    winset_colortable();

    glXMakeCurrent(XtDisplay(widget), wind, ColormapWindow.glx_context);

    glDisable(GL_DEPTH_TEST);
    ColormapWindow.height = (GLuint) (call_data->height);
    ColormapWindow.width = (GLuint) (call_data->width);

    glViewport(0, 0, (GLuint) (call_data->width),
	       (GLuint) (call_data->height));
    glScissor(0, 0, (GLuint) (call_data->width),
	      (GLuint) (call_data->height));

    clear_screen2();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 100, 0, 1000, -10000., 1000.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    draw_ctable();

    glXSwapBuffers(XtDisplay(ColormapWindow.widget), ColormapWindow.window);

    ProceedStatus++;
    winset_main();
}

/*-------------------------------------------------------------------
  glresize2CB()
  This function is called when the colormap window is resized 
  -------------------------------------------------------------------
*/
void glresize2CB(Widget widget, XtPointer client_data, XtPointer cdata)
{
    Window wind;
    GLwDrawingAreaCallbackStruct *call_data =
	(GLwDrawingAreaCallbackStruct *) cdata;

    wind = XtWindow(widget);
    glXMakeCurrent(XtDisplay(widget), wind, ColormapWindow.glx_context);
    glViewport(0, 0, (GLuint) (call_data->width),
	       (GLuint) (call_data->height));
    glScissor(0, 0, (GLuint) (call_data->width),
	      (GLuint) (call_data->height));
    ColormapWindow.height = (GLuint) (call_data->height);
    ColormapWindow.width = (GLuint) (call_data->width);

    clear_screen2();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 100, 0, 1000, -10000., 1000.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    draw_ctable();
    winset_main();
}

/*-------------------------------------------------------------------
  rotate_model()
  This function is called when the program is put into "Rotation Mode"
  where the user is modifying the view using mouse input.
  -------------------------------------------------------------------
*/
void rotate_model(struct dspec *D_spec)
{
    enableRotation();
    rotateLoop();
    disableRotation();
}

/*-------------------------------------------------------------------
  SingleMultipleCB()
  -------------------------------------------------------------------
*/
void SingleMultipleCB(Widget widget, XtPointer client_data, XtPointer cdata)
{
    int flag = (int)client_data;
    Arg args[10];
    int n;
    int i;

    if (flag == 1) {
	MultipleThresholdFlag = 0;
	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True);
	n++;
	XtSetValues(ThresholdRadio, args, n);
	XtManageChild(Button_plus);
	XtManageChild(Button_minus);
	XtUnmanageChild(PlotSelected);
	for (i = 1; i <= Headfax.linefax.nthres; i++) {
	    if (i <= MAXTHRESHOLDS)
		XmToggleButtonSetState(Threshbutton[i - 1], False, False);
	}
    }
    else {
	MultipleThresholdFlag = 1;
	for (i = 1; i <= Headfax.linefax.nthres; i++) {
	    if (i <= MAXTHRESHOLDS)
		XmToggleButtonSetState(Threshbutton[i - 1], False, False);
	}
	n = 0;
	XtSetArg(args[n], XmNradioBehavior, False);
	n++;
	XtSetValues(ThresholdRadio, args, n);
	XtUnmanageChild(Button_plus);
	XtUnmanageChild(Button_minus);
	XtManageChild(PlotSelected);
    }
}

/*-------------------------------------------------------------------
  PlotSelectedCB()
  This function is the callback function for the "Plot Selected" button.
  A plot containing thresholds corresponding to all selected Threshold 
  toggle buttons will be drawn.
  -------------------------------------------------------------------
*/
void PlotSelectedCB(Widget widget, XtPointer client_data, XtPointer cdata)
{
    int i;
    int selectedList[200];
    int numSelected = 0;

    for (i = 1; i <= Headfax.linefax.nthres; i++) {
	if (i <= MAXTHRESHOLDS) {
	    if (XmToggleButtonGetState(Threshbutton[i - 1]) == True) {
		selectedList[numSelected] = i;
		numSelected++;
	    }
	}
    }
    for (i = 0; i < numSelected; i++) {
	MainOGLWindow.ptr_D_spec->t[i] = selectedList[i] - 1;
    }
    MainOGLWindow.ptr_D_spec->nt = numSelected;
    draw_multiple();

}

/*-------------------------------------------------------------------
  setSingleSelectionMode()
  This function causes the Motif GUI to act in Single Selection Mode
  with regard to Threshold levels
  -------------------------------------------------------------------
*/
void setSingleSelectionMode()
{
    Arg args[10];
    int n;
    int i;

    XmToggleButtonSetState(SingleToggle, True, True);
    MultipleThresholdFlag = 0;
    n = 0;
    XtSetArg(args[n], XmNradioBehavior, True);
    n++;
    XtSetValues(ThresholdRadio, args, n);
    XtManageChild(Button_plus);
    XtManageChild(Button_minus);
    XtUnmanageChild(PlotSelected);
    for (i = 1; i <= Headfax.linefax.nthres; i++) {
	if (i <= MAXTHRESHOLDS)
	    XmToggleButtonSetState(Threshbutton[i - 1], False, False);
    }
}

/*-------------------------------------------------------------------
  setSingleSelectionMode()
  This function causes the Motif GUI to act in Multiple Selection Mode
  with regard to Threshold levels
  -------------------------------------------------------------------
*/
void setMultipleSelectionMode()
{
    Arg args[10];
    int n;
    int i;

    MultipleThresholdFlag = 1;
    XmToggleButtonSetState(MultipleToggle, True, True);
    for (i = 1; i <= Headfax.linefax.nthres; i++) {
	if (i <= MAXTHRESHOLDS)
	    XmToggleButtonSetState(Threshbutton[i - 1], False, False);
    }
    n = 0;
    XtSetArg(args[n], XmNradioBehavior, False);
    n++;
    XtSetValues(ThresholdRadio, args, n);
    XtUnmanageChild(Button_plus);
    XtUnmanageChild(Button_minus);
    XtManageChild(PlotSelected);
}

/*-------------------------------------------------------------------
  isSingleSelectionMode()
  This function returns 1 if the GUI is currently behaving in Single
  Selection Mode or returns 0 if the GUI is currently behaving in
  Multiple selection mode.
  -------------------------------------------------------------------
*/
int isSingleSelectionMode()
{
    return !MultipleThresholdFlag;
}
