#include <string.h>
#include <grass/display.h>
#include "globals.h"

#define NLINES 18
struct box
{
    int top, bottom, left, right;
};

static int text_size;
static int which;
static struct box cancel, more, less;
static int height, size, edge, count;
static int page, npages;
static struct
{
    char name[100], mapset[100];
    struct box box;
} list[NLINES * 2];

static int dobox(struct box *, char *, int, int, int, int, int);
static int uparrow(struct box *, int);
static int downarrow(struct box *, int);
static int pick(int, int, int);
static int inbox(struct box *, int, int);
static int cancel_which(void);
static int dotext(char *, int, int, int, int, int);

int ask_gis_files(char *type, char *file, char *xname, char *xmapset,
		  int position)
{
    static int use = 1;
    static Objects objects[] = {
	OTHER(pick, &use),
	{0}
    };

    char msg[100];
    FILE *fd;
    int width;
    int len1, len2, len;
    long offset;
    long *page_offset;
    int col, nlist;
    int line;
    int stat;
    char buf[100];
    int top, bottom, left, right, center;
    int topx, bottomx, leftx, rightx, widthx;
    char name[100], mapset[100], cur_mapset[100];
    int new_mapset;

    Menu_msg("");

    fd = fopen(file, "r");
    if (fd == NULL)
	G_fatal_error("ask_gis_files: can't read tempfile");
    if (fread(&len1, sizeof(len1), 1, fd) != 1
	|| fread(&len2, sizeof(len2), 1, fd) != 1 || len1 <= 0 || len2 <= 0) {
	fclose(fd);
	return 0;
    }

    sprintf(msg, "Double click on %s file to be plotted", type);

    /*
     * build a popup window at center of the screen.
     * 35% the height and wide enough to hold 2 columms of file names
     *
     * the window is for choosing file names and will be laid out in 2 columns
     *
     *             ------------------------------------------
     *             |     CANCEL           | (MORE) | (LESS) | 
     *             ------------------------------------------
     *             |             mapset                     |
     *             ------------------------------------------
     *             |      name1        |   name2            |
     *             ------------------------------------------
     *             |      name3        |   name4            |
     *             ------------------------------------------
     *             |      name5        |   name6            |
     *             |                   .                    |
     *             |                   .                    |
     *             |                   .                    |
     *             ------------------------------------------
     */

    /* height of 1 line, based on NLINES taking up 35% vertical space */
    height = (.35 * (SCREEN_BOTTOM - SCREEN_TOP)) / NLINES + 1;

    /* size of text, 80% of line height */
    text_size = .8 * height;
    size = text_size - 1;	/* fudge for computing pixels width of text */

    /* indent for the text */
    edge = .1 * height + 1;

    /* this is a fudge to determine the length of the largest text */
    len1 = 2 * len1;		/* name in 2 columns */
    len2 += strlen("mapset ");
    len = (len1 > len2 ? len1 : len2);

    /* width is for max chars plus sidecar for more/less */
    width = len * size + height;
    widthx = strlen(msg) * size;
    if (widthx < width)
	widthx = width;

    /* define the window */
    top = (SCREEN_TOP + SCREEN_BOTTOM - height * NLINES) / 2;
    bottom = top + height * NLINES;

    center = (SCREEN_LEFT + SCREEN_RIGHT) / 2;
    if (position > 0) {
	right = (center + SCREEN_RIGHT + width) / 2;
	if (right >= SCREEN_RIGHT)
	    right = SCREEN_RIGHT - 1;
	left = right - width;
    }
    else if (position < 0) {
	left = (center + SCREEN_LEFT - width) / 2;
	if (left <= SCREEN_LEFT)
	    left = SCREEN_LEFT + 1;
	right = left + width;
    }
    else {
	left = center + width / 2;
	right = left + width;
    }

    topx = top - 3 * height;
    bottomx = topx + 2 * height;
    leftx = (left + right - widthx) / 2;
    if (leftx < SCREEN_LEFT)
	leftx = SCREEN_LEFT;
    rightx = leftx + widthx;

    /* save what is under these areas, so they can be restored */
    R_panel_save(tempfile1, top, bottom + 1, left, right + 1);
    R_panel_save(tempfile2, topx, bottomx + 1, leftx, rightx + 1);

    /* fill it top with GREY, pick area with white */
    R_standard_color(WHITE);
    R_box_abs(left, top, right, bottom);
    R_standard_color(GREY);
    R_box_abs(leftx, topx, rightx, bottomx);

    R_standard_color(BLACK);
    Outline_box(top, bottom, left, right);
    right -= height;		/* reduce it to exclude sidecar */
    Outline_box(top, bottom, left, right);

    /* print messages above the files */
    dotext(msg, topx, topx + height, leftx, rightx, 1);
    dotext("Double click here to cancel", topx + height, bottomx, leftx,
	   rightx, 1);
    cancel.top = topx;
    cancel.bottom = bottomx;
    cancel.left = leftx;
    cancel.right = rightx;

    /* start the mouse in the cancel box */
    Set_mouse_xy((leftx + rightx) / 2, (topx + bottomx) / 2);

    dobox(&less, "", WHITE, top, right, right + height, 0);
    dobox(&more, "", WHITE, bottom - height, right, right + height, 0);

    /* as we read the file of names, keep track of pages so we can
     * page backward
     */
    page = 0;
    page_offset = (long *)G_calloc(npages = 1, sizeof(long));
    *page_offset = G_ftell(fd);

    nlist = sizeof(list) / sizeof(list[0]);
    for (stat = -1; stat < 0;) {
	line = 0;
	count = 0;
	*cur_mapset = 0;
	col = 0;
	while (1) {
	    offset = G_ftell(fd);
	    if (fgets(buf, sizeof buf, fd) == NULL
		|| sscanf(buf, "%s %s", name, mapset) != 2)
		break;
	    if (new_mapset = (strcmp(cur_mapset, mapset) != 0)) {
		if (line)
		    line++;
		if (col)
		    line++;
		col = 0;
	    }
	    if (count >= nlist || line + new_mapset >= NLINES) {
		if (page + 1 == npages) {
		    npages++;
		    page_offset =
			(long *)G_realloc(page_offset, npages * sizeof(long));
		    page_offset[npages - 1] = offset;
		}
		break;
	    }
	    if (new_mapset) {
		struct box dummy;
		char label[100];

		strcpy(cur_mapset, mapset);
		sprintf(label, "Mapset %s", mapset);
		dobox(&dummy, label, WHITE, top + line * height, left, right,
		      0);
		line++;
	    }
	    if (col) {
		dobox(&list[count].box, name, GREY, top + line * height,
		      left + width / 2, right, 0);
		line++;
		col = 0;
	    }
	    else {
		dobox(&list[count].box, name, GREY, top + line * height, left,
		      left + width / 2, 0);
		col = 1;
	    }
	    strcpy(list[count].name, name);
	    strcpy(list[count].mapset, mapset);
	    count++;
	}
	downarrow(&more, page + 1 < npages ? BLACK : WHITE);
	uparrow(&less, page > 0 ? BLACK : WHITE);
	which = -1;
	switch (Input_pointer(objects)) {
	case -1:		/* more or less */
	    break;
	case -2:		/* cancel */
	    stat = 0;
	    continue;
	default:		/* file picked */
	    strcpy(xname, list[which].name);
	    strcpy(xmapset, list[which].mapset);
	    stat = 1;
	    continue;
	}
	G_fseek(fd, page_offset[page], 0);
	R_standard_color(WHITE);
	R_box_abs(left + 1, top + 1, right - 1, bottom - 1);
    }

    /* all done. restore what was under the window */
    right += height;		/* move it back over the sidecar */
    R_standard_color(WHITE);
    R_box_abs(left, top, right, bottom);
    R_panel_restore(tempfile1);
    R_panel_restore(tempfile2);
    R_panel_delete(tempfile1);
    R_panel_delete(tempfile2);
    R_flush();

    G_free(page_offset);
    return stat;
}

static int dobox(struct box *box,
		 char *text, int color, int top, int left, int right,
		 int centered)
{
    int bottom;

    bottom = top + height;
    /* fill inside of box with color */
    R_standard_color(color);
    R_box_abs(left + 1, top + 1, right - 1, bottom - 1);

    /* draw box outline and text in black */
    R_standard_color(BLACK);
    Outline_box(top, bottom, left, right);
    dotext(text, top, bottom, left, right, centered);
    R_flush();

    box->top = top;
    box->bottom = bottom;
    box->left = left;
    box->right = right;

    return 0;
}

static int uparrow(struct box *box, int color)
{
    R_standard_color(color);
    Uparrow(box->top + edge, box->bottom - edge, box->left + edge,
	    box->right - edge);

    return 0;
}

static int downarrow(struct box *box, int color)
{
    R_standard_color(color);
    Downarrow(box->top + edge, box->bottom - edge, box->left + edge,
	      box->right - edge);

    return 0;
}

static int pick(int x, int y, int button)
{
    int n;

    if (inbox(&more, x, y)) {
	cancel_which();
	if (page + 1 >= npages)
	    return 0;
	page++;
	return -1;
    }
    if (inbox(&less, x, y)) {
	cancel_which();
	if (page == 0)
	    return 0;
	page--;
	return -1;
    }
    if (inbox(&cancel, x, y)) {
	if (which == -2)
	    return -2;
	cancel_which();
	which = -2;
	R_standard_color(RED);
	Outline_box(cancel.top, cancel.bottom, cancel.left, cancel.right);
	R_flush();
	return 0;
    }
    /* search name list. handle double click */
    for (n = 0; n < count; n++)
	if (inbox(&list[n].box, x, y)) {
	    if (n == which)	/* second click! */
		return 1;
	    cancel_which();
	    which = n;
	    R_standard_color(RED);
	    Outline_box(list[n].box.top, list[n].box.bottom,
			list[n].box.left, list[n].box.right);
	    R_flush();
	    return 0;		/* ignore first click */
	}

    cancel_which();
    return 0;
}

static int cancel_which(void)
{
    if (which == -2) {
	R_standard_color(BLACK);
	Outline_box(cancel.top, cancel.bottom, cancel.left, cancel.right);
    }
    else if (which >= 0) {
	R_standard_color(BLACK);
	Outline_box(list[which].box.top, list[which].box.bottom,
		    list[which].box.left, list[which].box.right);
    }
    which = -1;
    R_flush();

    return 0;
}

static int inbox(struct box *box, int x, int y)
{
    return (x > box->left && x < box->right && y > box->top &&
	    y < box->bottom);
}

static int dotext(char *text,
		  int top, int bottom, int left, int right, int centered)
{
    R_text_size(text_size, text_size);
    R_move_abs(left + 1 + edge, bottom - 1 - edge);
    if (centered)
	R_move_rel((right - left - strlen(text) * size) / 2, 0);
    R_set_window(top, bottom, left, right);	/* for text clipping */
    R_text(text);
    R_set_window(SCREEN_TOP, SCREEN_BOTTOM, SCREEN_LEFT, SCREEN_RIGHT);

    return 0;
}
