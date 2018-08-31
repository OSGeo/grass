/* Functions: commentfile, do_comments
 **
 ** Function commentfile is an extended version of the p.map function.
 **
 ** Author: Paul W. Carlson     April   1992
 */

#include <stdlib.h>
#include <string.h>
#include "comment.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help1[] = {
    "where      x y",
    "font       fontname",
    "fontsize   fontsize",
    "color      color",
    ""
};
static char *help2[] = {
    "enter comments, line by line",
    ""
};

int read_comment(char *name)
{
    char buf[1024];
    char *key, *data;
    FILE *in, *out;
    int need_blank;
    int color, fontsize;
    double x, y;

    fontsize = 0;
    color = BLACK;
    x = y = 0.0;

    while (input(2, buf, help1)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("where")) {
	    if (sscanf(data, "%lf %lf", &x, &y) != 2) {
		x = y = 0.0;
		error(key, data, "illegal where request");
	    }
	    else
		continue;
	}

	if (KEY("fontsize")) {
	    fontsize = atoi(data);
	    if (fontsize < 4 || fontsize > 50)
		fontsize = 0;
	    continue;
	}

	if (KEY("color")) {
	    color = get_color_number(data);
	    if (color < 0) {
		color = BLACK;
		error(key, data, "illegal color request");
	    }
	    continue;
	}

	if (KEY("font")) {
	    get_font(data);
	    cmt.font = G_store(data);
	    continue;
	}
	error(key, data, "illegal comment sub-request");
    }
    cmt.x = x;
    cmt.y = y;
    cmt.color = color;
    if (fontsize)
	cmt.fontsize = fontsize;

    in = NULL;
    if (*name) {
	in = fopen(name, "r");
	if (in == NULL) {
	    error("comment file", name, "can't open");
	    return 1;
	}
    }
    if (PS.commentfile == NULL) {
	PS.commentfile = G_tempfile();
	need_blank = 0;
	if ((out = fopen(PS.commentfile, "w")) != NULL)
	    fclose(out);
    }
    else
	need_blank = 1;

    out = fopen(PS.commentfile, "a");
    if (out == NULL) {
	error("can't create a comments file", "", "");
	if (in == NULL)
	    gobble_input();
	else
	    fclose(in);
	return 1;
    }

    if (in == NULL)
	while (input(2, buf, help2)) {
	    if (need_blank) {
		fprintf(out, "\n");
		need_blank = 0;
	    }
	    /* G_strip(buf); */
	    fprintf(out, "%s\n", buf);
	}
    else {
	while (G_getl2(buf, sizeof buf, in)) {
	    if (need_blank) {
		fprintf(out, "\n");
		need_blank = 0;
	    }
	    /* G_strip(buf); */
	    fprintf(out, "%s\n", buf);
	}
	fclose(in);
    }
    fclose(out);

    return 0;
}

int do_comment(void)
{
    FILE *fp;
    char text[1024];
    double x, y, dy, fontsize;

    /* set font */
    fontsize = (double)cmt.fontsize;
    fprintf(PS.fp, "(%s) FN %.1f SF\n", cmt.font, fontsize);

    /* set start of first line */
    dy = 1.2 * fontsize;
    y = 72.0 * (PS.page_height - cmt.y);
    if (cmt.y > PS.page_height)
	y = PS.min_y - dy;

    x = 72.0 * PS.left_marg + 1.5;
    if (72.0 * cmt.x > x)
	x = 72.0 * cmt.x;

    /* read the comment file */
    if ((fp = fopen(PS.commentfile, "r")) == NULL) {
	error("comment file", PS.commentfile, "can't open");
	return 1;
    }
    while (G_getl2(text, sizeof text, fp)) {
	/* G_strip(text); */
	if (*text)
	    show_text(x, y, text);
	y -= dy;
    }
    fclose(fp);
    y -= 0.25 * dy;
    if (PS.min_y > y)
	PS.min_y = y;

    return 0;
}
