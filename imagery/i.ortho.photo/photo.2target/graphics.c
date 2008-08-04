#include <grass/raster.h>
#include <grass/display.h>
#include "globals.h"
#include "local_proto.h"

static View *makeview(double bottom, double top, double left, double right)
{
    View *view;

    view = (View *) G_malloc(sizeof(View));

    top = 100 - top;
    bottom = 100 - bottom;

    view->top = SCREEN_TOP + (SCREEN_BOTTOM - SCREEN_TOP) * top / 100.0;
    view->bottom = SCREEN_TOP + (SCREEN_BOTTOM - SCREEN_TOP) * bottom / 100.0;
    view->left = SCREEN_LEFT + (SCREEN_RIGHT - SCREEN_LEFT) * left / 100.0;
    view->right = SCREEN_LEFT + (SCREEN_RIGHT - SCREEN_LEFT) * right / 100.0;

    if (view->top < SCREEN_TOP)
	view->top = SCREEN_TOP;
    if (view->bottom > SCREEN_BOTTOM)
	view->bottom = SCREEN_BOTTOM;
    if (view->left < SCREEN_LEFT)
	view->left = SCREEN_LEFT;
    if (view->right > SCREEN_RIGHT)
	view->right = SCREEN_RIGHT;

    Outline_box(view->top, view->bottom, view->left, view->right);

    view->top++;
    view->bottom--;
    view->left++;
    view->right--;

    view->nrows = view->bottom - view->top + 1;
    view->ncols = view->right - view->left + 1;
    view->cell.configured = 0;

    return view;
}

int Init_graphics(void)
{
    /* Dclearscreen(); */
    D_full_screen();

    SCREEN_TOP = R_screen_top();
    SCREEN_BOTTOM = R_screen_bot();
    SCREEN_LEFT = R_screen_left();
    SCREEN_RIGHT = R_screen_rite();


    BLACK = D_translate_color("black");
    BLUE = D_translate_color("blue");
    BROWN = D_translate_color("brown");
    GREEN = D_translate_color("green");
    GREY = D_translate_color("grey");
    ORANGE = D_translate_color("orange");
    PURPLE = D_translate_color("purple");
    RED = D_translate_color("red");
    WHITE = D_translate_color("white");
    YELLOW = D_translate_color("yellow");

    R_standard_color(WHITE);

    VIEW_TITLE1 = makeview(97.5, 100.0, 0.0, 50.0);
    VIEW_TITLE2 = makeview(97.5, 100.0, 50.0, 100.0);
    VIEW_MAP1 = makeview(51.0, 97.5, 0.0, 50.0);
    VIEW_MAP2 = makeview(51.0, 97.5, 50.0, 100.0);
    VIEW_TITLE1_ZOOM = makeview(47.5, 51.0, 0.0, 50.0);
    VIEW_TITLE2_ZOOM = makeview(47.5, 51.0, 50.0, 100.0);
    VIEW_MAP1_ZOOM = makeview(2.5, 47.5, 0.0, 50.0);
    VIEW_MAP2_ZOOM = makeview(2.5, 47.5, 50.0, 100.0);
    VIEW_MENU = makeview(0.0, 2.5, 0.0, 100.0);

    /* suppress color allocation warnings */
    G_suppress_warnings(1);
    G_init_colors(&VIEW_MAP1->cell.colors);
    G_init_colors(&VIEW_MAP2->cell.colors);
    G_suppress_warnings(0);

    return 0;
}

int Outline_box(int top, int bottom, int left, int right)
{
    R_move_abs(left, top);
    R_cont_abs(left, bottom);
    R_cont_abs(right, bottom);
    R_cont_abs(right, top);
    R_cont_abs(left, top);

    return 0;
}

int Text_width(char *text)
{
    int top, bottom, left, right;

    R_get_text_box(text, &top, &bottom, &left, &right);

    if (right > left)
	return right - left + 1;
    else
	return left - right + 1;
}

int Text(char *text, int top, int bottom, int left, int right, int the_border)
{
    R_set_window(top, bottom, left, right);
    R_move_abs(left + the_border, bottom - the_border);
    R_text(text);
    R_set_window(SCREEN_TOP, SCREEN_BOTTOM, SCREEN_LEFT, SCREEN_RIGHT);

    return 0;
}

int Uparrow(int top, int bottom, int left, int right)
{
    R_move_abs((left + right) / 2, bottom);
    R_cont_abs((left + right) / 2, top);
    R_cont_rel((left - right) / 2, (bottom - top) / 2);
    R_move_abs((left + right) / 2, top);
    R_cont_rel((right - left) / 2, (bottom - top) / 2);

    return 0;
}

int Downarrow(int top, int bottom, int left, int right)
{
    R_move_abs((left + right) / 2, top);
    R_cont_abs((left + right) / 2, bottom);
    R_cont_rel((left - right) / 2, (top - bottom) / 2);
    R_move_abs((left + right) / 2, bottom);
    R_cont_rel((right - left) / 2, (top - bottom) / 2);

    return 0;
}
