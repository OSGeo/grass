#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"


#define NLINES 24

/* global variables */
struct box
{
    int top, bottom, left, right;
};

static int WHITE, RED, BLACK, GREY;
static int which;
static struct box cancel, more, less;
static int height, size, edge, count;
static int page, npages;

static struct
{
    char name[30], mapset[30];
    struct box box;
} list[NLINES * 2];


/* function prototypes */
static int dobox(struct box *, char *, int, int, int, int, int);
static int uparrow(struct box *, int);
static int downarrow(struct box *, int);
static int inbox(struct box *, int, int);


int popup(FILE * fd, int x, int y, char *msg)
{
    int len1, len2, len;
    long offset;
    long *page_offset;
    int col, nlist;
    int line;
    char buf[100];
    int top, bottom, left, right, width;
    int topx, bottomx, leftx, rightx, widthx;
    char name[30], mapset[30], cur_mapset[30];
    int new_mapset;
    char *tempfile1, *tempfile2;

    tempfile1 = G_tempfile();
    tempfile2 = G_tempfile();

    WHITE = D_translate_color("white");
    RED = D_translate_color("red");
    BLACK = D_translate_color("black");
    GREY = D_translate_color("grey");

    if (fread(&len1, sizeof(len1), 1, fd) != 1
	|| fread(&len2, sizeof(len2), 1, fd) != 1 || len1 <= 0 || len2 <= 0) {
	fclose(fd);
	return 1;
    }

    /*
     * build a popup window with x,y at center
     * 35% the height and wide enough to hold 2 columms of file names
     *
     * the window is for choosing file names and will be laid out in 2 columns
     *
     *             -------------------------------------------------
     *             |                mapset                  | LESS |
     *             -------------------------------------------------
     *             |      name1        |   name2            |      |
     *             ------------------------------------------      |
     *             |      name3        |   name4            |      |
     *             ------------------------------------------      |
     *             |      name5        |   name6            |      |
     *             |                   .                    |      |
     *             |                   .                    |      |
     *             -------------------------------------------------
     *             |                   .                    | MORE |
     *             -------------------------------------------------
     */

    /* height of 1 line, based on NLINES taking up 35% vertical space */
    height = (.35 * (BOTTOM - TOP)) / NLINES + 1;

    /* size of text, 80% of line height */
    size = .8 * height;
    R_text_size(size, size);
    size--;			/* fudge to approximate letter width */

    /* indent for the text */
    edge = .1 * height + 1;

    /* this is a fudge to determine the length of the largest text */
    len1 = 2 * len1;		/* name in 2 columns */
    len = len1 > len2 ? len1 : len2;
    width = len * size + height /* sidecar has width == height */ ;
    widthx = strlen(msg) * size;
    if (widthx < width)
	widthx = width;

    /* define the window */
    bottom = y + height * NLINES / 2;
    if (bottom > BOTTOM)
	bottom = BOTTOM;
    top = bottom - height * NLINES;
    if (top < TOP) {
	top = TOP;
	bottom = top + height * NLINES;
    }

    topx = top - height * 3;
    if (topx < TOP) {
	top -= (TOP - topx);
	bottom -= (TOP - topx);
	topx = TOP;
    }
    bottomx = topx + 2 * height;

    right = x + width / 2;
    if (right > RIGHT)
	right = RIGHT;
    left = right - width;
    if (left < LEFT) {
	left = LEFT;
	right = left + width;
    }
    leftx = (left + right - widthx) / 2;
    if (leftx < LEFT)
	leftx = LEFT;
    rightx = leftx + widthx;

    /* save what is under this window, so it can be restored */
    R_panel_save(tempfile1, top, bottom + 1, left, right + 1);
    R_panel_save(tempfile2, topx, bottomx + 1, leftx, rightx + 1);

    /* fill it with white */
    R_standard_color(WHITE);
    R_box_abs(left, top, right, bottom);
    R_box_abs(leftx, topx, rightx, bottomx);

    R_standard_color(BLACK);

    do_text(msg, topx, topx + height, leftx, rightx, 1);
    do_text("Double click here to cancel", topx + height, bottomx, leftx,
	    rightx, 1);
    outline_box(top, bottom, left, right);
    right -= height;
    outline_box(top, bottom, left, right);


    dobox(&less, "", WHITE, top, right, right + height, 0);
    dobox(&more, "", WHITE, bottom - height, right, right + height, 0);
    cancel.top = topx;
    cancel.bottom = bottomx;
    cancel.left = leftx;
    cancel.right = rightx;

    x = (cancel.left + cancel.right) / 2;
    y = (cancel.top + cancel.bottom) / 2;

    /* as we read the file of names, keep track of pages so we can
     * page backward
     */
    page = 0;
    page_offset = (long *)G_calloc(npages = 1, sizeof(long));
    if (page_offset == NULL) {
	G_message(_("Out of memory"));
	return 1;
    }
    *page_offset = ftell(fd);

    nlist = sizeof(list) / sizeof(list[0]);
    while (1) {
	line = 0;
	count = 0;
	*cur_mapset = 0;
	col = 0;
	while (1) {
	    offset = ftell(fd);
	    if (fgets(buf, sizeof buf, fd) == NULL
		|| sscanf(buf, "%s %s", name, mapset) != 2)
		break;
	    if ((new_mapset = (strcmp(cur_mapset, mapset) != 0))) {
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
		char temp[100];

		strcpy(cur_mapset, mapset);
		sprintf(temp, "Mapset %s", mapset);
		dobox(&dummy, temp, WHITE, top + line * height, left, right,
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
	R_stabilize();
	which = -1;
	switch (pick(x, y)) {
	case -1:		/* more or less */
	    break;
	default:		/* file picked */
	    G_message(_("name=%s\n"), list[which].name);
	    G_message(_("mapset=%s\n"), list[which].mapset);
	    G_message(_("fullname=%s\n"),
		      G_fully_qualified_name(list[which].name,
					     list[which].mapset));
	 /*FALLTHROUGH*/ case -2:	/* cancel */
	    R_panel_restore(tempfile1);
	    R_panel_restore(tempfile2);
	    R_panel_delete(tempfile1);
	    R_panel_delete(tempfile2);
	    R_flush();
	    return 0;
	}
	fseek(fd, page_offset[page], 0);
	R_standard_color(WHITE);
	R_box_abs(left + 1, top + 1, right - 1, bottom - 1);
    }
}

static int dobox(struct box *box, char *text,
		 int color, int top, int left, int right, int centered)
{
    int bottom;

    bottom = top + height;
    /* fill inside of box with color */
    R_standard_color(color);
    R_box_abs(left + 1, top + 1, right - 1, bottom - 1);

    /* draw box outline and text in black */
    R_standard_color(BLACK);
    do_text(text, top, bottom, left, right, centered);
    outline_box(top, bottom, left, right);

    box->top = top;
    box->bottom = bottom;
    box->left = left;
    box->right = right;

    return 0;
}

static int uparrow(struct box *box, int color)
{
    int n;

    n = (box->bottom - box->top) / 2 - edge;

    R_standard_color(color);
    R_move_abs((box->left + box->right) / 2, box->bottom - edge);
    R_cont_abs((box->left + box->right) / 2, box->top + edge);
    R_cont_rel(-n, n);
    R_move_abs((box->left + box->right) / 2, box->top + edge);
    R_cont_rel(n, n);

    return 0;
}

static int downarrow(struct box *box, int color)
{
    int n;

    n = (box->bottom - box->top) / 2 - edge;

    R_standard_color(color);
    R_move_abs((box->left + box->right) / 2, box->top + edge);
    R_cont_abs((box->left + box->right) / 2, box->bottom - edge);
    R_cont_rel(-n, -n);
    R_move_abs((box->left + box->right) / 2, box->bottom - edge);
    R_cont_rel(n, -n);

    return 0;
}

int pick(int x, int y)
{
    int n;
    int button;
    int cur;

    while (1) {
	R_get_location_with_pointer(&x, &y, &button);
	if (which != -1)
	    draw_which(BLACK);
	cur = which;
	which = -1;
	if (inbox(&more, x, y)) {
	    if (page + 1 >= npages)
		continue;
	    page++;
	    return -1;
	}
	if (inbox(&less, x, y)) {
	    if (page == 0)
		continue;
	    page--;
	    return -1;
	}
	if (inbox(&cancel, x, y)) {
	    if (cur == -2)
		return -2;
	    which = -2;
	    draw_which(RED);
	    continue;		/* first click */
	}
	/* search name list. handle double click */
	for (n = 0; n < count; n++) {
	    if (inbox(&list[n].box, x, y)) {
		which = n;
		if (n == cur)	/* second click! */
		    return 1;
		draw_which(RED);
		break;
	    }
	}
    }

    return 0;
}

int draw_which(int color)
{
    R_standard_color(color);
    if (which == -2)
	outline_box(cancel.top, cancel.bottom, cancel.left, cancel.right);
    else if (which >= 0)
	outline_box(list[which].box.top, list[which].box.bottom,
		    list[which].box.left, list[which].box.right);

    return 0;
}

static int inbox(struct box *box, int x, int y)
{
    return (x > box->left && x < box->right && y > box->top &&
	    y < box->bottom);
}

int outline_box(int top, int bottom, int left, int right)
{
    R_move_abs(left, top);
    R_cont_abs(left, bottom);
    R_cont_abs(right, bottom);
    R_cont_abs(right, top);
    R_cont_abs(left, top);
    R_flush();

    return 0;
}

int
do_text(char *text, int top, int bottom, int left, int right, int centered)
{
    R_set_window(top, bottom, left, right);	/* for text clipping */
    R_move_abs(left + 1 + edge, bottom - 1 - edge);
    if (centered)
	R_move_rel((right - left - strlen(text) * (size - 1)) / 2, 0);
    R_text(text);
    R_set_window(TOP, BOTTOM, LEFT, RIGHT);

    return 0;
}
