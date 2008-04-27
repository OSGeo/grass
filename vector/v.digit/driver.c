#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include "global.h"
#include "proto.h"

static char color[16];
static int width;

void driver_rgb_color(int r, int g, int b)
{
    sprintf(color, "#%02x%02x%02x", r, g, b);
}

void driver_line_width(int w)
{
    width = w ? w : 1;
}

static int curx, cury;

static int driver_move_abs(int x, int y)
{
    curx = x;
    cury = y;
    return 0;
}

static int driver_cont_abs(int x, int y)
{
    char buf[1024];

    sprintf(buf, ".screen.canvas create line %d %d %d %d -width %d -fill %s",
	    curx, cury, x, y, width, color);
    Tcl_Eval(Toolbox, buf);

    curx = x;
    cury = y;
    return 0;
}

void driver_plot_icon(double x, double y, const char *icon)
{
    char buf[1024];
    int xi, yi;

    G_plot_where_xy(x, y, &xi, &yi);

    sprintf(buf, ".screen.canvas create bitmap %d %d -bitmap @$vdpath/%s.xbm -foreground %s -anchor center",
	    xi, yi, icon, color);
    if (Tcl_Eval(Toolbox, buf) != TCL_OK)
	G_warning("driver_plot_icon: %s", Toolbox->result);
}

static void get_window(int *t, int *b, int *l, int *r)
{
    Tcl_Eval(Toolbox, "list 0 [winfo height .screen.canvas] 0 [winfo width .screen.canvas]");
    sscanf(Toolbox->result, "%d %d %d %d", t, b, l, r);

    if (*b > 1 || *r > 1)
	    return;

    Tcl_Eval(Toolbox, "list 0 [.screen.canvas cget -height] 0 [.screen.canvas cget -width]");
    sscanf(Toolbox->result, "%d %d %d %d", t, b, l, r);
}

static void setup(void)
{
    struct Cell_head region;
    int t, b, l, r;

    get_window(&t, &b, &l, &r);

    /* Set the map region associated with graphics frame */
    G_get_set_window(&region);
    if(G_set_window(&region) < 0)
	G_fatal_error ("Invalid graphics coordinates");

    /* Determine conversion factors */
    if (D_do_conversions(&region, t, b, l, r))
	G_fatal_error("Error calculating graphics-region conversions") ;
}

int driver_refresh (void)
{
    setup();
    G_setup_plot (D_get_d_north(), D_get_d_south(), D_get_d_west(), D_get_d_east(),
		  driver_move_abs, driver_cont_abs);
    return 1;
}
   
int driver_open (void)
{
    double n, s, e, w;
    
    if (Tcl_Eval(Toolbox, "create_screen") != TCL_OK)
	G_warning("create_screen: %s", Toolbox->result);
    

    setup();

    n = D_d_to_u_row(D_get_d_north());
    s = D_d_to_u_row(D_get_d_south());
    w = D_d_to_u_col(D_get_d_west());
    e = D_d_to_u_col(D_get_d_east());

    Scale = (n - s) / ( D_get_d_south() - D_get_d_north() );
    
    G_setup_plot (D_get_d_north(), D_get_d_south(), D_get_d_west(), D_get_d_east(),
		  driver_move_abs, driver_cont_abs);
    return 1;
}

int driver_close (void)
{
    return 1;
}

