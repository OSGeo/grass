#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <grass/raster.h>
#include "globals.h"
#include "local_proto.h"

/* define MOUSE_YN to answer y/n by mouse click */
#define	MOUSE_YN

static int get_point2(double *, double *, double *);
static int keyboard(void);
static int _keyboard(void);
static int digitizer(void);
static int screen(int, int, int);
static int cancel(void);

int mark(int x, int y, int button)
{
    if (button == 2) {
	zoom_box1(x, y);
	return 0;
    }
    else if (button == 3) {
	zoom_point2(x, y, 1, 1.);
	return 0;
    }

    if (VIEW_MAP1->cell.configured && In_view(VIEW_MAP1, x, y))
	mark_point(VIEW_MAP1, x, y);
    else if (VIEW_MAP1_ZOOM->cell.configured && In_view(VIEW_MAP1_ZOOM, x, y))
	mark_point(VIEW_MAP1_ZOOM, x, y);

    return 0;			/* return but don't quit */
}

int mark_point(View * view, int x, int y)
{
    double e0, n0;
    double e1, n1, z1;
    double e2, n2, z2;
    double ee1, nn1;
    int row, col;
    char buf[100];


    /* convert x,y to east,north at center of cell */
    col = view_to_col(view, x);
    e0 = col_to_easting(&view->cell.head, col, 0.5);
    row = view_to_row(view, y);
    n0 = row_to_northing(&view->cell.head, row, 0.5);

    /*  These are image coordinates not photo coordinates */
    ee1 = e0;
    nn1 = n0;

    /*  e1, n1 now become photo coordinates */
    I_georef(e0, n0, &e1, &n1, group.E12, group.N12);
    z1 = -group.camera_ref.CFL;

    Curses_clear_window(MENU_WINDOW);
    sprintf(buf, "Point %d marked at IMAGE COORDINATES:",
	    group.control_points.count + 1);
    Curses_write_window(MENU_WINDOW, 1, 1, buf);
    sprintf(buf, "X:   %10.2f", ee1);
    Curses_write_window(MENU_WINDOW, 3, 3, buf);
    sprintf(buf, "Y:  %10.2f", nn1);
    Curses_write_window(MENU_WINDOW, 4, 3, buf);
    Curses_clear_window(INFO_WINDOW);

    R_standard_color(ORANGE);
    save_under_dot(x, y, tempfile_dot);
    dot(x, y);

    if (!get_point2(&e2, &n2, &z2)) {
	Curses_clear_window(MENU_WINDOW);
	Curses_clear_window(INFO_WINDOW);
	restore_under_dot(tempfile_dot);
    }
    else {
	Curses_write_window(MENU_WINDOW, 7, 1, "Target Point location:");
	sprintf(buf, "East:      %10.2f", e2);
	Curses_write_window(MENU_WINDOW, 8, 3, buf);
	sprintf(buf, "North:     %10.2f", n2);
	Curses_write_window(MENU_WINDOW, 9, 3, buf);
	if (G_is_d_null_value(&z2))
	    sprintf(buf, "Elevation:       NULL");
	else
	    sprintf(buf, "Elevation: %10.2f", z2);
	Curses_write_window(MENU_WINDOW, 10, 3, buf);

	I_new_con_point(&group.control_points, ee1, nn1, z1, e2, n2, z2, 1);
	I_new_con_point((struct Ortho_Control_Points *)&group.photo_points,
			e1, n1, z1, e2, n2, z2, 1);

	I_put_con_points(group.name, &group.control_points);

	sprintf(buf, "Computing equations ...");
	Curses_write_window(MENU_WINDOW, 13, 1, buf);
	Compute_ortho_equation();
	display_conz_points(1);
	Curses_clear_window(MENU_WINDOW);
	Curses_clear_window(INFO_WINDOW);
    }
    release_under_dot(tempfile_dot);

    return 0;
}

static double N, E, Z;

static int get_point2(double *east, double *north, double *elev)
{
    int stat;
    static int use = 1;
    static Objects objects[] = {
	MENU("CANCEL", cancel, &use),
	INFO("Mark control point on target image", &use),
	OTHER(screen, &use),
	{0}
    };


    if (from_digitizer > 0)
	stat = Input_other(digitizer, "Digitizer") > 0;
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
	if (G_is_d_null_value(&Z))
	    G_set_d_null_value(elev, 1);
	else
	    *elev = Z;
    }

    return stat;
}

static int keyboard(void)
{
    int ok;

    ok = _keyboard();
    return ok;
}

static int _keyboard(void)
{
    char buf[100];

    while (1) {
	Curses_clear_window(INFO_WINDOW);
	Curses_prompt_gets
	    ("Enter CONTROL COORDINATES as east north elevation: ", buf);
	G_strip(buf);
	if (*buf == 0) {
	    return 0;
	}
	if (sscanf(buf, "%lf %lf %lf", &E, &N, &Z) != 3) {
	    Beep();
	    continue;
	}
	Curses_clear_window(INFO_WINDOW);
	sprintf(buf, "East:      %f\n", E);
	Curses_write_window(INFO_WINDOW, 2, 2, buf);
	sprintf(buf, "North:     %f\n", N);
	Curses_write_window(INFO_WINDOW, 3, 2, buf);
	if (G_is_d_null_value(&Z))
	    sprintf(buf, "Elevation:       NULL");
	else
	    sprintf(buf, "Elevation: %f\n", Z);
	Curses_write_window(INFO_WINDOW, 4, 2, buf);
#ifdef	MOUSE_YN
	Curses_write_window(INFO_WINDOW, 6, 1,
			    "Look ok? (Left: y / Right: n) ");
#else
	Curses_write_window(INFO_WINDOW, 6, 1, "Look ok? (y/n) ");
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
    /*    return 0;    dont get here */
}

static int digitizer(void)
{
    int ok;
    char buf[100];

    ok = digitizer_point(&E, &N);
    if (ok) {
	if (!get_z_from_cell(N, E))
	    return 0;

	Curses_clear_window(INFO_WINDOW);
	sprintf(buf, "East:      %f\n", E);
	Curses_write_window(INFO_WINDOW, 3, 2, buf);
	sprintf(buf, "North:     %f\n", N);
	Curses_write_window(INFO_WINDOW, 4, 2, buf);
	if (G_is_d_null_value(&Z))
	    sprintf(buf, "Elevation:       NULL");
	else
	    sprintf(buf, "Elevation: %f\n", Z);
	Curses_write_window(INFO_WINDOW, 5, 2, buf);
#ifdef	MOUSE_YN
	Curses_write_window(INFO_WINDOW, 7, 1,
			    "Look ok? (Left: y / Right: n) ");
#else
	Curses_write_window(INFO_WINDOW, 7, 1, "Look ok? (y/n) ");
#endif

	while (1) {
#ifdef	MOUSE_YN
	    int x, y, b;

	    R_get_location_with_pointer(&x, &y, &b);
	    if (b == 1) {
		ok = 1;
		break;
	    }
	    else if (b == 3) {
		ok = -1;
		break;
	    }
#else
	    c = Curses_getch(0);
	    if (c == 'y' || c == 'Y') {
		ok = 1;
		break;
	    }
	    if (c == 'n' || c == 'N') {
		ok = -1;
		break;
	    }
#endif
	    Beep();
	}
	Curses_clear_window(INFO_WINDOW);
	return ok;
    }
    return 0;
}

static int screen(int x, int y, int button)
{
    int row, col, ok;
    char buf[50];
    View *view;

    if (button == 3)		/* cancel */
	return -1;

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

    if (!get_z_from_cell(N, E))
	return 0;

    Curses_clear_window(INFO_WINDOW);
    sprintf(buf, "East:      %10.2f\n", E);
    Curses_write_window(INFO_WINDOW, 3, 2, buf);
    sprintf(buf, "North:     %10.2f\n", N);
    Curses_write_window(INFO_WINDOW, 4, 2, buf);
    if (G_is_d_null_value(&Z))
	sprintf(buf, "Elevation:       NULL");
    else
	sprintf(buf, "Elevation: %10.2f\n", Z);
    Curses_write_window(INFO_WINDOW, 5, 2, buf);
#ifdef	MOUSE_YN
    Curses_write_window(INFO_WINDOW, 7, 1, "Look ok? (Left: y / Right: n) ");
#else
    Curses_write_window(INFO_WINDOW, 7, 1, "Look ok? (y/n) ");
#endif

    R_standard_color(ORANGE);
    save_under_dot(x, y, tempfile_dot2);
    dot(x, y);
    R_flush();

    while (1) {
#ifdef	MOUSE_YN
	int x, y, b;

	R_get_location_with_pointer(&x, &y, &b);
	if (b == 1) {
	    ok = 1;
	    break;
	}
	else if (b == 3) {
	    restore_under_dot(tempfile_dot2);
	    ok = 0;
	    break;
	}
#else
	int c;

	c = Curses_getch(0);
	if (c == 'y' || c == 'Y') {
	    ok = 1;
	    break;
	}
	if (c == 'n' || c == 'N') {
	    ok = -1;
	    break;
	}
#endif
	Beep();
    }
    Curses_clear_window(INFO_WINDOW);

    return ok;
}

/* Get height from raster 
 * return 1 if coors are in raster or cannot open rster
 *        0 if coors are outside raster
 */
int get_z_from_cell2(double north, double east, double *height)
{
    int row, col;
    struct Cell_head elevhd;
    RASTER_MAP_TYPE data_type;

    G_set_d_null_value(height, 1);

    /* allocate the elev buffer */
    select_target_env();
    G_get_cellhd(elev_layer, mapset_elev, &elevhd);
    G_set_window(&elevhd);

    elev = G_open_cell_old(elev_layer, mapset_elev);
    if (elev < 0)
	return 0;
    data_type = G_get_raster_map_type(elev);

    elevbuf = G_allocate_raster_buf(data_type);

    /* find row, col in elevation raster map */
    row = (int)northing_to_row(&elevhd, north);
    col = (int)easting_to_col(&elevhd, east);

    if (row < 0 || row >= elevhd.rows || col < 0 || col >= elevhd.cols) {
	G_close_cell(elev);
	G_free(elevbuf);
	return 0;
    }

    if (G_get_raster_row(elev, elevbuf, row, data_type) <= 0) {
	G_close_cell(elev);
	G_free(elevbuf);
	return 0;
    }

    if ((data_type == CELL_TYPE &&
	 !G_is_c_null_value((CELL *) & ((CELL *) elevbuf)[col])) ||
	(data_type == FCELL_TYPE &&
	 !G_is_f_null_value((FCELL *) & ((FCELL *) elevbuf)[col])) ||
	(data_type == DCELL_TYPE &&
	 !G_is_d_null_value((DCELL *) & ((DCELL *) elevbuf)[col]))) {
	if (data_type == CELL_TYPE)
	    *height = (double)((CELL *) elevbuf)[col];
	else if (data_type == FCELL_TYPE)
	    *height = (double)((FCELL *) elevbuf)[col];
	else if (data_type == DCELL_TYPE)
	    *height = (double)((DCELL *) elevbuf)[col];
    }

    G_close_cell(elev);
    G_free(elevbuf);
    select_current_env();
    return (1);
}

int get_z_from_cell(double north, double east)
{
    char buf[100];

    if (!get_z_from_cell2(north, east, &Z)) {
	Curses_write_window(INFO_WINDOW, 5, 1, "point not on elevation map");
	Curses_write_window(INFO_WINDOW, 6, 1, "no elevation data available");
	Beep();
	G_sleep(3);
	Curses_clear_window(INFO_WINDOW);

	while (1) {
	    Curses_prompt_gets
		("Enter elevation value (hit return if not known): ", buf);
	    Curses_clear_window(PROMPT_WINDOW);
	    G_strip(buf);
	    if (*buf == 0)
		return 0;
	    if (sscanf(buf, "%lf ", &Z) == 1)
		return (1);
	    Beep();
	}
    }
    else {
	return (1);
    }
    return 0;
}

static int cancel(void)
{
    return -1;
}
