#include "globals.h"
#include <grass/raster.h>

static int first = 1;
static int curx, cury;

int Mouse_pointer(int *x, int *y, int *button)
{
    if (first) {
	curx = (SCREEN_LEFT + SCREEN_RIGHT) / 2;
	cury = (SCREEN_TOP + SCREEN_BOTTOM) / 2;
	first = 0;
    }
    R_get_location_with_pointer(&curx, &cury, button);
    *x = curx;
    *y = cury;

#ifdef BUTTON3
    if (*button == 3)
	quit();
#endif
    return 0;
}

int Mouse_line_anchored(int x1, int y1, int *x2, int *y2, int *button)
{
    R_get_location_with_line(x1, y1, x2, y2, button);
    curx = *x2;
    cury = *y2;
    first = 0;

#ifdef BUTTON3
    if (*button == 3)
	quit();
#endif
    return 0;
}

int Mouse_box_anchored(int x1, int y1, int *x2, int *y2, int *button)
{
    R_get_location_with_box(x1, y1, x2, y2, button);
    curx = *x2;
    cury = *y2;
    first = 0;

#ifdef BUTTON3
    if (*button == 3)
	quit();
#endif
    return 0;
}

int Get_mouse_xy(int *x, int *y)
{
    *x = curx;
    *y = cury;
    return 0;
}

int Set_mouse_xy(int x, int y)
{
    first = 0;
    curx = x;
    cury = y;
    return 0;
}
