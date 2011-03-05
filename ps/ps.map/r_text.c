/* Function: record_label
 **
 ** This is a modified version of the p.map function.
 **
 ** Modified by: Paul W. Carlson        May 1992
 */

#include <string.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include "clr.h"
#include "labels.h"
#include "local_proto.h"

#define EQ(x,y) (strcmp(x,y)==0)
#define KEY(x) EQ(key,x)
#define LEFT 0
#define RIGHT 1
#define LOWER 0
#define UPPER 1
#define CENTER 2

extern char *get_color_name();
extern int get_color_number();

static char *help[] = {
    "font        fontname",
    "color       color",
    "width       #",
    "background  color|none",
    "border      color|none",
    "size        #",
    "fontsize    fontsize",
    "hcolor      color|none",
    "hwidth      #",
    "ref         upper|lower|center left|right|center",
    "rotate      deg CCW",
    "xoffset     #",
    "yoffset     #",
    "opaque      [y|n]",
    ""
};

int read_text(char *east, char *north, char *text)
{
    PSCOLOR color, hcolor, background, border;
    int r, g, b;
    int ret;
    int xoffset;
    int yoffset;
    float size;
    int fontsize;
    double width;
    double hwidth;
    double rotate;
    int xref, yref;
    int opaque;
    char t1[128];
    char buf[1024];
    char *key, *data;
    FILE *fd;
    char fontname[128];

    set_color(&color, 0, 0, 0);	/* black */
    unset_color(&hcolor);
    unset_color(&background);
    unset_color(&border);
    opaque = TRUE;
    size = 0.0;
    fontsize = 0;
    xoffset = 0;
    yoffset = 0;
    width = 1.;
    hwidth = 0.;
    rotate = 0.0;
    xref = CENTER;
    yref = CENTER;
    strcpy(fontname, "Helvetica");

    while (*text == ' ' || *text == '\t')
	text++;
    if (*text == '\\')
	text++;
    if (*text == 0) {
	error("text", "", "no text given");
	gobble_input();
	return 0;
    }

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("font")) {
	    get_font(data);
	    strcpy(fontname, data);
	    continue;
	}

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&color, r, g, b);
	    else if (ret == 2)
		error(key, data, "primary color cannot be \"none\"");
	    else
		error(key, data, "illegal color request");

	    continue;
	}

	if (KEY("hcolor")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&hcolor, r, g, b);
	    else if (ret == 2)
		unset_color(&hcolor);
	    else
		error(key, data, "illegal hcolor request");

	    if (color_none(&hcolor) || hwidth <= 0.)
		hwidth = 0.;
	    continue;
	}

	if (KEY("background")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&background, r, g, b);
	    else if (ret == 2) {
		unset_color(&background);
		opaque = FALSE;
	    }
	    else
		error(key, data, "illegal background color request");

	    continue;
	}

	if (KEY("border")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&border, r, g, b);
	    else if (ret == 2)
		unset_color(&border);
	    else
		error(key, data, "illegal border color request");

	    continue;
	}

	if (KEY("opaque")) {
	    opaque = yesno(key, data);
	    continue;
	}

	if (KEY("width")) {
	    width = -1.;
	    *t1 = 0;
	    if (sscanf(data, "%lf%1s", &width, t1) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal width request");
	    }
	    if (t1[0] == 'i')
		width = width / 72.0;
	    continue;
	}

	if (KEY("hwidth")) {
	    hwidth = -1.;
	    *t1 = 0;
	    if (sscanf(data, "%lf%1s", &hwidth, t1) < 1 || hwidth < 0.) {
		hwidth = 0.;
		error(key, data, "illegal width request");
	    }
	    if (t1[0] == 'i')
		hwidth = hwidth / 72.0;
	    continue;
	}

	if (KEY("size")) {
	    double x;

	    if (!scan_resolution(data, &x)) {
		size = 0.0;
		error(key, data, "illegal size request");
	    }
	    else
		size = x;
	    continue;
	}

	if (KEY("fontsize")) {
	    if (sscanf(data, "%d", &fontsize) != 1 || fontsize <= 0) {
		error(key, data, "illegal fontsize request");
	    }
	    else
		continue;
	}

	if (KEY("xoffset")) {
	    *t1 = 0;
	    if (sscanf(data, "%d%1s", &xoffset, t1) != 1 || *t1) {
		xoffset = 0;
		error(key, data, "illegal request (text)");
	    }
	    continue;
	}

	if (KEY("yoffset")) {
	    *t1 = 0;
	    if (sscanf(data, "%d%1s", &yoffset, t1) != 1 || *t1) {
		yoffset = 0;
		error(key, data, "illegal request (text)");
	    }
	    continue;
	}

	if (KEY("rotate")) {
	    if (sscanf(data, "%lf", &rotate) != 1) {
		rotate = 0.0;
		error(key, data, "illegal rotate request");
	    }
	    continue;
	}

	if (KEY("ref")) {
	    if (!scan_ref(data, &xref, &yref)) {
		xref = CENTER;
		yref = CENTER;
		error(key, data, "illegal ref request");
	    }
	    continue;
	}

	error(key, data, "illegal request (text)");
    }

    /* if file doesn't exist create it and close it */
    if (labels.other == NULL) {
	labels.other = G_tempfile();
	if ((fd = fopen(labels.other, "w")) != NULL)
	    fclose(fd);
    }

    /* open file in append mode */
    fd = fopen(labels.other, "a");
    if (fd == NULL) {
	error("misc labels file", "", "can't open");
	return 1;
    }

    /* write the file */
    fprintf(fd, "font: %s\n", fontname);
    fprintf(fd, "east: %s\n", east);
    fprintf(fd, "north: %s\n", north);
    fprintf(fd, "xoffset: %d\n", xoffset);
    fprintf(fd, "yoffset: %d\n", yoffset);
    fprintf(fd, "width: %f\n", width);
    fprintf(fd, "hwidth: %f\n", hwidth);
    fprintf(fd, "size: %f\n", size);
    fprintf(fd, "fontsize: %d\n", fontsize);
    fprintf(fd, "opaque: %s\n", opaque ? "yes" : "no");
    if (rotate != 0)
	fprintf(fd, "rotate: %f\n", rotate);

    fprintf(fd, "color: ");
    if (!color_none(&color))
	fprintf(fd, "%d:%d:%d\n", color.r, color.g, color.b);
    else
	fprintf(fd, "black\n");

    fprintf(fd, "hcolor: ");
    if (!color_none(&hcolor))
	fprintf(fd, "%d:%d:%d\n", hcolor.r, hcolor.g, hcolor.b);
    else
	fprintf(fd, "none\n");

    fprintf(fd, "background: ");
    if (!color_none(&background))
	fprintf(fd, "%d:%d:%d\n", background.r, background.g, background.b);
    else
	fprintf(fd, "none\n");


    fprintf(fd, "border: ");
    if (!color_none(&border))
	fprintf(fd, "%d:%d:%d\n", border.r, border.g, border.b);
    else
	fprintf(fd, "none\n");

    fprintf(fd, "ref: ");
    switch (yref) {
    case UPPER:
	fprintf(fd, "upper");
	break;
    case LOWER:
	fprintf(fd, "lower");
	break;
    case CENTER:
	fprintf(fd, "center");
	break;
    }
    switch (xref) {
    case LEFT:
	fprintf(fd, " left");
	break;
    case RIGHT:
	fprintf(fd, " right");
	break;
    case CENTER:
	fprintf(fd, "%s", (xref == CENTER) ? "" : " center");
	break;
    }
    fprintf(fd, "\n");
    fprintf(fd, "text:%s\n\n", text);
    fclose(fd);

    return 0;
}
