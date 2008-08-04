#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include "global.h"
#include "proto.h"

/* Zoom - select new window interactively in the monitor */
struct zoom_window
{
    int mode;			/* 1 - first corner; 2 - first or second corner */
    int next_mode;
    int sxo, syo;
};

int zoom_window_begin(void *closure)
{
    struct zoom_window *zw = closure;

    G_debug(2, "zoom_window()");

    i_prompt("Zoom by window");
    i_prompt_buttons("1. corner", "1. corner", "Quit");

    zw->mode = 1;
    zw->next_mode = 1;
    zw->sxo = 0;
    zw->syo = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int zoom_window_update(void *closure, int sxn, int syn, int button)
{
    struct zoom_window *zw = closure;

    if (zw->mode == 1) {
	i_prompt_buttons("1. corner", "2. corner", "Quit");
	zw->next_mode = 2;
    }

    G_debug(2, "button = %d x = %d y = %d", button, sxn, syn);

    if (button == 3)
	return 1;

    if (zw->mode == 2 && button == 2) {
	double x1 = D_d_to_u_col(zw->sxo);
	double y1 = D_d_to_u_row(zw->syo);
	double x2 = D_d_to_u_col(sxn);
	double y2 = D_d_to_u_row(syn);

	G_debug(2, "x1 = %f x2 = %f y1 = %f y2 = %f", x1, x2, y1, y2);

	window.north = y1 > y2 ? y1 : y2;
	window.south = y1 < y2 ? y1 : y2;
	window.west = x1 < x2 ? x1 : x2;
	window.east = x1 > x2 ? x1 : x2;

	G_debug(2, "w = %f e = %f n = %f s = %f", window.west, window.east,
		window.north, window.south);

	G_adjust_Cell_head(&window, 0, 0);
	G_put_window(&window);
	G_set_window(&window);

	display_redraw();

	i_prompt_buttons("1. corner", "1. corner", "Quit");
	zw->next_mode = 1;
    }

    zw->sxo = sxn;
    zw->syo = syn;
    zw->mode = zw->next_mode;

    set_mode(zw->mode == 2 ? MOUSE_BOX : MOUSE_POINT);
    set_location(zw->sxo, zw->syo);

    return 0;
}

int zoom_window_end(void *closure)
{
    i_prompt("");
    i_prompt_buttons("", "", "");
    i_coor(COOR_NULL, COOR_NULL);

    G_debug(3, "zoom_window(): End");

    return 1;
}

void zoom_window(void)
{
    static struct zoom_window zw;

    set_tool(zoom_window_begin, zoom_window_update, zoom_window_end, &zw);
}

/* Zoom - in / out (centre unchanged) */
int zoom_centre(double factor)
{
    double xc, yc, dx, dy;

    G_debug(2, "zoom_centre()");

    driver_open();

    G_debug(2, "1 n = %f s = %f", window.north, window.south);

    dx = (window.east - window.west) / 2;
    dy = (window.north - window.south) / 2;
    xc = (window.east + window.west) / 2;
    yc = (window.north + window.south) / 2;

    G_debug(2, "  yc = %f dy = %f", yc, dy);

    window.north = yc + dy * factor;
    window.south = yc - dy * factor;
    window.east = xc + dx * factor;
    window.west = xc - dx * factor;


    G_debug(2, "2 n = %f s = %f", window.north, window.south);
    G_adjust_Cell_head(&window, 0, 0);
    G_debug(2, "3 n = %f s = %f", window.north, window.south);
    G_put_window(&window);
    G_set_window(&window);

    display_redraw();

    driver_close();

    G_debug(3, "zoom_centre(): End");

    return 1;
}

/* Zoom - pan */

struct zoom_pan
{
    int dummy;			/* zoom_pan is stateless */
};

int zoom_pan_begin(void *closure)
{
    G_debug(2, "zoom_pan()");

    i_prompt("Pan");
    i_prompt_buttons("New center", "", "Quit");

    set_mode(MOUSE_POINT);

    return 0;
}

int zoom_pan_update(void *closure, int sxn, int syn, int button)
{
    G_debug(2, "button = %d x = %d y = %d", button, sxn, syn);

    if (button == 3)
	return 1;

    if (button == 1) {
	double x = D_d_to_u_col(sxn);
	double y = D_d_to_u_row(syn);
	double dx = (window.east - window.west) / 2;
	double dy = (window.north - window.south) / 2;

	window.north = y + dy;
	window.south = y - dy;
	window.east = x + dx;
	window.west = x - dx;

	G_debug(2, "w = %f e = %f n = %f s = %f", window.west, window.east,
		window.north, window.south);
	G_adjust_Cell_head(&window, 0, 0);
	G_put_window(&window);
	G_set_window(&window);

	display_redraw();
    }

    return 0;
}

int zoom_pan_end(void *closure)
{
    i_prompt("");
    i_prompt_buttons("", "", "");
    i_coor(COOR_NULL, COOR_NULL);

    G_debug(3, "zoom_pan(): End");

    return 1;
}

void zoom_pan(void)
{
    static struct zoom_pan zp;

    set_tool(zoom_pan_begin, zoom_pan_update, zoom_pan_end, &zp);
}

/* Zoom - default region */
int zoom_default(void)
{
    struct Cell_head defwin;

    G_debug(2, "zoom_default()");

    driver_open();

    G_get_default_window(&defwin);
    G_put_window(&defwin);
    G_set_window(&defwin);

    display_redraw();

    driver_close();

    G_debug(3, "zoom_default(): End");

    return 1;
}

/* Zoom - to region */
int zoom_region(void)
{
    struct Cell_head win;
    char *mapset;

    G_debug(2, "zoom_region()");

    driver_open();

    mapset = G_find_file2("windows", var_getc(VAR_ZOOM_REGION), NULL);
    if (mapset == NULL) {
	G_warning("Cannot find window '%s'", var_getc(VAR_ZOOM_REGION));
	return 0;
    }
    G__get_window(&win, "windows", var_getc(VAR_ZOOM_REGION), mapset);
    G_put_window(&win);
    G_set_window(&win);

    display_redraw();

    driver_close();

    G_debug(3, "zoom_region(): End");

    return 1;
}
