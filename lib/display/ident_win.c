#include <string.h>
#include <grass/raster.h>
#include <grass/display.h>

int ident_win(char *cur_pad)
{
    char **list;
    char **pads;
    int count;
    int closest;
    int npads;
    int p;
    int stat;
    int x, y, t, b, l, r;
    int button;
    int gotone;

    /* Get list of pads (windows) */
    R_pad_list(&pads, &npads);

    button = 1;

    x = (R_screen_rite() + R_screen_left()) / 2;
    y = (R_screen_top() + R_screen_bot()) / 2;

    while (button == 1) {
	closest = 9999999;
	gotone = 0;

	R_get_location_with_pointer(&x, &y, &button);
	for (p = 0; p < npads; p++) {
	    if (!strlen(pads[p]))
		continue;

	    stat = R_pad_select(pads[p]);
	    if (stat) {
		R_pad_perror("ERROR", stat);
		continue;
	    }

	    /* Check each window's "d_win" */
	    stat = R_pad_get_item("d_win", &list, &count);
	    if (stat) {
		R_pad_perror("ERROR", stat);
		continue;
	    }
	    sscanf(list[0], "%d %d %d %d", &t, &b, &l, &r);
	    R_pad_freelist(list, count);

	    /* If chosen point is outside pad window, continue */
	    if (x < l || x > r || y < t || y > b)
		continue;

	    /* If right edge closer than closest, the save pad name */
	    if ((r - x) >= 0 && (r - x) < closest) {
		closest = r - x;
		gotone = 1;
		strcpy(cur_pad, pads[p]);
	    }
	}

	if (gotone)
	    D_set_cur_wind(cur_pad);
    }
    return (button);
}
