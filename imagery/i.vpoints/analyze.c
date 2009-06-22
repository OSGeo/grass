#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include "globals.h"
#include <grass/display.h>
#include "crs.h"

#define NLINES 18
struct box
{
    int top, bottom, left, right;
};

static int which;
static struct box more, less, report;
static int height, size, edge, nlines;
static int curp, first_point;
static double rms;
static double *xres, *yres, *gnd;
static int pager;
static int xmax, ymax, gmax;
static char buf[300];

static char delete_msg[11] = "delete_off";
static char pick_msg[40] = " Double click to include/exclude pt. ";
static int delete_mode;
static char order_msg[10] = "1st ORDER";
static int trans_order = 1;

#define FMT0(buf,n) \
	sprintf (buf, "%3d ", n)
#define FMT1(buf,xres,yres,gnd) \
	sprintf (buf, "%5.1f %5.1f %6.1f ", xres,yres,gnd)
#define LHEAD1 "        error          "
#define LHEAD2 "  #   col   row  target"

#define FMT2(buf,e1,n1,e2,n2) \
	sprintf (buf, "%9.1f %9.1f %9.1f %9.1f ", e1,n1,e2,n2)
#define RHEAD1 "         image              target"
#define RHEAD2 "    east     north      east     north"

#define BACKGROUND GREY

static int delete_mark(void);
static int do_warp(void);
static int rast_redraw(void);
static int warp(void);
static int no_warp(void);
static int downarrow(struct box *, int);
static int uparrow(struct box *, int);
static int pick(int, int);
static int get_order(void);
static int show_point(int, int);
static int printcentered(FILE *, char *, int);
static int inbox(struct box *, int, int);
static int cancel_which(void);
static int done(void);
static int do_report(FILE *);
static int to_printer(void);
static int askfile(void);
static int to_file(void);
static int dotext(char *, int, int, int, int, int, int);
static int compute_transformation(void);

static int do_1st(void)
{
    strcpy(order_msg, "1st ORDER");
    trans_order = 1;
    if (compute_transformation())
	return 1;		/* back to analyze menu */

    rast_redraw();
    return 1;
}

static int do_2nd(void)
{
    strcpy(order_msg, "2nd ORDER");
    trans_order = 2;
    if (compute_transformation())
	return 0;

    rast_redraw();
    return 1;
}

static int do_3rd(void)
{
    strcpy(order_msg, "3rd ORDER");
    trans_order = 3;
    if (compute_transformation())
	return 0;

    rast_redraw();
    return 1;
}

static int delete_mark(void)
{
    if (delete_mode) {
	strcpy(pick_msg, " Double click to include/exclude point");
	strcpy(delete_msg, "delete_off");
	delete_mode = 0;
    }
    else {
	strcpy(pick_msg, " Double click on point to be DELETED   ");
	strcpy(delete_msg, "DELETE_ON ");
	delete_mode = 1;
    }
    pager = 1;			/* redisplay entire form */

    return 1;
}

int delete_control_point(int n)
{
    int i;
    char msg[80];

    if ((n < 0) | (n > group.points.count - 1)) {
	sprintf(msg, "%d is an invalid control point index value.", n);
	G_warning(msg);
	return 1;
    }
    for (i = n; i < group.points.count - 1; i++) {
	G_copy(&group.points.e1[i], &group.points.e1[i + 1],
	       sizeof(group.points.e1[0]));
	G_copy(&group.points.n1[i], &group.points.n1[i + 1],
	       sizeof(group.points.n1[0]));
	G_copy(&group.points.e2[i], &group.points.e2[i + 1],
	       sizeof(group.points.e2[0]));
	G_copy(&group.points.n2[i], &group.points.n2[i + 1],
	       sizeof(group.points.n2[0]));
	G_copy(&group.points.status[i], &group.points.status[i + 1],
	       sizeof(group.points.status[0]));
    }
    group.points.count -= 1;
    if (I_put_control_points(group.name, &group.points) < 0) {
	G_fatal_error("bad return on I_put_control_points");
	return 1;
    }
    return 0;
}

/*---------------------------------------------------------------
*/
static int do_warp(void)
{
    static int use = 1;
    int x, y;

    static Objects objects[] = {
	MENU("RASTER REDRAW", rast_redraw, &use),
	INFO("Overlay vectors on raster image? ", &use),
	MENU("NO", no_warp, &use),
	MENU("YES", warp, &use),
	{0}
    };
    x = (SCREEN_LEFT + SCREEN_RIGHT) / 2;
    y = SCREEN_BOTTOM;
    Set_mouse_xy(x, y);
    Input_pointer(objects);
    return 1;
}

static int no_warp(void)
{
    return -1;
}

static int warp(void)
{
    warpvect(group.E21, group.N21, trans_order);
    return -1;
}

static int rast_redraw(void)
{
    Erase_view(VIEW_MAP1);
    drawcell(VIEW_MAP1, 0);	/* 0 means don't initialize VIEW_MAP2 */
    display_points(1);
    return 0;
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
	if (curp >= group.points.count)
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
    if (n == cur) {		/* second click! */
	if (!delete_mode) {
	    group.points.status[first_point + n] =
		!group.points.status[first_point + n];
	    compute_transformation();
	    show_point(first_point + n, 1);
	    return 1;
	}
	else {
	    delete_control_point(first_point + n);
	    first_point = 0;
	    compute_transformation();
	    pager = 1;
	    return 1;
	}
    }

    /* first click */
    which = n;
    show_point(first_point + n, 0);
    if (!delete_mode)
	R_standard_color(RED);
    else
	R_standard_color(ORANGE);

    Outline_box((report.top + n * height) + 1, report.top + (n + 1) * height,
		report.left, report.right - 1);

    R_flush();

    return 0;			/* ignore first click */

}

static int done(void)
{
    cancel_which();
    return -1;
}

static int cancel_which(void)
{
    if (which >= 0) {
	R_standard_color(BACKGROUND);
	Outline_box((report.top + which * height) + 1,
		    report.top + (which + 1) * height, report.left,
		    report.right - 1);
	show_point(first_point + which, 1);
    }
    which = -1;

    return 0;
}

static int inbox(struct box *box, int x, int y)
{
    return (x > box->left && x < box->right && y > box->top &&
	    y < box->bottom);
}

static int dotext(char *text, int top, int bottom, int left, int right,
		  int centered, int color)
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

static int compute_transformation(void)
{				/* returns 0 on success, 1 on fail */
    int n, count;
    double d, d1, d2, sum;
    double e1, e2, n1, n2;
    double xval, yval, gval;
    static int order_pnts[3] = { 3, 6, 10 };
    char msg[40];

    xmax = ymax = gmax = 0;
    xval = yval = gval = 0.0;

    CRS_Compute_equation(trans_order);
    if (group.equation_stat <= 0) {
	if (group.equation_stat == 0) {
	    sprintf(msg, "Not Enough Points -- %d are required.",
		    order_pnts[trans_order - 1]);
	    Menu_msg(msg);
	    G_sleep(2);
	}
	return 1;
    }


    /* compute the row,col error plus ground error 
     * keep track of largest and second largest error
     */
    sum = 0.0;
    rms = 0.0;
    count = 0;
    for (n = 0; n < group.points.count; n++) {
	if (group.points.status[n] <= 0)
	    continue;
	count++;
	CRS_georef(group.points.e2[n], group.points.n2[n], &e1, &n1,
		   group.E21, group.N21, trans_order);
	CRS_georef(group.points.e1[n], group.points.n1[n], &e2, &n2,
		   group.E12, group.N12, trans_order);

	if ((d = xres[n] = e1 - group.points.e1[n]) < 0)
	    d = -d;
	if (d > xval) {
	    xmax = n;
	    xval = d;
	}

	if ((d = yres[n] = n1 - group.points.n1[n]) < 0)
	    d = -d;
	if (d > yval) {
	    ymax = n;
	    yval = d;
	}

	/* compute ground error (ie along diagonal) */
	d1 = e2 - group.points.e2[n];
	d2 = n2 - group.points.n2[n];
	d = d1 * d1 + d2 * d2;
	sum += d;		/* add it to rms sum, before taking sqrt */
	d = sqrt(d);
	gnd[n] = d;
	if (d > gval) {		/* is this one the max? */
	    gmax = n;
	    gval = d;
	}
    }

    /* compute overall rms error */
    if (count)
	rms = sqrt(sum / count);

    return 0;
}

static int to_file(void)
{
    FILE *fd;
    char msg[1024];

    cancel_which();
    if (Input_other(askfile, "Keyboard") < 0) {
	return 0;
    }

    fd = fopen(buf, "w");
    if (fd == NULL) {
	sprintf(msg, "** Unable to create file %s\n", buf);
	Beep();
	Curses_write_window(PROMPT_WINDOW, 2, 1, msg);
    }
    else {
	do_report(fd);
	fclose(fd);
	sprintf(msg, "Report saved in file %s\n", buf);
	Curses_write_window(PROMPT_WINDOW, 2, 1, msg);
    }
    return 0;
}

static int askfile(void)
{
    char file[GNAME_MAX];

    while (1) {
	Curses_prompt_gets("Enter file to hold report: ", file);
	G_strip(file);
	if (*file == 0)
	    return -1;
	if (G_index(file, '/'))
	    strcpy(buf, file);
	else
	    sprintf(buf, "%s/%s", G_home(), file);
	if (access(buf, 0) != 0)
	    return 1;
	sprintf(buf, "** %s already exists. choose another file", file);
	Beep();
	Curses_write_window(PROMPT_WINDOW, 2, 1, buf);
    }

    return 0;
}

static int to_printer(void)
{
    FILE *fd;

    cancel_which();
    Menu_msg("Sending report to printer ...");
    Curses_write_window(PROMPT_WINDOW, 1, 1, "Sending report to printer ...");
    fd = popen("lp", "w");
    do_report(fd);
    pclose(fd);
    return 0;
}

static int do_report(FILE * fd)
{
    char buf[100];
    int n;
    int width;

    fprintf(fd, "LOCATION: %-20s GROUP: %-20s MAPSET: %s\n\n",
	    G_location(), group.name, G_mapset());
    fprintf(fd, "%15sAnalysis of control point registration\n\n", "");
    fprintf(fd, "%s   %s\n", LHEAD1, RHEAD1);
    fprintf(fd, "%s   %s\n", LHEAD2, RHEAD2);

    FMT1(buf, 0.0, 0.0, 0.0);
    width = strlen(buf);

    for (n = 0; n < group.points.count; n++) {
	FMT0(buf, n + 1);
	fprintf(fd, "%s", buf);
	if (group.equation_stat > 0 && group.points.status[n] > 0) {
	    FMT1(buf, xres[n], yres[n], gnd[n]);
	    fprintf(fd, "%s", buf);
	}
	else if (group.points.status[n] > 0)
	    printcentered(fd, "?", width);
	else
	    printcentered(fd, "not used", width);
	FMT2(buf,
	     group.points.e1[n],
	     group.points.n1[n], group.points.e2[n], group.points.n2[n]);
	fprintf(fd, "   %s\n", buf);
    }
    fprintf(fd, "\n");
    if (group.equation_stat < 0)
	fprintf(fd, "Poorly place control points\n");
    else if (group.equation_stat == 0)
	fprintf(fd, "No active control points\n");
    else
	fprintf(fd,
		"Overall rms error: %.2f                %s Transformation\n",
		rms, order_msg);

    return 0;
}

static int printcentered(FILE * fd, char *buf, int width)
{
    int len;
    int n;
    int i;

    len = strlen(buf);
    n = (width - len) / 2;

    for (i = 0; i < n; i++)
	fprintf(fd, " ");
    fprintf(fd, "%s", buf);
    i += len;
    while (i++ < width)
	fprintf(fd, " ");

    return 0;
}

static int show_point(int n, int true_color)
{
    if (!true_color)
	R_standard_color(ORANGE);
    else if (group.points.status[n])
	R_standard_color(GREEN);
    else
	R_standard_color(RED);

    display_one_point(VIEW_MAP1, group.points.e1[n], group.points.n1[n]);

    return 0;
}

static int get_order(void)
{
    static int use = 1;

    static Objects objects[] = {
	INFO("Select order of transformation->", &use),
	MENU("1st Order", do_1st, &use),
	MENU("2nd Order", do_2nd, &use),
	MENU("3rd Order", do_3rd, &use),
	{0}
    };

    if (Input_pointer(objects) < 0)
	return -1;

    return 1;

}

int analyze(void)
{
    static int use = 1;

    static Objects objects[] = {
	MENU("DONE", done, &use),
	MENU("PRINT", to_printer, &use),
	MENU("FILE", to_file, &use),
	MENU("OVERLAY", do_warp, &use),
	MENU(delete_msg, delete_mark, &use),
	INFO("Transform->", &use),
	MENU(order_msg, get_order, &use),
	INFO(pick_msg, &use),
	OTHER(pick, &use),
	{0}
    };

    int color;
    int tsize;
    int cury;
    int len;
    int line;
    int top, bottom, left, right, width, middle, nums;

    /* to give user a response of some sort */
    Menu_msg("Preparing analysis ...");

    /*
     * build a popup window at center of the screen.
     * 35% the height and wide enough to hold the report
     *
     */

    /* height of 1 line, based on NLINES taking up 35% vertical space */
    height = (.35 * (SCREEN_BOTTOM - SCREEN_TOP)) / NLINES + 1;

    /* size of text, 80% of line height */
    tsize = .8 * height;
    size = tsize - 2;		/* fudge for computing pixels width of text */

    /* indent for the text */
    edge = .1 * height + 1;

    /* determine the length, in chars, of printed line */
    FMT0(buf, 0);
    nums = strlen(buf) * size;
    FMT1(buf, 0.0, 0.0, 0.0);
    len = strlen(buf);
    middle = len * size;
    FMT2(buf, 0.0, 0.0, 0.0, 0.0);
    len += strlen(buf);

    /* width is for max chars plus sidecar for more/less */
    width = len * size + nums + 2 * height;
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
    R_panel_save(tempfile1, top, bottom + 1, left, right + 1);


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
     * top two lines are for column labels
     * last two line is for overall rms error.
     */
    nlines = NLINES - 3;
    first_point = 0;

    /* allocate predicted values */
    xres = (double *)G_calloc(group.points.count, sizeof(double));
    yres = (double *)G_calloc(group.points.count, sizeof(double));
    gnd = (double *)G_calloc(group.points.count, sizeof(double));

    /* compute transformation for the first time */
    compute_transformation();

    /* put head on the report */
    cury = top;
    dotext(LHEAD1, cury, cury + height, left, middle, 0, BLACK);
    dotext(RHEAD1, cury, cury + height, middle, right - 1, 0, BLACK);
    cury += height;
    dotext(LHEAD2, cury, cury + height, left, middle, 0, BLACK);
    dotext(RHEAD2, cury, cury + height, middle, right - 1, 0, BLACK);
    cury += height;
    R_move_abs(left, cury - 1);
    R_cont_abs(right, cury - 1);

    /* isolate the sidecar */
    R_move_abs(right, top);
    R_cont_abs(right, bottom);

    /* define report box */
    report.top = cury;
    report.left = left;
    report.right = right;

    /* lets do it */

    pager = 1;
    while (1) {
	R_text_size(tsize, tsize);
	line = 0;
	curp = first_point;
	cury = top + 2 * height;
	while (1) {
	    if (line >= nlines || curp >= group.points.count)
		break;
	    line++;

	    if (!delete_mode)
		color = BLACK;
	    else
		color = BLUE;

	    if (group.equation_stat > 0 && group.points.status[curp] > 0) {
		/* color = BLACK; */
		FMT1(buf, xres[curp], yres[curp], gnd[curp]);
		if (curp == xmax || curp == ymax || curp == gmax)
		    color = RED;
		dotext(buf, cury, cury + height, nums, middle, 0, color);
	    }
	    else if (group.points.status[curp] > 0)
		dotext("?", cury, cury + height, nums, middle, 1, color);
	    else
		dotext("not used", cury, cury + height, nums, middle, 1,
		       color);

	    if (pager) {
		FMT0(buf, curp + 1);
		dotext(buf, cury, cury + height, left, nums, 0, color);
		FMT2(buf,
		     group.points.e1[curp],
		     group.points.n1[curp],
		     group.points.e2[curp], group.points.n2[curp]);
		dotext(buf, cury, cury + height, middle, right - 1, 0, color);
	    }
	    cury += height;
	    curp++;
	}
	report.bottom = cury;
	downarrow(&more, curp < group.points.count ? color : BACKGROUND);
	uparrow(&less, first_point > 0 ? color : BACKGROUND);
	R_standard_color(BACKGROUND);
	R_box_abs(left, cury, right - 1, bottom);
	if (group.equation_stat < 0) {

	    if (group.equation_stat == -1) {
		color = RED;
		strcpy(buf, "Poorly placed control points");
	    }
	    else {
		if (group.equation_stat == -2)
		    G_fatal_error("NOT ENOUGH MEMORY");
		else
		    G_fatal_error("PARAMETER ERROR");
	    }

	}
	else if (group.equation_stat == 0) {
	    color = RED;
	    strcpy(buf, "No active control points");
	}
	else {
	    color = BLACK;
	    sprintf(buf, "Overall rms error: %.2f", rms);
	}
	dotext(buf, bottom - height, bottom, left, right - 1, 0, color);
	R_standard_color(BLACK);
	R_move_abs(left, bottom - height);
	R_cont_abs(right - 1, bottom - height);

	pager = 0;
	which = -1;
	if (Input_pointer(objects) < 0)
	    break;
	display_points(1);
    }

    /* all done. restore what was under the window */
    right += 2 * height;	/* move it back over the sidecar */
    R_standard_color(BACKGROUND);
    R_box_abs(left, top, right, bottom);
    R_panel_restore(tempfile1);
    R_panel_delete(tempfile1);
    R_flush();

    G_free(xres);
    G_free(yres);
    G_free(gnd);
    I_put_control_points(group.name, &group.points);
    display_points(1);
    return 0;			/* return but don't QUIT */
}
