#include <string.h>
#include <grass/raster.h>
#include "globals.h"
#include "camera_ref.h"


#undef DEBUG

/* define MOUSE_YN to answer y/n by mouse click */
#define	MOUSE_YN


static char buf[300];
static int get_point2(double *, double *);
static int keyboard(void);
static int _keyboard(void);
static int fromfile(void);
static int _drawcam(void);
static int uparrow(struct box *, int);
static int downarrow(struct box *, int);
static int pick(int, int);
static int done(void);
static int cancel_which(void);
static int inbox(struct box *, int, int);
static int dotext(char *, int, int, int, int, int, int);

#ifdef DEBUG
static int show_point(int, int);
static int debug(char *);
#endif


int mark(int x, int y, int button)
{
    if (button != 1)
	return where(x, y);

    if (VIEW_MAP1->cell.configured && In_view(VIEW_MAP1, x, y))
	mark_point(VIEW_MAP1, x, y);
    else if (VIEW_MAP1_ZOOM->cell.configured && In_view(VIEW_MAP1_ZOOM, x, y))
	mark_point(VIEW_MAP1_ZOOM, x, y);
    return 0;			/* return but don't quit */
}

int mark_point(View * view, int x, int y)
{
    double e1, n1;
    double e2, n2;
    int row, col;

    char buf[100];

    /* convert x,y to east,north at center of cell */
    col = view_to_col(view, x);
    e1 = col_to_easting(&view->cell.head, col, 0.5);
    row = view_to_row(view, y);
    n1 = row_to_northing(&view->cell.head, row, 0.5);

    Curses_clear_window(MENU_WINDOW);
    sprintf(buf, "Point %d marked at IMAGE COORDINATES:",
	    group.photo_points.count + 1);
    Curses_write_window(MENU_WINDOW, 1, 1, buf);
    sprintf(buf, "East:  %10.2f", e1);
    Curses_write_window(MENU_WINDOW, 3, 3, buf);
    sprintf(buf, "North: %10.2f", n1);
    Curses_write_window(MENU_WINDOW, 4, 3, buf);
    Curses_clear_window(INFO_WINDOW);

    R_standard_color(ORANGE);
    save_under_dot(x, y);
    dot(x, y);

    if (!get_point2(&e2, &n2)) {
	Curses_clear_window(MENU_WINDOW);
	restore_under_dot();
    }
    else {

	sprintf(buf, "Point %d referenced to PHOTO COORDINATES:",
		group.photo_points.count + 1);
	Curses_write_window(MENU_WINDOW, 7, 1, buf);
	sprintf(buf, "X:  %10.2f", e2);
	Curses_write_window(MENU_WINDOW, 9, 3, buf);
	sprintf(buf, "Y:  %10.2f", n2);
	Curses_write_window(MENU_WINDOW, 10, 3, buf);
	I_new_ref_point(&group.photo_points, e1, n1, e2, n2, 1);
	I_put_ref_points(group.name, &group.photo_points);
	Compute_equation();
	display_ref_points(1);
    }
    release_under_dot();

    return 0;
}

static double N, E;

static int get_point2(double *east, double *north)
{
    int stat;

    if (from_screen < 0) {
	from_flag = 1;
	from_screen = 0;
	if (from_keyboard < 0) {
	    from_keyboard = 0;
	    from_screen = 1;
	}
    }

    if (from_screen > 0) {
	stat = Input_other(fromfile, "CAMERA FILE");
	set_colors(&VIEW_MAP1->cell.colors);
    }
    else
	stat = Input_other(keyboard, "KEYBOARD");

    if (stat) {
	*east = E;
	*north = N;
    }

    return stat;
}

static int keyboard(void)
{
    int ok;

    Curses_clear_window(INFO_WINDOW);
    ok = _keyboard();
    Curses_clear_window(INFO_WINDOW);
    return ok;
}

static int _keyboard(void)
{
    char buf[100];

    while (1) {
	Curses_prompt_gets("Enter PHOTO COORDINATES as X Y: ", buf);
	G_strip(buf);
	if (*buf == 0) {
	    return 0;
	}
	if (sscanf(buf, "%lf %lf", &E, &N) != 2) {
	    Beep();
	    continue;
	}
	Curses_clear_window(INFO_WINDOW);
	sprintf(buf, "X:  %f\n", E);
	Curses_write_window(INFO_WINDOW, 3, 2, buf);
	sprintf(buf, "Y:  %f\n", N);
	Curses_write_window(INFO_WINDOW, 4, 2, buf);
#ifdef	MOUSE_YN
	Curses_write_window(INFO_WINDOW, 5, 2,
			    "Look ok? (Left: y / Right: n) ");
#else
	Curses_write_window(INFO_WINDOW, 5, 2, "Look ok? (y/n) ");
#endif

	while (1) {
#ifdef	MOUSE_YN
	    int x, y, b;

	    R_get_location_with_pointer(&x, &y, &b);
	    if (b == 1)
		return 1;
	    else if (b == 3)
		break;
#else
	    int c;

	    c = Curses_getch(0);
	    if (c == 'y' || c == 'Y')
		return 1;
	    if (c == 'n' || c == 'N')
		break;
#endif
	    Beep();
	}
    }
}


static int fromfile(void)
{
    /*  int ok; */
    Curses_clear_window(INFO_WINDOW);
    _drawcam();
    Curses_clear_window(INFO_WINDOW);
    return ok;
}

static int _drawcam(void)
{
    static int use = 1;
    static Objects objects[] = {
	MENU("CANCEL", done, &use),
	INFO(" Double click on point to be referenced", &use),
	OTHER(pick, &use),
	{0}
    };

    /* to give user a response of some sort */
    Menu_msg("Preparing Camera Reference File...");

    /*
     * more, less, and report boxes defined in use_camera.c
     *
     */

    /* allocate predicted values */
    Xf = (double *)G_calloc(group.camera_ref.num_fid, sizeof(double));
    Yf = (double *)G_calloc(group.camera_ref.num_fid, sizeof(double));

    /*  redraw current report */

/****
    R_standard_color (GREY);
    R_box_abs (report.left, report.top, report.right, report.bottom);
    R_standard_color (BACKGROUND);
****/

    /* lets do it */
    /*    curp = first_point = 0;   */
    pager = 0;
    while (1) {
	if (pager) {
	    R_standard_color(GREY);
	    R_box_abs(report.left, report.top, report.right, report.bottom);
	    R_standard_color(BACKGROUND);
	    line = 0;
	    curp = first_point;
	}

	R_text_size(tsize, tsize);
	cury = report.top;
	/* line = 0; */

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
	    report.bottom = cury;
	}

	downarrow(&more,
		  curp < group.camera_ref.num_fid ? BLACK : BACKGROUND);
	uparrow(&less, first_point > 0 ? BLACK : BACKGROUND);

	pager = 0;
	which = -1;
	if (Input_pointer(objects) < 0)
	    break;
    }
    return 1;
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
    int n;
    int cur;

    cur = which;
    cancel_which();
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
    if (!inbox(&report, x, y)) {
	return 0;
    }

    n = (y - report.top) / height;
    /* debug ("n = %d",n);
       debug ("cur = %d",cur); */

    if (n == cur) {		/* second click! */

	/* debug ("Getting point %d   E = %f  N = %f",n, E, N); */

	E = group.camera_ref.fiducials[first_point + n].Xf;
	N = group.camera_ref.fiducials[first_point + n].Yf;
	/* debug ("Got point %d   E = %f  N = %f",n, E, N); */
	Curses_clear_window(INFO_WINDOW);
	sprintf(buf, "X:  %f\n", E);
	Curses_write_window(INFO_WINDOW, 3, 2, buf);
	sprintf(buf, "Y:  %f\n", N);
	Curses_write_window(INFO_WINDOW, 4, 2, buf);
#ifdef	MOUSE_YN
	Curses_write_window(INFO_WINDOW, 5, 1,
			    "Look ok? (Left: y / Right: n) ");
#else
	Curses_write_window(INFO_WINDOW, 5, 1, "Look ok? (y/n) ");
	Curses_write_window(PROMPT_WINDOW, 1, 1, "Keyboard Input Required ");
#endif

	while (1) {
#ifdef	MOUSE_YN
	    int x, y, b;

	    R_get_location_with_pointer(&x, &y, &b);
	    if (b == 1) {
		ok = 1;
		return -1;
	    }
	    else if (b == 3) {
		ok = 0;
		break;
	    }
#else
	    int c;

	    c = Curses_getch(0);
	    if (c == 'y' || c == 'Y') {
		ok = 1;
		return -1;
	    }
	    if (c == 'n' || c == 'N') {
		ok = 0;
		break;
	    }
#endif
	    Beep();
	}

#ifdef DEBUG
	show_point(first_point + n, 1);
#endif
	Curses_clear_window(INFO_WINDOW);
	Curses_write_window(PROMPT_WINDOW, 1, 1, "Use Mouse Now \n");
	return 1;
    }
    which = n;
#ifdef DEBUG
    show_point(first_point + n, 0);
#endif
    R_standard_color(RED);
    Outline_box(report.top + n * height, report.top + (n + 1) * height,
		report.left, report.right - 1);
    Curses_write_window(PROMPT_WINDOW, 1, 1, "Use Mouse Now \n");

    return 0;			/* ignore first click */
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
#ifdef DEBUG
	show_point(first_point + which, 1);
#endif
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


#ifdef DEBUG
static int show_point(int n, int true_color)
{
    if (!true_color)
	R_standard_color(ORANGE);
    else if (group.photo_points.status[n])
	R_standard_color(GREEN);
    else
	R_standard_color(RED);

    return 0;
}


static int debug(char *msg)
{
    R_stabilize();
    Curses_write_window(PROMPT_WINDOW, 1, 1, msg);
    Curses_getch(0);

    return 0;
}
#endif
