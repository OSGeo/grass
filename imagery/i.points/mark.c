#include "globals.h"
#include "local_proto.h"
#include <grass/raster.h>

static int get_point2(double *, double *);
static int keyboard(void);
static int _keyboard(void);
static int screen(int, int, int);
static int digitizer(void);
static int cancel(void);

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
    sprintf(buf, "Point %d marked on image at", group.points.count + 1);
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
	Curses_write_window(MENU_WINDOW, 7, 1, "Point located at");
	sprintf(buf, "East:  %10.2f", e2);
	Curses_write_window(MENU_WINDOW, 9, 3, buf);
	sprintf(buf, "North: %10.2f", n2);
	Curses_write_window(MENU_WINDOW, 10, 3, buf);
	I_new_control_point(&group.points, e1, n1, e2, n2, 1);
	I_put_control_points(group.name, &group.points);
	Compute_equation();
	display_points(1);
    }
    release_under_dot();

    return 0;
}

static double N, E;

static int get_point2(double *east, double *north)
{
    int stat;
    static int use = 1;
    static Objects objects[] = {
	MENU("CANCEL", cancel, &use),
	INFO("Mark point on target image", &use),
	OTHER(screen, &use),
	{0}
    };

    if (from_digitizer > 0) {
	stat = Input_other(digitizer, "Digitizer");
    }
    else if (from_screen > 0) {
	set_colors(&VIEW_MAP2->cell.colors);
	stat = Input_pointer(objects) > 0;
	set_colors(&VIEW_MAP1->cell.colors);
    }
    else
	stat = Input_other(keyboard, "Keyboard");

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
    char buf[100], buf1[100], buf2[100];

    while (1) {
	Curses_prompt_gets("Enter coordinates as east north: ", buf);
	G_strip(buf);
	if (*buf == 0) {
	    return 0;
	}

	if (sscanf(buf, "%s %s", buf1, buf2) != 2) {
	    Beep();
	    continue;
	}
	/* scan for lat/lon string first as "123E 45S" passes the %lf test but is wrong */
	if (!(G_lon_scan(buf1, &E) && G_lat_scan(buf2, &N))) {
	    if (sscanf(buf, "%lf %lf", &E, &N) != 2) {
		Beep();
		continue;
	    }
	}

	Curses_clear_window(INFO_WINDOW);
	sprintf(buf, "East:   %f\n", E);
	Curses_write_window(INFO_WINDOW, 2, 2, buf);
	sprintf(buf, "North:  %f\n", N);
	Curses_write_window(INFO_WINDOW, 3, 2, buf);
	Curses_write_window(INFO_WINDOW, 5, 1, "Look ok? (y/n) ");

	while (1) {
	    int c;

	    c = Curses_getch(0);
	    if (c == 'y' || c == 'Y')
		return 1;
	    if (c == 'n' || c == 'N')
		break;
	    Beep();
	}
    }

    return 0;
}

static int digitizer(void)
{
    return digitizer_point(&E, &N);
}


static int screen(int x, int y, int button)
{
    int row, col;
    char buf[50];

    View *view;

    if (In_view(VIEW_MAP2, x, y) && VIEW_MAP2->cell.configured)
	view = VIEW_MAP2;
    else if (In_view(VIEW_MAP2_ZOOM, x, y) && VIEW_MAP2_ZOOM->cell.configured)
	view = VIEW_MAP2_ZOOM;
    else
	return 0;		/* ignore mouse event */

    col = view_to_col(view, x);
    E = col_to_easting(&view->cell.head, col, 0.5);
    row = view_to_row(view, y);
    N = row_to_northing(&view->cell.head, row, 0.5);

    if (button == 1)
	return 1;

    sprintf(buf, "East:   %10.2f\n", E);
    Curses_write_window(INFO_WINDOW, 2, 2, buf);
    sprintf(buf, "North:  %10.2f\n", N);
    Curses_write_window(INFO_WINDOW, 3, 2, buf);

    return 0;
}

static int cancel(void)
{
    return -1;
}
