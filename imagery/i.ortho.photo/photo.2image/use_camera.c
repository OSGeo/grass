#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include "globals.h"
#define  GLOBALCAM
#include "camera_ref.h"


static char buf[300];

#if 0
static int no(void);
#endif
static int yes(void);
static int drawcamnew(void);
static int uparrow(struct box *, int);
static int downarrow(struct box *, int);
static int pick(int, int);
static int done(void);
static int cancel_which(void);
static int inbox(struct box *, int, int);
static int dotext(char *, int, int, int, int, int, int);


int setup_camera_file(void)
{
    char *camera;

#if 0
    static int use = 1;
    static Objects objects[] = {
	INFO("Do you wish to use the camera reference file?  ", &use),
	MENU("YES", yes, &use),
	MENU("NO", no, &use),
	{0}
    };
#endif

    camera = (char *)G_malloc(40 * sizeof(char));
    use_camera_file = 1;

/** look for camera info  for this group **/
    if (!I_get_group_camera(group.name, camera)) {
	use_camera_file = 0;
	return 0;
    }
    if (!I_get_cam_info(camera, &group.camera_ref)) {
	use_camera_file = 0;
	return 0;
    }
    yes();
    return (0);

#if 0

/*** camera required 
    Start_mouse_in_menu();
    if (Input_pointer (objects) < 0) return (1);
    else return 0;
****/
#endif
}

#if 0
static int no(void)
{
    use_camera_file = 0;
    return -1;
}
#endif

static int yes(void)
{
    use_camera_file = 1;
    drawcamnew();
    return -1;
}


static int drawcamnew(void)
{
    static int use = 1;
    static Objects objects[] = {
	MENU("CANCEL", done, &use),
	INFO("Click mouse to continue.", &use),
	OTHER(pick, &use),
	{0}
    };

    /* to give user a response of some sort */
    Menu_msg("Preparing Camera Reference File...");

    /*
     * build a popup window at center of the screen.
     * 45% the height and wide enough to hold the report
     *
     */

    /* height of 1 line, based on NLINES taking up 45% vertical space */
    height = (.45 * (SCREEN_BOTTOM - SCREEN_TOP)) / NLINES + 1;

    /* size of text, 80% of line height */
    tsize = .8 * height;
    size = tsize - 2;		/* fudge for computing pixels width of text */

    /* indent for the text */
    edge = .1 * height + 1;

    /* determine the length, in chars, of printed line */
    FMT0f(buf, 0.0);
    nums = strlen(buf) * size;
    FMT1(buf, " ", 0.0, 0.0);
    len = strlen(buf);
    middle = len * size;

    /* width is for max chars plus sidecar for ess */
    width = len * size + nums + (2 * height);
    if ((SCREEN_RIGHT - SCREEN_LEFT) < width)
	width = SCREEN_RIGHT - SCREEN_LEFT;


    /* define the window */
    bottom = VIEW_MENU->top - 1;
    top = bottom - height * NLINES;


    left = SCREEN_LEFT;
    right = left + width;
    middle += left + nums;
    nums += left;

    /* save what is under this area, so it can be restored */
    R_panel_save(tempfile1, top, bottom, left, right);


    /* fill it with white */
    R_standard_color(BACKGROUND);
    R_box_abs(left, top, right, bottom);

    right -= 2 * height;	/* reduce it to exclude sidecar */

    /* print messages in message area */
    R_text_size(tsize, tsize);


    /* setup the more/less boxes in the sidecar */
    R_standard_color(BLACK);
    less.top = top;
    less.bottom = top + 2 * height;
    less.left = right;
    less.right = right + 2 * height;
    Outline_box(less.top, less.bottom, less.left, less.right);

    more.top = bottom - 2 * height;
    more.bottom = bottom;
    more.left = right;
    more.right = right + 2 * height;
    Outline_box(more.top, more.bottom, more.left, more.right);

    /*
     * top eight lines are for column labels
     * last two line is for overall rms error.
     */
    nlines = NLINES - 11;
    first_point = 0;

    /* allocate predicted values */
    Xf = (double *)G_calloc(group.camera_ref.num_fid, sizeof(double));
    Yf = (double *)G_calloc(group.camera_ref.num_fid, sizeof(double));


    /* put head on the report */
    cury = top;
    dotext(LHEAD1, cury, cury + height, left, right - 1, 0, BLACK);
    cury += height;
    dotext(LHEAD2, cury, cury + height, left, right - 1, 0, BLACK);
    cury += height;

    FMT2(buf, group.camera_ref.cam_name);
    dotext(buf, cury, cury + height, left, right - 1, 0, color);
    cury += height;
    FMT3(buf, group.camera_ref.cam_id);
    dotext(buf, cury, cury + height, left, right - 1, 0, color);
    cury += height;
    FMT4(buf, group.camera_ref.CFL);
    dotext(buf, cury, cury + height, left, right - 1, 0, color);
    cury += height;
    FMT5(buf, group.camera_ref.Xp);
    dotext(buf, cury, cury + height, left, right - 1, 0, color);
    cury += height;
    FMT6(buf, group.camera_ref.Yp);
    dotext(buf, cury, cury + height, left, right - 1, 0, color);
    cury += height;
    FMT7(buf, group.camera_ref.num_fid);
    dotext(buf, cury, cury + height, left, right - 1, 0, color);
    cury += height;

    dotext(LHEAD3, cury, cury + height, left, right - 1, 0, BLACK);
    cury += height;
    dotext(LHEAD4, cury, cury + height, left, right - 1, 0, BLACK);
    cury += height;

    R_move_abs(left, cury - 1);
    R_cont_abs(right, cury - 1);

    /* isolate the sidecar */
    R_move_abs(right, top);
    R_cont_abs(right, bottom);

    /* define report box */
    /*  report.top = top + (10*height); */
    report.top = cury;
    report.left = left;
    report.right = right - 1;

    /* lets do it */
    pager = 1;
    while (1) {
	R_text_size(tsize, tsize);
	line = 0;
	curp = first_point;
	cury = top + 10 * height;
	while (1) {
	    if (line >= nlines || curp >= group.camera_ref.num_fid)
		break;
	    line++;
	    color = BLACK;
	    if (pager) {
		FMT1(buf, group.camera_ref.fiducials[curp].fid_id,
		     group.camera_ref.fiducials[curp].Xf,
		     group.camera_ref.fiducials[curp].Yf);
		dotext(buf, cury, cury + height, left, right - 1, 0, color);
	    }
	    cury += height;
	    curp++;
	}
	report.bottom = cury;
	downarrow(&more,
		  curp < group.camera_ref.num_fid ? BLACK : BACKGROUND);
	uparrow(&less, first_point > 0 ? BLACK : BACKGROUND);
	R_standard_color(BACKGROUND);
	R_box_abs(left, cury, right - 1, bottom);
	R_standard_color(BLACK);
	R_move_abs(left, bottom - height);
	R_cont_abs(right - 1, bottom - height);

	pager = 0;
	which = -1;
	if (Input_pointer(objects) < 0)
	    break;
    }
    return 1;			/* return but don't QUIT */
}


static int uparrow(struct box *box, int color)
{
    R_standard_color(color);
    Uparrow(box->top + edge, box->bottom - edge, box->left + edge,
	    box->right - edge);

    return 0;
}

static int downarrow(struct box *box, int color)
{
    R_standard_color(color);
    Downarrow(box->top + edge, box->bottom - edge, box->left + edge,
	      box->right - edge);

    return 0;
}

static int pick(int x, int y)
{
    int cur;

    cur = which;
    if (inbox(&more, x, y)) {
	if (curp >= group.camera_ref.num_fid)
	    return 0;
	first_point = curp;
	pager = 1;
	return 1;
    }
    if (inbox(&less, x, y)) {
	if (first_point == 0)
	    return 0;
	first_point -= nlines;
	if (first_point < 0)
	    first_point = 0;
	pager = 1;
	return 1;
    }
    return -1;			/* keep going */
    /* return 0;  ignore first click */
}

static int done(void)
{
    cancel_which();
    ok = 0;
    return -1;
}

static int cancel_which(void)
{
    if (which >= 0) {
	R_standard_color(BACKGROUND);
	Outline_box(report.top + which * height,
		    report.top + (which + 1) * height, report.left,
		    report.right - 1);
	/* show_point (first_point+which, 1); */
    }
    which = -1;

    return 0;
}

static int inbox(struct box *box, int x, int y)
{
    return (x > box->left && x < box->right && y > box->top &&
	    y < box->bottom);
}

static int dotext(char *text,
		  int top, int bottom, int left, int right, int centered,
		  int color)
{
    R_standard_color(BACKGROUND);
    R_box_abs(left, top, right, bottom);
    R_standard_color(color);
    R_move_abs(left + 1 + edge, bottom - 1 - edge);
    if (centered)
	R_move_rel((right - left - strlen(text) * size) / 2, 0);
    R_set_window(top, bottom, left, right);	/* for text clipping */
    R_text(text);
    R_set_window(SCREEN_TOP, SCREEN_BOTTOM, SCREEN_LEFT, SCREEN_RIGHT);

    return 0;
}
