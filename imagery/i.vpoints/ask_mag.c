#include "globals.h"
#include <grass/raster.h>

struct box
{
    int top, bottom, left, right;
};

static struct box plus, minus, value;
static struct box cancel, accept;
static int mag;

static int dotext(char *, int, int, int, int, int);
static int inbox(struct box *, int, int);
static int incr(int, int);

int ask_magnification(int *magnification)
{
    static int use = 1;
    int x, y;
    int height;
    int stat;
    int width;
    int top, bottom, left, right;

    static Objects objects[] = {
	OTHER(incr, &use),
	{0}
    };

    Menu_msg("");

    mag = *magnification;
    if (mag < 1)
	mag = 1;

    height = VIEW_MENU->nrows;
    R_text_size(height - 4, height - 4);


    Get_mouse_xy(&x, &y);
    top = y - height / 2;
    if (top < SCREEN_TOP)
	top = SCREEN_TOP;
    bottom = top + 4 * height;
    if (bottom >= VIEW_MENU->top) {
	top -= bottom - (VIEW_MENU->top - 1);
	bottom = VIEW_MENU->top - 1;
    }
    width = Text_width("MAGNIFICATION") + 4;
    left = x - width / 2;
    if (left < SCREEN_LEFT)
	left = SCREEN_LEFT;
    right = left + width;
    if (right > SCREEN_RIGHT) {
	left -= right - SCREEN_RIGHT;
	right = SCREEN_RIGHT;
    }

    R_panel_save(tempfile1, top, bottom, left, right);
    R_standard_color(WHITE);
    R_box_abs(left, top, right, bottom);
    R_standard_color(BLACK);
    Outline_box(top, bottom, left, right);

    plus.top = top + height;
    plus.bottom = plus.top + height;
    plus.left = left;
    plus.right = plus.left + Text_width("++") + 4;
    Outline_box(plus.top, plus.bottom, plus.left, plus.right);

    minus.top = top + height;
    minus.bottom = minus.top + height;
    minus.right = right;
    minus.left = minus.right - Text_width("--") - 4;
    Outline_box(minus.top, minus.bottom, minus.left, minus.right);

    value.top = top + height;
    value.bottom = value.top + height;
    value.left = plus.right;
    value.right = minus.left;
    Outline_box(value.top, value.bottom, value.left, value.right);

    accept.top = value.bottom;
    accept.bottom = accept.top + height;
    accept.left = left;
    accept.right = right;
    Outline_box(accept.top, accept.bottom, accept.left, accept.right);

    cancel.top = accept.bottom;
    cancel.bottom = cancel.top + height;
    cancel.left = left;
    cancel.right = right;
    Outline_box(cancel.top, cancel.bottom, cancel.left, cancel.right);

    dotext("MAGNIFICATION", top, top + height, left, right, WHITE);
    dotext("+", plus.top, plus.bottom, plus.left, plus.right, GREY);
    dotext("-", minus.top, minus.bottom, minus.left, minus.right, GREY);
    dotext("ACCEPT", accept.top, accept.bottom, accept.left, accept.right,
	   GREY);
    dotext("CANCEL", cancel.top, cancel.bottom, cancel.left, cancel.right,
	   GREY);
    draw_mag();

    stat = Input_pointer(objects);

    /* to respond to user */
    R_standard_color(WHITE);
    R_box_abs(left, top, right, bottom);
    R_flush();

    R_panel_restore(tempfile1);
    R_panel_delete(tempfile1);

    *magnification = mag;
    return stat > 0;
}

int draw_mag(void)
{
    char buf[10];

    sprintf(buf, "%d", mag);
    dotext(buf, value.top, value.bottom, value.left, value.right, WHITE);

    return 0;
}

static int incr(int x, int y)
{
    if (inbox(&accept, x, y))
	return 1;
    if (inbox(&cancel, x, y))
	return -1;
    if (inbox(&plus, x, y)) {
	mag++;
	draw_mag();
    }
    else if (inbox(&minus, x, y) && mag > 1) {
	mag--;
	draw_mag();
    }
    return 0;
}

static int dotext(char *text,
		  int top, int bottom, int left, int right, int background)
{
    R_standard_color(background);
    R_box_abs(left + 1, top + 1, right - 1, bottom - 1);
    R_standard_color(BLACK);
    /* center the text */
    left = (left + right - Text_width(text)) / 2;
    Text(text, top, bottom, left, right, 2);
    R_flush();

    return 0;
}

static int inbox(struct box *box, int x, int y)
{
    return (x > box->left && x < box->right && y > box->top &&
	    y < box->bottom);
}
