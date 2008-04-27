/*
 ****************************************************************************
 *
 * MODULE:       d.text
 *
 * AUTHOR(S):    James Westervelt, US Army CERL
 *               Updated by Huidae Cho
 *
 * PURPOSE:      display text in active frame
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/*
 *   d.text commands:
 *
 *      .F {font|path}[:charset]                font
 *      .C {color_name|RR:GG:BB|0xRRGGBB}       color
 *      .S [+|-]size[p]                         text size
 *                                              +/-: relative size
 *                                              p: size in pixels
 *      .B {0|1}                                bold (double print) off/on
 *      .A {ll|lc|lr|cl|cc|cr|ul|uc|ur}         align (TODO)
 *      .R [+|-]rotation[r]                     rotation
 *                                              +/-: relative rotation
 *                                              r: angle in radian
 *      .I linespacing                          linespacing
 *      .X [+|-]x[%|p]                          x: relative to x origin
 *                                              +/-: relative dx
 *                                              %: percentage
 *                                              p: pixels
 *      .Y [+|-]y[%|p]                          y: relative to y origin
 *                                              +/-: relative dy
 *                                              %: percentage
 *                                              p: pixels
 *      .L {0|1}                                linefeed off/on
 *      .E [+|-]east[%|p]                       x origin: geographic coordinates
 *                                              +/-: relative de
 *                                              %: percentage
 *                                              p: pixels
 *      .N [+|-]north[%|p]                      y origin: geographic coordinates
 *                                              +/-: relative dn
 *                                              %: percentage
 *                                              p: pixels
 *      ..                                      draw one dot (.)
 *      .<SPACE>                                comments
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/colors.h>
#include <grass/glocale.h>

#define BACKWARD_COMPATIBILITY
#define DEFAULT_COLOR "gray"

struct rectinfo
{
    int t, b, l, r;
};

static void set_color(char *);
static int get_coordinates(int *, int *, double *, double *,
			   struct rectinfo, char **, char, char, char);
static void draw_text(char *, int *, int *, int, char *, double, char);

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *text;
	struct Option *size;
	struct Option *color;
	struct Option *line;
	struct Option *at;
	struct Option *rotation;
	struct Option *align;
	struct Option *linespacing;
	struct Option *font;
	struct Option *path;
	struct Option *charset;
    } opt;
    struct
    {
	struct Flag *p;
	struct Flag *g;
	struct Flag *b;
	struct Flag *r;
	struct Flag *s;
	struct Flag *m;
	struct Flag *c;
    } flag;

    /* options and flags */
    char *text;
    int size;
    int x, y;
    int line;
    double rotation;
    char align[3];
    double linespacing;

    char bold;

    /* window info */
    char win_name[64];
    struct rectinfo win;

    /* command file */
    FILE *cmd_fp;
    char *cmd_file;

    char buf[512];

    int first_text;
    int linefeed;
    int set_l;
    int orig_x, orig_y;
    int prev_x, prev_y;
    int set_x, set_y;
    double east, north;

    /* initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display");
    module->description =
	_
	("Draws text in the active display frame on the graphics monitor using the current font.");

    opt.text = G_define_option();
    opt.text->key = "text";
    opt.text->type = TYPE_STRING;
    opt.text->required = NO;
    opt.text->description = _("Text to display");

    opt.size = G_define_option();
    opt.size->key = "size";
    opt.size->type = TYPE_DOUBLE;
    opt.size->required = NO;
    opt.size->answer = "5";
    opt.size->options = "0-100";
    opt.size->description =
	_("Height of letters in percentage of available frame height");

    opt.color = G_define_option();
    opt.color->key = "color";
    opt.color->type = TYPE_STRING;
    opt.color->answer = DEFAULT_COLOR;
    opt.color->required = NO;
    opt.color->description =
	_("Text color, either a standard GRASS color or R:G:B triplet");
    opt.color->gisprompt = GISPROMPT_COLOR;

    opt.line = G_define_option();
    opt.line->key = "line";
    opt.line->required = NO;
    opt.line->type = TYPE_INTEGER;
    opt.line->options = "1-1000";
    opt.line->description =
	_("The screen line number on which text will begin to be drawn");

    opt.at = G_define_option();
    opt.at->key = "at";
    opt.at->key_desc = "x,y";
    opt.at->type = TYPE_DOUBLE;
    opt.at->required = NO;
    opt.at->description =
	_
	("Screen position at which text will begin to be drawn (percentage, [0,0] is lower left)");

    opt.align = G_define_option();
    opt.align->key = "align";
    opt.align->type = TYPE_STRING;
    opt.align->required = NO;
    opt.align->answer = "ll";
    opt.align->options = "ll,lc,lr,cl,cc,cr,ul,uc,ur";
    opt.align->description = _("Text alignment");

    opt.rotation = G_define_option();
    opt.rotation->key = "rotation";
    opt.rotation->type = TYPE_DOUBLE;
    opt.rotation->required = NO;
    opt.rotation->answer = "0";
    opt.rotation->description =
	_("Rotation angle in degrees (counter-clockwise)");

    opt.linespacing = G_define_option();
    opt.linespacing->key = "linespacing";
    opt.linespacing->type = TYPE_DOUBLE;
    opt.linespacing->required = NO;
    opt.linespacing->answer = "1.25";
    opt.linespacing->description = _("Line spacing");

    opt.font = G_define_option();
    opt.font->key = "font";
    opt.font->type = TYPE_STRING;
    opt.font->required = NO;
    opt.font->description = _("Font name");

    opt.path = G_define_option();
    opt.path->key = "path";
    opt.path->type = TYPE_STRING;
    opt.path->required = NO;
    opt.path->description = _("Path to font file");
    opt.path->gisprompt = "old_file,file,font";

    opt.charset = G_define_option();
    opt.charset->key = "charset";
    opt.charset->type = TYPE_STRING;
    opt.charset->required = NO;
    opt.charset->description = "Text encoding (only applicable to TrueType fonts)";

    flag.m = G_define_flag();
    flag.m->key = 'm';
    flag.m->description = _("Use mouse to interactively place text");

    flag.p = G_define_flag();
    flag.p->key = 'p';
    flag.p->description = _("Screen position in pixels ([0,0] is top left)");

    flag.g = G_define_flag();
    flag.g->key = 'g';
    flag.g->description = _("Screen position in geographic coordinates");

    flag.b = G_define_flag();
    flag.b->key = 'b';
    flag.b->description = _("Use bold text");

    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description = _("Use radians instead of degrees for rotation");

    flag.s = G_define_flag();
    flag.s->key = 's';
    flag.s->description = _("Font size is height in pixels");

    flag.c = G_define_flag();
    flag.c->key         = 'c';
    flag.c->description = _("Ignored (compatibility with d.text.freetype)");

    /* check command line */
    if (G_parser(argc, argv))
	exit(1);

    /* parse and check options and flags */

    if ((opt.line->answer && opt.at->answer) ||
	(opt.line->answer && flag.m->answer) ||
	(opt.at->answer && flag.m->answer) ||
	(flag.p->answer && flag.g->answer))
	G_fatal_error(_("Please choose only one placement method"));

    text = opt.text->answer;

    line = (opt.line->answer ? atoi(opt.line->answer) : 1);

    /* calculate rotation angle in radian */
    rotation = atof(opt.rotation->answer);
    if (!flag.r->answer)
	rotation *= M_PI / 180.0;
    rotation = fmod(rotation, 2.0 * M_PI);
    if (rotation < 0.0)
	rotation += 2.0 * M_PI;

    strncpy(align, opt.align->answer, 2);
    linespacing = atof(opt.linespacing->answer);

    bold = flag.b->answer;

    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));


    if (opt.font->answer)
	R_font(opt.font->answer);
    else if (opt.path->answer)
	R_font(opt.path->answer);

    if (opt.charset->answer)
	R_charset(opt.charset->answer);

    D_setup(0);

    if (D_get_cur_wind(win_name))
	G_fatal_error(_("No current window"));

    if (D_set_cur_wind(win_name))
	G_fatal_error(_("Current window not available"));

    /* figure out where to put text */
    D_get_screen_window(&win.t, &win.b, &win.l, &win.r);
    R_set_window(win.t, win.b, win.l, win.r);

    if (flag.s->answer)
	size = (int)atof(opt.size->answer);
    else
#ifdef BACKWARD_COMPATIBILITY
	size =
	    (int)(atof(opt.size->answer) / 100.0 * (win.b - win.t) /
		  linespacing);
#else
	size = (int)(atof(opt.size->answer) / 100.0 * (win.b - win.t));
#endif

    set_color(opt.color->answer);

    orig_x = orig_y = 0;

    if (opt.at->answer || flag.m->answer) {
	if (get_coordinates(&x, &y, &east, &north,
			    win, opt.at->answers, flag.m->answer,
			    flag.p->answer, flag.g->answer))
	    G_fatal_error(_("Invalid coordinates"));
	orig_x = x;
	orig_y = y;
    }
    else {
	x = win.l + (int)(size * linespacing + 0.5) - size;	/* d.text: +5 */
	y = win.t + line * (int)(size * linespacing + 0.5);
    }

    prev_x = x;
    prev_y = y;

    R_text_size(size, size);
    R_text_rotation((float)(rotation * 180.0 / M_PI));

    if (text) {
	int x2, y2;

	x2 = x;
	y2 = y;

	if (text[0])
	    draw_text(text, &x2, &y2, size, align, rotation, bold);
	if (opt.at->answer || opt.line->answer)
	    D_add_to_list(G_recreate_command());
	else {
	    if (flag.m->answer) {
		if (flag.p->answer)
		    sprintf(buf, "%s -p at=%d,%d", argv[0], x, y);
		else if (flag.g->answer)
		    sprintf(buf, "%s -g at=%f,%f", argv[0], east, north);
		else
		    sprintf(buf, "%s at=%f,%f", argv[0],
			    (double)(x * 100.0 / (double)(win.r - win.l)),
			    (double)((win.b - y) * 100.0 / (double)(win.b -
								    win.t)));

		sprintf(buf, "%s text=\"%s\"", buf, opt.text->answer);
		sprintf(buf, "%s size=%s", buf, opt.size->answer);
		sprintf(buf, "%s color=%s", buf, opt.color->answer);
		sprintf(buf, "%s align=%s", buf, opt.align->answer);
		sprintf(buf, "%s rotation=%s", buf, opt.rotation->answer);
		sprintf(buf, "%s linespacing=%s", buf, opt.linespacing->answer);
		if (flag.b->answer)
		    strcat(buf, " -b");
		if (flag.r->answer)
		    strcat(buf, " -r");
		if (flag.s->answer)
		    strcat(buf, " -s");
	    }
	    else
		sprintf(buf, "%s line=1", G_recreate_command());
	    D_add_to_list(buf);
	}

	/* reset */
	R_text_size(5, 5);
	R_text_rotation(0.0);

	R_close_driver();

	exit(EXIT_SUCCESS);
    }

    if (isatty(0))
	fprintf(stdout,
		_
		("\nPlease enter text instructions.  Enter EOF (ctrl-d) on last line to quit\n"));

    cmd_file = G_tempfile();
    if ((cmd_fp = fopen(cmd_file, "w")) == NULL)
	G_fatal_error(_("Unable to open temporary file <%s>"), cmd_file);

    set_x = set_y = set_l = 0;
    first_text = 1;
    linefeed = 1;
    /* do the plotting */
    while (fgets(buf, 512, stdin)) {
	int buf_len;
	char *buf_ptr, *ptr;

	fprintf(cmd_fp, "%s", buf);

	buf_len = strlen(buf) - 1;
	for (; buf[buf_len] == '\r' || buf[buf_len] == '\n'; buf_len--) ;
	buf[buf_len + 1] = 0;

	if (buf[0] == '.' && buf[1] != '.') {
	    int i;
	    double d;

	    G_squeeze(buf);	/* added 6/91 DBS @ CWU */
	    for (buf_ptr = buf + 2; *buf_ptr == ' '; buf_ptr++) ;
	    buf_len = strlen(buf_ptr);

	    switch (buf[1] & 0x7f) {
	    case 'F':
		/* font */
		if ((ptr = strchr(buf_ptr, ':')))
		    *ptr = 0;
		R_font(buf_ptr);
		if (ptr)
		    R_charset(ptr + 1);
		break;
	    case 'C':
		/* color */
		set_color(buf_ptr);
		break;
	    case 'S':
		/* size */
		i = 0;
		if (strchr("+-", buf_ptr[0]))
		    i = 1;
		d = atof(buf_ptr);
		if (buf_ptr[buf_len - 1] != 'p')
#ifdef BACKWARD_COMPATIBILITY
		    d *= (double)(win.b - win.t) / 100.0 / linespacing;
#else
		    d *= (double)(win.b - win.t) / 100.0;
#endif
		size = (int)d + (i ? size : 0);
		R_text_size(size, size);
		break;
	    case 'B':
		/* bold */
		bold = (atoi(buf_ptr) ? 1 : 0);
		break;
	    case 'A':
		/* align */
		strncpy(align, buf_ptr, 2);
		break;
	    case 'R':
		/* rotation */
		i = 0;
		if (strchr("+-", buf_ptr[0]))
		    i = 1;
		d = atof(buf_ptr);
		if (buf_ptr[buf_len - 1] != 'r')
		    d *= M_PI / 180.0;
		d += (i ? rotation : 0.0);
		rotation = fmod(d, 2.0 * M_PI);
		if (rotation < 0.0)
		    rotation += 2.0 * M_PI;
		R_text_rotation((float)(rotation * 180.0 / M_PI));
		break;
	    case 'I':
		/* linespacing */
		linespacing = atof(buf_ptr);
		break;
	    case 'X':
		/* x */
		set_l = 0;
		set_x = 1;
		i = 0;
		if (strchr("+-", buf_ptr[0]))
		    i = 1;
		d = atof(buf_ptr);
		if (buf_ptr[buf_len - 1] == '%')
		    /* percentage */
		    d *= (double)(win.r - win.l) / 100.0;
		else if (buf_ptr[buf_len - 1] != 'p')
		    /* column */
		    d = (d - 1) * size * linespacing + 0.5;
		x = prev_x = (int)d + (i ? x : orig_x);
		break;
	    case 'Y':
		/* y */
		set_l = 0;
		set_y = 1;
		i = 0;
		if (strchr("+-", buf_ptr[0]))
		    i = 1;
		d = atof(buf_ptr);
		if (buf_ptr[buf_len - 1] == '%')
		    /* percentage */
		    d = win.b - d * (double)(win.b - win.t) / 100.0;
		else if (buf_ptr[buf_len - 1] != 'p')
		    /* row */
		    d *= size * linespacing + 0.5;
		y = prev_y = (int)d + (i ? y : orig_y);
		break;
	    case 'L':
		/* linefeed */
		set_l = 1;
		linefeed = (atoi(buf_ptr) ? 1 : 0);
		break;
	    case 'E':
		i = 0;
		if (strchr("+-", buf_ptr[0]))
		    i = 1;
		d = atof(buf_ptr);
		if (buf_ptr[buf_len - 1] == '%')
		    d *= (double)(win.r - win.l) / 100.0;
		else if (buf_ptr[buf_len - 1] != 'p')
		    d = D_u_to_d_col(d);
		x = prev_x = orig_x = (int)d + (i ? orig_x : win.l);
		break;
	    case 'N':
		i = 0;
		if (strchr("+-", buf_ptr[0]))
		    i = 1;
		d = atof(buf_ptr);
		if (buf_ptr[buf_len - 1] == '%')
		    d *= (double)(win.b - win.t) / 100.0;
		else if (buf_ptr[buf_len - 1] != 'p')
		    d = D_u_to_d_row(d);
		y = prev_y = orig_y = (int)d + (i ? orig_y : win.t);
		break;
	    }
	}
	else {
	    buf_ptr = buf;
	    if (buf[0] == '.' && buf[1] == '.')
		buf_ptr++;

	    if (!first_text && (linefeed || set_l)) {
		/* if x is not given, increment x */
		if (!set_x)
		    x = prev_x +
			(int)((size * linespacing + 0.5) * sin(rotation));
		/* if y is not given, increment y */
		if (!set_y)
		    y = prev_y +
			(int)((size * linespacing + 0.5) * cos(rotation));
		prev_x = x;
		prev_y = y;
	    }
	    set_x = set_y = set_l = first_text = 0;

	    draw_text(buf_ptr, &x, &y, size, align, rotation, bold);
	}
    }

    fclose(cmd_fp);

    sprintf(buf, "%s < %s", G_recreate_command(), cmd_file);
    D_add_to_list(buf);

    /* reset */
    R_text_size(5, 5);
    R_text_rotation(0.0);

    R_close_driver();

    exit(EXIT_SUCCESS);
}

static void set_color(char *tcolor)
{
    int r, g, b, color;

    if (sscanf(tcolor, "%d:%d:%d", &r, &g, &b) == 3 ||
	sscanf(tcolor, "0x%02x%02x%02x", &r, &g, &b) == 3) {
	if (r >= 0 && r < 256 && g >= 0 && g < 256 && b >= 0 && b < 256) {
	    R_RGB_color(r, g, b);
	}
    }
    else {
	color = D_translate_color(tcolor);
	if (!color) {
	    G_warning(_("[%s]: No such color. Use '%s'"), tcolor,
		      DEFAULT_COLOR);
	    color = D_translate_color(DEFAULT_COLOR);
	}
	R_standard_color(color);
    }


    return;
}

static int
get_coordinates(int *x, int *y, double *east, double *north,
		struct rectinfo win, char **at, char mouse, char pixel,
		char geocoor)
{
    int i;
    double e, n;

    if (mouse) {
	fprintf(stderr, _("Click!\n"));
	fprintf(stderr, _(" Left:    Place text here\n"));
	fprintf(stderr, _(" Right:   Quit\n"));

	R_get_location_with_pointer(x, y, &i);
	i &= 0x0f;
	if (i != 1)
	    return 1;
	e = D_d_to_u_col((double)*x);
	n = D_d_to_u_row((double)*y);
    }
    else if (at) {
	e = atof(at[0]);
	n = atof(at[1]);
	if (pixel) {
	    *x = e + win.l;
	    *y = n + win.t;
	    e = D_d_to_u_col((double)*x);
	    n = D_d_to_u_row((double)*y);
	}
	else if (geocoor) {
	    *x = (int)D_u_to_d_col(e);
	    *y = (int)D_u_to_d_row(n);
	}
	else {
	    *x = win.l + (int)((win.r - win.l) * e / 100.0);
	    *y = win.t + (int)((win.b - win.t) * (100.0 - n) / 100.0);
	    e = D_d_to_u_col((double)*x);
	    n = D_d_to_u_row((double)*y);
	}
    }
    else
	return 1;

    if (east)
	*east = e;
    if (north)
	*north = n;

    return 0;
}

static void draw_text(char *text, int *x, int *y, int size, char *align,
		      double rotation, char bold)
{
    int w, h;
    int t, b, l, r;
    double c, s;

    /* TODO: get text dimension */
    /* R_get_text_box() does not work with rotation and returns a little bit
     * bigger dimension than actual text size */
    if (rotation != 0.0)
	R_text_rotation(0.0);

    R_get_text_box(text, &t, &b, &l, &r);

    if (rotation != 0.0)
	R_text_rotation((float)(rotation * 180.0 / M_PI));
    w = r - l;
    h = b - t;
    if (w > 0)
	w += 0.2 * size;
    else
	/* R_text() does not draw " ". */
	w = 0.8 * size;
    if (h > 0)
	h += 0.2 * size;
    else
	/* R_text() does not draw " ". */
	h = 0.8 * size;

    c = cos(rotation);
    s = sin(rotation);

    if (strcmp(align, "ll") != 0) {
	switch (align[0]) {
	case 'l':
	    break;
	case 'c':
	    *x += h / 2.0 * s;
	    *y += h / 2.0 * c;
	    break;
	case 'u':
	    *x += h * s;
	    *y += h * c;
	    break;
	}

	switch (align[1]) {
	case 'l':
	    break;
	case 'c':
	    *x -= w / 2.0 * c;
	    *y += w / 2.0 * s;
	    break;
	case 'r':
	    *x -= w * c;
	    *y += w * s;
	    break;
	}
    }

    R_move_abs(*x, *y);
    R_text(text);

    if (bold) {
	R_move_abs(*x, *y + 1);
	R_text(text);
	R_move_abs(*x + 1, *y);
	R_text(text);
    }

    *x += (int)(w * c);
    *y -= (int)(w * s);

    return;
}
