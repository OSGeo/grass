#include <math.h>

#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

struct rectangle
{
    double t, b, l, r;
};

static void remember(struct rectangle *box, double x, double y)
{
    if (x > box->r)
	box->r = x;
    if (x < box->l)
	box->l = x;
    if (y > box->b)
	box->b = y;
    if (y < box->t)
	box->t = y;
}

static void transform(double *x, double *y,
		      int ix, int iy,
		      double orig_x, double orig_y)
{
    double ax = text_size_x * ix / 25;
    double ay = text_size_y * iy / 25;
    double rx = ax * text_cosrot - ay * text_sinrot;
    double ry = ax * text_sinrot + ay * text_cosrot;
    *x = orig_x + rx;
    *y = orig_y - ry;
}

static void draw_char(double *px, double *py, unsigned char character, struct rectangle *box)
{
    unsigned char *X;
    unsigned char *Y;
    int n_vects;
    int i;
    void (*func)(double, double);

    get_char_vects(character, &n_vects, &X, &Y);

    if (!box)
	COM_Begin();

    func = COM_Move;

    for (i = 1; i < n_vects; i++) {
	int ix, iy;
	double x, y;

	if (X[i] == ' ') {
	    func = COM_Move;
	    continue;
	}

	ix = 10 + X[i] - 'R';
	iy = 10 - Y[i] + 'R';

	transform(&x, &y, ix, iy, *px, *py);

	if (box)
	    remember(box, x, y);
	else {
	    (*func)(x, y);
	    func = COM_Cont;
	}
    }

    transform(px, py, 20, 0, *px, *py);

    if (box)
	remember(box, *px, *py);
    else
	COM_Stroke();
}

static void draw_text(const char *string, struct rectangle *box)
{
    double base_x = cur_x;
    double base_y = cur_y;
    const unsigned char *p;

    for (p = (const unsigned char *) string; *p; p++)
	draw_char(&base_x, &base_y, *p, box);
}

void get_text_ext(const char *string, double *top, double *bot, double *left, double *rite)
{
    struct rectangle box;

    box.t = 1e300;
    box.b = -1e300;
    box.l = 1e300;
    box.r = -1e300;

    draw_text(string, &box);

    *top = box.t;
    *bot = box.b;
    *left = box.l;
    *rite = box.r;
}

void soft_text(const char *string)
{
    draw_text(string, NULL);
}

