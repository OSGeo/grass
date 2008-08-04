#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include "global.h"
#include "proto.h"

/* --- DISLAY ---
 *  For all display functions the display driver must be opened first.
 *  Because some functions like erase() call other grass commands, driver is closed
 *  and reopened within these functions
 */

/* Display points */
void display_points(struct line_pnts *Points, int flsh)
{
    int i;

    G_debug(2, "display_points()");

    driver_line_width(var_geti(VAR_LINEWIDTH));
    for (i = 1; i < Points->n_points; i++) {
	G_plot_line(Points->x[i - 1], Points->y[i - 1], Points->x[i],
		    Points->y[i]);
    }
    driver_line_width(0);
}

/* Display icon */
void display_icon(double x, double y, int icon, double angle, int size,
		  int flsh)
{
    G_debug(2, "display_icon()");
#if 0
    driver_line_width(var_geti(VAR_LINEWIDTH));
    G_plot_icon(x, y, icon, angle, Scale * size);
    driver_line_width(0);
#else
    if (icon == G_ICON_CROSS && angle == 0)
	driver_plot_icon(x, y, "cross");
    else if (icon == G_ICON_CROSS)
	driver_plot_icon(x, y, "cross45");
    else if (icon == G_ICON_BOX)
	driver_plot_icon(x, y, "box");
#endif
}

/* Display vector line 
 *  color : code from SymbNumber 
 *
 *  ! This function doesn't check Symb[symb].on so that new digitized line is displayed even
 *    if its symbology is switched off (that means incorrect and user friendly)
 */
void display_line(int line, int color, int flsh)
{
    int type, symb;
    static struct line_pnts *Points;
    static struct line_cats *Cats;
    static int first = 1;

    G_debug(2, "display_line(): line = %d color = %d", line, color);

    if (first) {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
	first = 0;
    }

    if (!Vect_line_alive(&Map, line))
	return;

    type = Vect_read_line(&Map, Points, Cats, line);

    if (color == SYMB_DEFAULT)
	symb = LineSymb[line];
    else
	symb = color;

    symb_set_driver_color(symb);

    if (type & GV_POINTS)
	display_icon(Points->x[0], Points->y[0], G_ICON_CROSS, 0, 6, flsh);
    else
	display_points(Points, flsh);

}

/* Redraw updated lines */
void display_updated_lines(int symb)
{
    int i, line;

    for (i = 0; i < Vect_get_num_updated_lines(&Map); i++) {
	line = Vect_get_updated_line(&Map, i);
	if (!Vect_line_alive(&Map, line))
	    continue;
	display_line(line, symb, 0);
    }
}

/* Display node, color may be given but shape and size is read from symbology table,
 *  this is useful to delete (redraw by background) existing node
 *  
 *  color : code from SymbNumber 
 */
void display_node(int node, int color, int flsh)
{
    int symb;
    double x, y;

    G_debug(2, "display_node(): node = %d color = %d", node, color);

    if (!Vect_node_alive(&Map, node))
	return;

    if (color == SYMB_DEFAULT)
	symb = NodeSymb[node];
    else
	symb = color;

    symb_set_driver_color(symb);
    Vect_get_node_coor(&Map, node, &x, &y, NULL);
    display_icon(x, y, G_ICON_CROSS, 0.785, 6, flsh);
}

/* Redraw updated nodes */
void display_updated_nodes(int symb)
{
    int i, node;

    if (symb != SYMB_DEFAULT)
	symb_set_driver_color(symb);

    for (i = 0; i < Vect_get_num_updated_nodes(&Map); i++) {
	node = Vect_get_updated_node(&Map, i);
	if (!Vect_node_alive(&Map, node))
	    continue;
	if (NodeSymb[node] == SYMB_NODE_0)
	    continue;
	display_node(node, symb, 0);
    }
}

/* Display vector map */
void display_map(void)
{
    int i, n, symb;


    G_debug(2, "display_map()");


    /* Because after resize of monitor we expect manual call to display_map()
     *  it is good to refresh D_* here */
    driver_refresh();

    /* Lines */
    n = Vect_get_num_lines(&Map);
    symb_set_driver_color(SYMB_HIGHLIGHT);
    for (i = 1; i <= n; i++) {
	symb = LineSymb[i];
	G_debug(2, "symb = %d", symb);
	if (!Symb[symb].on)
	    continue;
	display_line(i, SYMB_DEFAULT, 0);
    }

    /* Nodes: first nodes with more than 1 line, then nodes with only 1 line, 
     *   so that dangles are not hidden, and nodes without lines (points, 
     *   centroids, are displayed) */
    n = Vect_get_num_nodes(&Map);

    if (Symb[SYMB_NODE_2].on) {
	symb_set_driver_color(SYMB_NODE_2);
	for (i = 1; i <= n; i++) {
	    if (!Vect_node_alive(&Map, i))
		continue;
	    if (NodeSymb[i] != SYMB_NODE_2)
		continue;
	    display_node(i, NodeSymb[i], 0);
	}
    }

    if (Symb[SYMB_NODE_1].on) {
	symb_set_driver_color(SYMB_NODE_1);
	for (i = 1; i <= n; i++) {
	    G_debug(2, "node = %d NodeSymb = %d", i, NodeSymb[i]);
	    if (!Vect_node_alive(&Map, i))
		continue;
	    if (NodeSymb[i] != SYMB_NODE_1)
		continue;
	    display_node(i, NodeSymb[i], 0);
	}
    }
}

/* Display bacground */
void display_bg(void)
{
    static char w_buf[] = "GRASS_WIDTH=0000000000";
    static char h_buf[] = "GRASS_HEIGHT=0000000000";
    static char img_buf[GPATH_MAX];
    static char cmd_buf[GPATH_MAX];
    char *ppmfile = G_tempfile();
    int i;

    G_debug(2, "display_bg()");

    putenv("GRASS_VERBOSE=0");
    putenv("GRASS_RENDER_IMMEDIATE=TRUE");
    putenv("GRASS_TRUECOLOR=TRUE");

    sprintf(cmd_buf, "GRASS_BACKGROUNDCOLOR=%02x%02x%02x",
	    Symb[SYMB_BACKGROUND].r, Symb[SYMB_BACKGROUND].g,
	    Symb[SYMB_BACKGROUND].b);
    putenv(cmd_buf);

    sprintf(img_buf, "GRASS_PNGFILE=%s.ppm", ppmfile);
    putenv(img_buf);

    sprintf(w_buf, "GRASS_WIDTH=%d", (int)(D_get_d_east() - D_get_d_west()));
    putenv(w_buf);

    sprintf(h_buf, "GRASS_HEIGHT=%d",
	    (int)(D_get_d_south() - D_get_d_north()));
    putenv(h_buf);

    for (i = 0; i < nbgcmd; i++) {
	putenv((i > 0) ? "GRASS_PNG_READ=TRUE" : "GRASS_PNG_READ=FALSE");
	if (Bgcmd[i].on)
	    system(Bgcmd[i].cmd);
    }

    sprintf(cmd_buf, "image create photo bgimage -file {%s.ppm}", ppmfile);
    Tcl_Eval(Toolbox, cmd_buf);

    sprintf(cmd_buf,
	    ".screen.canvas create image %d %d -image bgimage -anchor nw",
	    (int)D_a_to_d_col(0), (int)D_a_to_d_row(0));
    Tcl_Eval(Toolbox, cmd_buf);

    remove(ppmfile);
    G_free(ppmfile);
}

/* Erase */
void display_erase(void)
{
    driver_close();
    Tcl_Eval(Toolbox, ".screen.canvas delete all");
    driver_open();

    /* As erase must be run after each zoom by v.digit, here is good place to reset plot.
     *  Other such place is display_map() */
    driver_refresh();
}

/* Redraw */
void display_redraw(void)
{
    display_erase();
    display_bg();
    display_map();
}
