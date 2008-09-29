#include "globals.h"

static int where_am_i(View *, int, int, Window *, double *, double *,
		      Window *, int);
static int where_12(View *, int, int);
static int where_21(View *, int, int);

int where(int x, int y)
{
    if (VIEW_MAP1->cell.configured && In_view(VIEW_MAP1, x, y))
	where_12(VIEW_MAP1, x, y);
    else if (VIEW_MAP1_ZOOM->cell.configured && In_view(VIEW_MAP1_ZOOM, x, y))
	where_12(VIEW_MAP1_ZOOM, x, y);
    else if (VIEW_MAP2->cell.configured && In_view(VIEW_MAP2, x, y))
	where_21(VIEW_MAP2, x, y);
    else if (VIEW_MAP2_ZOOM->cell.configured && In_view(VIEW_MAP2_ZOOM, x, y))
	where_21(VIEW_MAP2_ZOOM, x, y);
    return 0;			/* return but don't quit */
}

static int where_12(View * view, int x, int y)
{
    where_am_i(view, x, y, MENU_WINDOW, group.E12, group.N12, INFO_WINDOW, 1);
    return 0;
}

static int where_21(View * view, int x, int y)
{
    where_am_i(view, x, y, INFO_WINDOW, group.E21, group.N21, MENU_WINDOW, 2);
    return 0;
}

static int where_am_i(View * view, int x, int y, Window * w1,
		      double *E, double *N, Window * w2, int windnum)
{
    double e1, n1, e2, n2;
    int row, col;

    char buf[100];

    /* convert x,y to east,north at center of cell */
    col = view_to_col(view, x);
    e1 = col_to_easting(&view->cell.head, col, 0.5);
    row = view_to_row(view, y);
    n1 = row_to_northing(&view->cell.head, row, 0.5);

    /* Curses_clear_window (w1); */
    if (windnum == 2)
	Curses_write_window(INFO_WINDOW, 15, 2, "WHERE CURSOR             ");
    else
	Curses_write_window(INFO_WINDOW, 15, 2, "WHERE CELL               ");
    sprintf(buf, "E = %10.2f", e1);
    Curses_write_window(INFO_WINDOW, 17, 3, buf);
    sprintf(buf, "N = %10.2f", n1);
    Curses_write_window(INFO_WINDOW, 18, 3, buf);

    /* if transformation equation is useable, determine point via equation */
    if (group.equation_stat <= 0 || windnum == 2) {
	Curses_write_window(INFO_WINDOW, 15, 18, "           ");
	Curses_write_window(INFO_WINDOW, 17, 18, "                 ");
	Curses_write_window(INFO_WINDOW, 18, 18, "                 ");
	return 1;
    }

    I_georef(e1, n1, &e2, &n2, E, N);
    /* Curses_clear_window (w2); */
    Curses_write_window(INFO_WINDOW, 15, 18, "COORDINATES");
    sprintf(buf, "E = %10.2f", e2);
    Curses_write_window(INFO_WINDOW, 17, 18, buf);
    sprintf(buf, "N = %10.2f", n2);
    Curses_write_window(INFO_WINDOW, 18, 18, buf);
    return 0;
}
