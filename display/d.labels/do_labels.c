#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define NL	012
#define TAB	011
#define BACK	0134
#define MTEXT	1024

#define TOP	0
#define CENT	1
#define BOT	2
#define LEFT	0
#define RITE	2
#define YES	1
#define NO	0

static double east;
static double north;
static int xoffset;
static int yoffset;
static int xref;
static int yref;
static RGBA_Color color, highlight_color, background, border;
static double size;
static int fontsize;
static int highlight_width;
static int opaque;
static double width, rotation;
static char text[MTEXT];
static char font[256];
static const char *std_font;

static int ymatch(char *);
static int xmatch(char *);


int initialize_options(void)
{
    east = 0.0;
    north = 0.0;
    xoffset = 0;
    yoffset = 0;
    xref = CENT;
    yref = CENT;
    set_RGBA_from_str(&color, "black");
    set_RGBA_from_str(&highlight_color, "white");
    set_RGBA_from_str(&background, "white");
    set_RGBA_from_str(&border, "black");
    size = 1000.;
    fontsize = 0;
    width = 1.;
    highlight_width = 0;
    opaque = YES;
    rotation = 0.0;
    std_font = getenv("GRASS_FONT");
    if (!std_font)
	std_font = "romans";
    strcpy(font, std_font);

    return 0;
}


int do_labels(FILE * infile, int do_rotation)
{
    char buff[128];

    initialize_options();

    while (G_getl2(text, MTEXT, infile)) {
	if (text[0] == '#')
	    continue;

	if (!strncmp(text, "eas", 3))
	    sscanf(text, "%*s %lf", &east);
	else if (!strncmp(text, "nor", 3))
	    sscanf(text, "%*s %lf", &north);
	else if (!strncmp(text, "xof", 3))
	    sscanf(text, "%*s %d", &xoffset);
	else if (!strncmp(text, "yof", 3))
	    sscanf(text, "%*s %d", &yoffset);
	else if (!strncmp(text, "col", 3)) {
	    sscanf(text, "%*s %s", buff);
	    set_RGBA_from_str(&color, buff);
	}
	else if (!strncmp(text, "siz", 3))
	    sscanf(text, "%*s %lf", &size);
	else if (!strncmp(text, "fontsize", 8))
	    sscanf(text, "%*s %d", &fontsize);
	else if (!strncmp(text, "wid", 3))
	    sscanf(text, "%*s %lf", &width);
	else if (!strncmp(text, "bac", 3)) {
	    sscanf(text, "%*s %s", buff);
	    set_RGBA_from_str(&background, buff);
	}
	else if (!strncmp(text, "bor", 3)) {
	    sscanf(text, "%*s %s", buff);
	    set_RGBA_from_str(&border, buff);
	}
	else if (!strncmp(text, "opa", 3)) {
	    sscanf(text, "%*s %s", buff);
	    if (!strncmp(buff, "YES", 3))
		opaque = YES;
	    else
		opaque = NO;
	}
	else if (!strncmp(text, "ref", 3)) {
	    if (sscanf(text, "%*s %16[^\n]", buff) < 1 || scan_ref(buff) == 0) {
		xref = CENT;
		yref = CENT;
	    }
	}
	else if (!strncmp(text, "fon", 3)) {
	    if (sscanf(text, "%*s %s", font) != 1
		|| !strcmp(font, "standard"))
		strcpy(font, std_font);
	}
	else if (!strncmp(text, "rot", 3)) {
	    if (do_rotation)
		sscanf(text, "%*s %lf", &rotation);
	}
	else if (!strncmp(text, "hco", 3)) {
	    sscanf(text, "%*s %s", buff);
	    set_RGBA_from_str(&highlight_color, buff);
	}
	else if (!strncmp(text, "hwi", 3))
	    sscanf(text, "%*s %d", &highlight_width);

	else if (!strncmp(text, "tex", 3)) {
	    show_it();
	    rotation = 0.0;	/* reset */
	}

	else {
	    if (sscanf(text, "%1s", buff) == 1)
		fprintf(stderr, _("Error: %s\n"), text);
	}
    }

    return 0;
}

int show_it(void)
{
    /*
     * The border+background box coords given by R_get_text_box() expand to
     * cover the area of the rotated text, but the bottom left corner of that
     * box is not always the ref=lower,left spot (rot>90), and middle|upper
     * left of the text do not match the middle|upper left of the expanded 
     * text box when rotated.
     * 
     * The solution is to calculate the position and dimensions of the text
     * without rotation, then rotate those points about the point's coord,
     * and replot. For text we must calculate the starting coord of the text
     * independent of the text box, once for each line of text (if multiline).
     *
     */
    int i, j;
    int n_lines;
    int n_chars;
    char line[256];
    char *lptr, *tptr;
    double line_size;
    int text_size;
    int X, Y, Y0;
    int T, B, L, R;
    int t, b, l, r;
    int xarr[5];
    int yarr[5];
    int Xoffset, Yoffset;	/* in XY plane */
    int X_just_offset, Y_just_offset;	/* in rotated label plane */
    int ll_x, ll_y, ul_x, ul_y, lr_x, lr_y, ur_x, ur_y, text_x, text_y;

    G_debug(3, "Doing '%s'", text);
    X = (int)(D_u_to_d_col(east));
    Y0 = (int)(D_u_to_d_row(north));

    /* Set font */
    R_font(font);

    /* Set text size */
    if (fontsize) {
	text_size = fontsize;
	line_size =
	    (D_d_to_u_row(0.0) - D_d_to_u_row((double)fontsize)) * 1.2;
    }
    else {
	text_size = D_u_to_d_row(0.0) - D_u_to_d_row(size);
	line_size = size * 1.2;
    }

    R_text_size(text_size, text_size);

    /* Find extent of all text (assume ref point is upper left) */
    T = 999999;
    B = 0;
    L = 999999;
    R = 0;

    /* Scan to beginning of text string */
    for (tptr = text; *tptr != ':'; tptr++) ;
    tptr++;

    /* get the box size for each line of text and expand the bounding box as needed */
    n_lines = 0;
    for (;;) {
	n_chars = 0;

	for (lptr = line; *tptr && *tptr != NL; *lptr++ = *tptr++) {
	    /* R_get_text_box() seems to skip leading spaces?, so for
	       multiline we need to append a space to secondary lines
	       to get the placement right (??) */
	    if ((lptr == line) && (n_lines > 0))
		*lptr++ = ' ';

	    if ((*tptr == BACK) && (*(tptr + 1) == 'n'))
		break;
	    n_chars++;
	}
	n_lines++;

	if (n_chars == 0)
	    break;

	*lptr = '\0';

	G_debug(3, "line %d ='%s'", n_lines, line);

	Y = (int)(D_u_to_d_row
		  (north - (line_size * 1.2) - ((n_lines - 1) * line_size)));
	R_move_abs(X, Y);
	R_text_rotation(0.0);	/* reset */
	R_get_text_box(line, &t, &b, &l, &r);

	if (t < T)
	    T = t;
	if (b > B)
	    B = b;
	if (l < L)
	    L = l;
	if (r > R)
	    R = r;

	if ((*tptr == '\0') || (*tptr == NL))
	    break;
	tptr++;
	tptr++;
    }
    G_debug(3, "nlines=%d", n_lines);

    /* enforce min/max width */
    if (width > 25.)
	width = 25.;
    if (width < 0.)
	width = 0.;

    /* Expand border 1/2 of text size */
    T = T - (text_size * 0.2) - .5;
    B = B + (text_size * 0.2) + .5;
    L = L - (text_size * 0.2) - .5;
    R = R + (text_size * 0.2) + .5;

    Xoffset = xoffset;
    Yoffset = -yoffset;
    X_just_offset = 0;
    Y_just_offset = 0;

    /* shift to match justification */
    if (xref == CENT)
	X_just_offset -= (R - L + text_size) / 2;
    if (xref == RITE)
	X_just_offset -= R - L + text_size;
    if (yref == CENT)
	Y_just_offset -= ((B - Y0) / 2) - (Y0 - T);
    if (yref == BOT)
	Y_just_offset -= (B - Y0) - (Y0 - T);

    /* get unrotated corners of text box, and rotate them */
    ul_y = ur_y = T + Y_just_offset;
    ll_y = lr_y = B + Y_just_offset;
    ll_x = ul_x = L + X_just_offset;
    lr_x = ur_x = R + X_just_offset;
    G_rotate_around_point_int(X, Y0, &ll_x, &ll_y, -1 * rotation);
    G_rotate_around_point_int(X, Y0, &ul_x, &ul_y, -1 * rotation);
    G_rotate_around_point_int(X, Y0, &ur_x, &ur_y, -1 * rotation);
    G_rotate_around_point_int(X, Y0, &lr_x, &lr_y, -1 * rotation);

    /* rotate lower starting corner of text */
    text_x = X + X_just_offset;
    text_y = Y + Y_just_offset;
    G_rotate_around_point_int(X, Y0, &text_x, &text_y, -1 * rotation);

    /* define rotated bounding box */
    xarr[0] = ll_x + Xoffset;
    xarr[1] = ul_x + Xoffset;
    xarr[2] = ur_x + Xoffset;
    xarr[3] = lr_x + Xoffset;
    xarr[4] = ll_x + Xoffset;
    yarr[0] = ll_y + Yoffset;
    yarr[1] = ul_y + Yoffset;
    yarr[2] = ur_y + Yoffset;
    yarr[3] = lr_y + Yoffset;
    yarr[4] = ll_y + Yoffset;

    /* skip labels which will go offscreen (even partially) */
    for (i = 0; i < 5; i++) {
	if ((xarr[i] > D_get_d_east()) || (xarr[i] < D_get_d_west()))
	    return 0;
	if ((yarr[i] < D_get_d_north()) || (yarr[i] > D_get_d_south()))
	    return 0;
	if ((xarr[i] < 0) || (yarr[i] < 0))
	    return 0;
    }

#ifdef OUTPUT_ASCII
    fprintf(stdout, "L 5\n");
    for (i = 0; i < 5; i++) {
	fprintf(stdout, " %f %f\n", D_d_to_u_col(xarr[i]),
		D_d_to_u_row(yarr[i]));
    }
    /* d.labels labfile | v.in.ascii -n out=labbox format=standard */
#endif

    /* draw boxes */
    if (RGBA_has_color(&background)) {
	set_color_from_RGBA(&background);
	R_polygon_abs(xarr, yarr, 5);
    }
    if (RGBA_has_color(&border)) {
	set_color_from_RGBA(&border);
	R_line_width((int)(width + 0.5));
	R_polyline_abs(xarr, yarr, 5);
	R_line_width(1);	/* reset to default. note a value of 0 is valid and slightly thinner and 1 can be ugly */
    }

    /* Set font rotation */
    R_text_rotation((float)rotation);
    G_debug(3, "  rotation = %.2f", rotation);

    /**** draw highlighted text background ****/
    if (highlight_width && RGBA_has_color(&highlight_color)) {
	set_color_from_RGBA(&highlight_color);

	/* Scan to beginning of text string */
	for (tptr = text; *tptr != ':'; tptr++) ;
	tptr++;

	for (i = 1; i <= n_lines; i++) {
	    /* get line of text from full label text string */
	    n_chars = 0;
	    for (lptr = line; *tptr && *tptr != NL; *lptr++ = *tptr++) {
		if ((lptr == line) && (i > 1))	/* see comment above */
		    *lptr++ = ' ';
		if ((*tptr == BACK) && (*(tptr + 1) == 'n'))
		    break;
		n_chars++;
	    }
	    if (n_chars == 0)
		break;
	    *lptr = '\0';

	    /* figure out text placement */
	    Y = (int)(D_u_to_d_row
		      (north - (line_size * 1.2) - ((i - 1) * line_size)));
	    text_x = X + X_just_offset;	/* reset after G_rotate_around_point_int() */
	    text_y = Y + Y_just_offset;
	    G_rotate_around_point_int(X, Y0, &text_x, &text_y, -1 * rotation);

	    for (j = 1; j <= highlight_width; j++) {
		/* smear it around. probably a better way (knight's move? rand?) */
		R_move_abs(text_x + Xoffset, text_y + Yoffset + j);
		R_text(line);
		R_move_abs(text_x + Xoffset, text_y + Yoffset - j);
		R_text(line);
		R_move_abs(text_x + Xoffset + j, text_y + Yoffset);
		R_text(line);
		R_move_abs(text_x + Xoffset - j, text_y + Yoffset);
		R_text(line);

		R_move_abs(text_x + Xoffset + j, text_y + Yoffset + j);
		R_text(line);
		R_move_abs(text_x + Xoffset - j, text_y + Yoffset - j);
		R_text(line);
		R_move_abs(text_x + Xoffset + j, text_y + Yoffset - j);
		R_text(line);
		R_move_abs(text_x + Xoffset - j, text_y + Yoffset + j);
		R_text(line);
	    }

	    if ((*tptr == '\0') || (*tptr == NL))
		break;
	    tptr++;
	    tptr++;
	}
    }


    /**** place the text ****/
    set_color_from_RGBA(&color);

    /* Scan to beginning of text string */
    for (tptr = text; *tptr != ':'; tptr++) ;
    tptr++;

    for (i = 1; i <= n_lines; i++) {
	/* get line of text from full label text string */
	n_chars = 0;
	for (lptr = line; *tptr && *tptr != NL; *lptr++ = *tptr++) {
	    if ((lptr == line) && (i > 1))	/* see comment above */
		*lptr++ = ' ';
	    if ((*tptr == BACK) && (*(tptr + 1) == 'n'))
		break;
	    n_chars++;
	}
	if (n_chars == 0)
	    break;
	*lptr = '\0';

	/* figure out text placement */
	Y = (int)(D_u_to_d_row
		  (north - (line_size * 1.2) - ((i - 1) * line_size)));
	text_x = X + X_just_offset;	/* reset after G_rotate_around_point_int() */
	text_y = Y + Y_just_offset;
	G_rotate_around_point_int(X, Y0, &text_x, &text_y, -1 * rotation);

	R_move_abs(text_x + Xoffset, text_y + Yoffset);
	R_text(line);

	if ((*tptr == '\0') || (*tptr == NL))
	    break;
	tptr++;
	tptr++;
    }


    return 0;
}

static int xok, yok;

int scan_ref(char *buf)
{
    char word1[50], word2[50];
    int i;

    xok = yok = 0;

    for (i = 0; buf[i]; i++)
	if (buf[i] >= 'A' && buf[i] <= 'Z')
	    buf[i] += 'a' - 'A';
    xref = yref = CENT;
    switch (sscanf(buf, "%s%s", word1, word2)) {
    case 2:
	if (!(xmatch(word2) || ymatch(word2)))
	    return 0;
    case 1:
	if (xmatch(word1) || ymatch(word1))
	    return 1;
    default:
	return 0;
    }
}

static int xmatch(char *word)
{
    if (strcmp(word, "center") == 0)
	return 1;
    if (strcmp(word, "middle") == 0)
	return 1;
    if (xok)
	return 0;

    if (strcmp(word, "left") == 0)
	xref = LEFT;
    else if (strcmp(word, "right") == 0)
	xref = RITE;
    else
	return 0;
    xok = 1;
    return 1;
}

static int ymatch(char *word)
{
    if (strcmp(word, "center") == 0)
	return 1;
    if (strcmp(word, "middle") == 0)
	return 1;
    if (yok)
	return 0;

    if (strcmp(word, "upper") == 0)
	yref = TOP;
    else if (strcmp(word, "top") == 0)
	yref = TOP;
    else if (strcmp(word, "lower") == 0)
	yref = BOT;
    else if (strcmp(word, "bottom") == 0)
	yref = BOT;
    else
	return 0;
    yok = 1;
    return 1;
}
