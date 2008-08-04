#include <stdio.h>
#include <grass/raster.h>

int get_win_w_mouse(float *top, float *bottom, float *left, float *right)
{
    int button;
    int st, sb, sl, sr;
    int t, b, l, r;

    st = R_screen_top();
    sb = R_screen_bot();
    sl = R_screen_left();
    sr = R_screen_rite();

    fprintf(stderr, "\nButtons:\n");
    fprintf(stderr, "Left:   Establish a corner\n");
    fprintf(stderr, "Right:  Accept window\n");

    l = sl;
    b = sb;
    r = l + 10;
    t = b - 10;

    do {
	R_get_location_with_box(l, b, &r, &t, &button);
	if (button == 1) {
	    l = r;
	    b = t;
	}
    } while (button != 3);

    if (l > r) {
	button = l;
	l = r;
	r = button;
    }

    if (t > b) {
	button = t;
	t = b;
	b = button;
    }
    *bottom = 100. - 100. * (b - st) / (sb - st);
    *top = 100. - 100. * (t - st) / (sb - st);
    *left = 100. * (l - sl) / (sr - sl);
    *right = 100. * (r - sl) / (sr - sl);

    return 0;
}
