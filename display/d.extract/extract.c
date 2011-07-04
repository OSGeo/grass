#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#define WDTH 5

#define M_START 1
#define M_ADD   2
#define M_DEL   3
#define M_END   4

int display(struct Map_info *Map, struct boxlist *List,
	    const struct color_rgb *color);

int extract(struct Map_info *In, struct Map_info *Out, int type,
	    const struct color_rgb *color, const struct color_rgb *hcolor)
{
    int i, button, mode, line;
    int screen_x, screen_y, cur_screen_x, cur_screen_y;
    double x1, y1, x2, y2;
    struct boxlist *List, *CList;
    struct bound_box box;
    struct line_pnts *Points;
    struct line_cats *Cats;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    List = Vect_new_boxlist(0);
    CList = Vect_new_boxlist(0);

    /* box.T = PORT_DOUBLE_MAX;
       box.B = -PORT_DOUBLE_MAX; */
    Vect_get_map_box(In, &box);

    mode = M_START;
    G_message(_("Select vector(s) with mouse"));
    G_message(_(" - L: draw box with left mouse button to select"));
    G_message(_(" - M: draw box with middle mouse button to remove from display"));
    G_message(_(" - R: quit and save selected vectors to new map\n"));
    while (1) {
	G_message(_("L: add  M: remove  R: quit and save\n"));

	if (mode == M_START) {
	    R_get_location_with_pointer(&screen_x, &screen_y, &button);
	    cur_screen_x = screen_x;
	    cur_screen_y = screen_y;
	}
	else {
	    R_get_location_with_box(cur_screen_x, cur_screen_y, &screen_x,
				    &screen_y, &button);
	    x1 = D_d_to_u_col((double)(cur_screen_x));
	    y1 = D_d_to_u_row((double)(cur_screen_y));
	    x2 = D_d_to_u_col((double)(screen_x));
	    y2 = D_d_to_u_row((double)(screen_y));

	    if (x1 < x2) {
		box.W = x1;
		box.E = x2;
	    }
	    else {
		box.W = x2;
		box.E = x1;
	    }
	    if (y1 < y2) {
		box.S = y1;
		box.N = y2;
	    }
	    else {
		box.S = y2;
		box.N = y1;
	    }
	    G_debug(1, "Box: N S E W = %f %f %f %f\n", box.N, box.S, box.E,
		    box.W);
	}

	/* TODO: check if line really intersects box, not only box intersects box */
	switch (button) {
	case 1:
	    if (mode == M_START) {
		mode = M_ADD;
	    }
	    else if (mode == M_ADD) {
		Vect_select_lines_by_box(In, &box, type, CList);
		Vect_boxlist_append_boxlist(List, CList);
		display(In, List, hcolor);
		mode = M_START;
	    }
	    break;
	case 2:
	    if (mode == M_START) {
		mode = M_DEL;
	    }
	    else if (mode == M_DEL) {
		Vect_select_lines_by_box(In, &box, type, CList);
		Vect_boxlist_delete_boxlist(List, CList);
		display(In, CList, color);
		mode = M_START;
	    }
	    break;
	case 3:
	    for (i = 0; i < List->n_values; i++) {
		line = List->value[i];
		type = Vect_read_line(In, Points, Cats, line);
		Vect_write_line(Out, type, Points, Cats);
	    }
	    display(In, List, color);

	    return 1;
	    break;
	}
    };

    Vect_destroy_boxlist(List);
    Vect_destroy_boxlist(CList);

    return 1;
}

int
display(struct Map_info *Map, struct boxlist *List,
	const struct color_rgb *color)
{
    int i, j, line, type;
    struct line_pnts *Points;
    double msize;

    msize = 10 * (D_d_to_u_col(2) - D_d_to_u_col(1));	/* do it better */
    G_debug(1, "msize = %f\n", msize);

    Points = Vect_new_line_struct();
    D_RGB_color(color->r, color->g, color->b);

    for (i = 0; i < List->n_values; i++) {
	line = abs(List->id[i]);
	type = Vect_read_line(Map, Points, NULL, line);

	if (type & GV_POINTS)
	    D_plot_icon(Points->x[0], Points->y[0], G_ICON_CROSS, 0.0, msize);
	else
	    for (j = 0; j < Points->n_points - 1; j++) {
		D_move(Points->x[j], Points->y[j]);
		D_cont(Points->x[j + 1], Points->y[j + 1]);
	    }
    }

    R_flush();

    Vect_destroy_line_struct(Points);

    return 0;
}
