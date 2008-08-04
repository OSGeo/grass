/*
 * Start up graphics processing.  Anything that needs to be assigned, set up,
 * started-up, or otherwise initialized happens here.  This is called only at
 * the startup of the graphics driver.
 *
 * The external variables define the pixle limits of the graphics surface.  The
 * coordinate system used by the applications programs has the (0,0) origin
 * in the upper left-hand corner.  Hence,
 *    screen_left < screen_right
 *    screen_top  < screen_bottom 
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include <grass/gis.h>
#include "psdriver.h"

#define DATE_FORMAT "%c"

const char *file_name;
FILE *outfp;
int true_color;
int width, height;
int encapsulated;
int no_header, no_trailer;

static int landscape;
static int left, right, bot, top;

struct paper
{
    const char *name;
    double width, height;
    double left, right, bot, top;
};

static const struct paper papers[] = {
    /* name         width   height  left    right   bottom  top */
    {"a4", 8.268, 11.693, 0.5, 0.5, 1.0, 1.0},
    {"a3", 11.693, 16.535, 0.5, 0.5, 1.0, 1.0},
    {"a2", 16.54, 23.39, 1.0, 1.0, 1.0, 1.0},
    {"a1", 23.39, 33.07, 1.0, 1.0, 1.0, 1.0},
    {"a0", 33.07, 46.77, 1.0, 1.0, 1.0, 1.0},
    {"us-legal", 8.5, 14.0, 1.0, 1.0, 1.0, 1.0},
    {"us-letter", 8.5, 11.0, 1.0, 1.0, 1.0, 1.0},
    {"us-tabloid", 11.0, 17.0, 1.0, 1.0, 1.0, 1.0},
    {NULL, 0, 0, 0, 0, 0, 0}
};

static void write_prolog(void)
{
    char prolog_file[GPATH_MAX];
    char date_str[256];
    FILE *prolog_fp;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(date_str, sizeof(date_str), DATE_FORMAT, tm);

    sprintf(prolog_file, "%s/etc/psdriver.ps", G_gisbase());

    prolog_fp = fopen(prolog_file, "r");
    if (!prolog_fp)
	G_fatal_error("Unable to open prolog file");

    if (encapsulated)
	output("%%!PS-Adobe-3.0 EPSF-3.0\n");
    else
	output("%%!PS-Adobe-3.0\n");

    output("%%%%LanguageLevel: %d\n", 3);
    output("%%%%Creator: GRASS PS Driver\n");
    output("%%%%Title: %s\n", file_name);
    output("%%%%For: %s\n", G_whoami());
    output("%%%%Orientation: %s\n", landscape ? "Landscape" : "Portrait");
    output("%%%%BoundingBox: %d %d %d %d\n", left, bot, right, top);
    output("%%%%CreationDate: %s\n", date_str);
    output("%%%%EndComments\n");

    output("%%%%BeginProlog\n");
    while (!feof(prolog_fp)) {
	char buf[256];

	if (!fgets(buf, sizeof(buf), prolog_fp))
	    break;

	fputs(buf, outfp);
    }
    output("%%%%EndProlog\n");

    fclose(prolog_fp);
}

void write_setup(void)
{
    output("%%%%BeginSetup\n");

    output("%d %d translate\n", left, bot);

    if (landscape)
	output("90 rotate 0 1 -1 scale\n");
    else
	output("0 %d translate 1 -1 scale\n", height);

    output("%d %d BEGIN\n", width, height);

    output("%%%%EndSetup\n");
    output("%%%%Page: 1 1\n");
}

static int in2pt(double x)
{
    return (int)(x * 72);
}

static void swap(int *x, int *y)
{
    int tmp = *x;

    *x = *y;
    *y = tmp;
}

static void get_paper(void)
{
    const char *name = getenv("GRASS_PAPER");
    const struct paper *paper;
    int i;

    width = screen_right - screen_left;
    height = screen_bottom - screen_top;

    left = 0;
    right = width;
    bot = 0;
    top = height;

    if (landscape)
	swap(&right, &top);

    if (!name)
	return;

    for (i = 0;; i++) {
	paper = &papers[i];

	if (!paper->name)
	    return;

	if (G_strcasecmp(name, paper->name) == 0)
	    break;
    }

    left = in2pt(paper->left);
    right = in2pt(paper->width) - in2pt(paper->right);
    bot = in2pt(paper->bot);
    top = in2pt(paper->height) - in2pt(paper->top);

    width = right - left;
    height = in2pt(paper->height) - in2pt(paper->top) - in2pt(paper->bot);

    if (landscape)
	swap(&width, &height);

    screen_right = screen_left + width;
    screen_bottom = screen_top + height;
}

int PS_Graph_set(int argc, char **argv)
{
    const char *p;

    G_gisinit("PS driver");

    p = getenv("GRASS_PSFILE");
    if (!p || strlen(p) == 0)
	p = FILE_NAME;

    file_name = p;
    p = file_name + strlen(file_name) - 4;
    encapsulated = (G_strcasecmp(p, ".eps") == 0);

    p = getenv("GRASS_TRUECOLOR");
    true_color = p && strcmp(p, "TRUE") == 0;

    p = getenv("GRASS_LANDSCAPE");
    landscape = p && strcmp(p, "TRUE") == 0;

    p = getenv("GRASS_PS_HEADER");
    no_header = p && strcmp(p, "FALSE") == 0;

    p = getenv("GRASS_PS_TRAILER");
    no_trailer = p && strcmp(p, "FALSE") == 0;

    G_message("PS: GRASS_TRUECOLOR status: %s",
	      true_color ? "TRUE" : "FALSE");

    get_paper();

    init_color_table();

    outfp = fopen(file_name, no_header ? "a" : "w");

    if (!outfp)
	G_fatal_error("Unable to open output file: %s", file_name);

    if (!no_header) {
	write_prolog();
	write_setup();
    }

    G_message
	("PS: collecting to file: %s,\n     GRASS_WIDTH=%d, GRASS_HEIGHT=%d",
	 file_name, width, height);

    fflush(outfp);

    return 0;
}

void output(const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    vfprintf(outfp, fmt, va);
    va_end(va);
}
