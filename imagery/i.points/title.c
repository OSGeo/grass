#include "globals.h"
#include "local_proto.h"
#include <grass/display.h>

int display_title(View * view)
{
    View *title;
    char left[100], center[100];
    int size;
    double magnification();

    *left = 0;
    *center = 0;

    if (view->cell.configured) {
	sprintf(center, "%s (mag %.1f)",
		view->cell.name, magnification(view));
    }

    if (view == VIEW_MAP1) {
	sprintf(left, "%s", G_location());
	title = VIEW_TITLE1;
    }
    else if (view == VIEW_MAP1_ZOOM) {
	title = VIEW_TITLE1_ZOOM;
    }

    if (view == VIEW_MAP2) {
	sprintf(left, "%s", G_location());
	title = VIEW_TITLE2;
    }
    else if (view == VIEW_MAP2_ZOOM) {
	title = VIEW_TITLE2_ZOOM;
    }

    Erase_view(title);
    R_standard_color(WHITE);
    size = title->nrows - 4;
    R_text_size(size, size);
    Text(left, title->top, title->bottom, title->left, title->right, 2);
    if (*center) {
	R_standard_color(YELLOW);
	Text(center, title->top, title->bottom,
	     (title->left + title->right - Text_width(center)) / 2,
	     title->right, 2);
    }

    return 0;
}
