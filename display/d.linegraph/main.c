
/****************************************************************************
 *
 * MODULE:       d.linegraph
 * AUTHOR(S):    Chris Rewerts, Agricultural Engineering, Purdue University (original contributor)
 *               Markus Neteler <neteler itc.it>
 *               Roberto Flor <flor itc.it>, Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>
 *               Vaclav Petras <wenzeslaus gmail com> (various features)
 *
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2016 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* Chris Rewerts
   rewerts@ecn.purdue.edu
   Agricultural Engineering, Purdue University
   February 1992
   program: d.linegraph

   This program is based on Raghaven Srinivasan's modification  
   of the programs written by Dave Johnson for d.histogram. 

   Will read files containing a column of numbers and create line
   graphs. One file can be used for the X axis, up to 10 for the 
   Y axis. Each numerical x,y file should be a single column of
   numbers.    
 */

#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "linegraph.h"

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

/* the default order of precedence of colors to use for Y lines */
int default_y_colors[] = {
    0,
    RED, GREEN, VIOLET, BLUE, ORANGE,
    GRAY, BROWN, MAGENTA, WHITE, INDIGO
};

static double rem(long int x, long int y)
{
    long int d = x / y;

    return ((double)(x - y * d));
}

/* TODO: remove constants */
/* TODO: include X? (common fun for X and Y) */
static void set_optimal_text_size(double text_width, double text_height,
                                  const char *text, double *tt, double *tb,
                                  double *tl, double *tr)
{
    D_text_size(text_width, text_height);
    D_get_text_box(text, tt, tb, tl, tr);
    while ((tt - tb) > YTIC_DIST) {
        text_width *= 0.75;
        text_height *= 0.75;
        D_text_size(text_width, text_height);
        D_get_text_box(text, tt, tb, tl, tr);
    }
}

/* copied from d.vect */
static int cmp(const void *a, const void *b)
{
    return (strcmp(*(char **)a, *(char **)b));
}

/* TODO: this should go to the library with new gisprompt and std opt */
/* copied from d.vect, requires dirent.h */
static char *icon_files(void)
{
    char **list, *ret;
    char buf[GNAME_MAX], path[GPATH_MAX], path_i[GPATH_MAX];
    int i, count;
    size_t len;
    DIR *dir, *dir_i;
    struct dirent *d, *d_i;

    list = NULL;
    len = 0;
    sprintf(path, "%s/etc/symbol", G_gisbase());

    dir = opendir(path);
    if (!dir)
        return NULL;

    count = 0;

    /* loop over etc/symbol */
    while ((d = readdir(dir))) {
        if (d->d_name[0] == '.')
            continue;

        sprintf(path_i, "%s/etc/symbol/%s", G_gisbase(), d->d_name);
        dir_i = opendir(path_i);

        if (!dir_i)
            continue;

        /* loop over each directory in etc/symbols */
        while ((d_i = readdir(dir_i))) {
            if (d_i->d_name[0] == '.')
                continue;

            list = G_realloc(list, (count + 1) * sizeof(char *));

            sprintf(buf, "%s/%s", d->d_name, d_i->d_name);
            list[count++] = G_store(buf);

            len += strlen(d->d_name) + strlen(d_i->d_name) + 2; /* '/' + ',' */
        }

        closedir(dir_i);
    }

    closedir(dir);

    qsort(list, count, sizeof(char *), cmp);

    if (len > 0) {
        ret = G_malloc((len + 1) * sizeof(char));       /* \0 */
        *ret = '\0';
        for (i = 0; i < count; i++) {
            if (i > 0)
                strcat(ret, ",");
            strcat(ret, list[i]);
            G_free(list[i]);
        }
        G_free(list);
    }
    else {
        ret = G_store("");
    }

    return ret;
}

int main(int argc, char **argv)
{
    double xoffset;             /* offset for x-axis */
    double yoffset;             /* offset for y-axis */
    double text_height;
    double text_width;
    int i;
    int j;
    int c;
    int tic_every;
    int max_tics;
    int title_color;
    int num_y_files;
    int tic_unit;
    double t, b, l, r;
    double tt, tb, tl, tr;
    double prev_x, prev_y[11];
    double new_x, new_y[11];
    int line;
    double x_line[3];
    double y_line[3];
    int err;

    struct in_file
    {
        int num_pnts;           /* number of lines in file  */
        int color;              /* color to use for y lines */
        int r, g, b;
        double width;
        float max;              /* maximum value in file    */
        float min;              /* minimum value in file    */
        float value;            /* current value read in    */
        char name[1024];        /* name of file      */
        char full_name[1024];   /* path/name of file    */
        FILE *fp;               /* pointer to file        */
    };

    struct in_file in[12];
    struct GModule *module;

    float max_y;
    float min_y;
    float height, width;
    float xscale;
    float yscale;

    char txt[1024], xlabel[512];
    char tic_name[1024];
    char *name;
    char color_name[20];

    FILE *fopen();

    struct Option *dir_opt, *x_opt, *y_opt;
    struct Option *y_color_opt;
    struct Option *color_table_opt;
    struct Option *line_width_opt;
    struct Option *title[3];
    struct Option *t_color_opt;
    struct Option *y_range_opt;
    struct Option *ytics_opt;
    struct Option *point_symbol_opt;
    struct Option *point_size_opt;
    struct Option *point_color2_opt;
    struct Option *secondary_width_opt;
    struct Flag *do_points_flg, *no_lines_flg;
    struct Option *x_scale_opt, *y_scale_opt;
    struct Flag *x_scale_labels_flg, *y_scale_labels_flg;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    module->description =
        _("Generates and displays simple line graphs in the active graphics monitor display frame.");

    x_opt = G_define_option();
    x_opt->key = "x_file";
    x_opt->description = _("Name of data file for X axis of graph");
    x_opt->type = TYPE_STRING;
    x_opt->required = YES;

    y_opt = G_define_option();
    y_opt->key = "y_file";
    y_opt->description = _("Name of data file(s) for Y axis of graph");
    y_opt->type = TYPE_STRING;
    y_opt->required = YES;
    y_opt->multiple = YES;

    dir_opt = G_define_option();
    dir_opt->key = "directory";
    dir_opt->label = _("Path to files");
    dir_opt->description =
        _("Path to the directory where the input files are located");
    dir_opt->type = TYPE_STRING;
    dir_opt->required = NO;
    /* Remove answer because create problem with full path */
    /* dir_opt->answer = "."; */

    y_color_opt = G_define_option();
    y_color_opt->key = "y_color";
    y_color_opt->description = _("Color for Y data");
    y_color_opt->type = TYPE_STRING;
    y_color_opt->required = NO;
    y_color_opt->multiple = YES;
    y_color_opt->gisprompt = "old_color,color,color";
    y_color_opt->answers = NULL;

    color_table_opt = G_define_standard_option(G_OPT_M_COLR);
    color_table_opt->key = "color_table";
    color_table_opt->guisection = _("Define");

    line_width_opt = G_define_option();
    line_width_opt->key = "width";
    line_width_opt->description = _("Width of the lines");
    line_width_opt->type = TYPE_INTEGER;
    line_width_opt->required = NO;
    line_width_opt->multiple = YES;

    t_color_opt = G_define_option();
    t_color_opt->key = "title_color";
    t_color_opt->description = _("Color for axis, tics, numbers, and title");
    t_color_opt->type = TYPE_STRING;
    t_color_opt->required = NO;
    t_color_opt->gisprompt = "old_color,color,color";
    t_color_opt->answer = DEFAULT_FG_COLOR;

    title[0] = G_define_option();
    title[0]->key = "x_title";
    title[0]->description = _("Title for X data");
    title[0]->type = TYPE_STRING;
    title[0]->required = NO;
    title[0]->answer = "";

    title[1] = G_define_option();
    title[1]->key = "y_title";
    title[1]->description = _("Title for Y data");
    title[1]->type = TYPE_STRING;
    title[1]->required = NO;
    title[1]->answer = "";

    title[2] = G_define_option();
    title[2]->key = "title";
    title[2]->description = _("Title for Graph");
    title[2]->type = TYPE_STRING;
    title[2]->required = NO;
    title[2]->answer = "";

    y_range_opt = G_define_option();
    y_range_opt->key = "y_range";
    y_range_opt->description =
        _("Minimum and maximun value for Y axis (min,max)");
    y_range_opt->type = TYPE_DOUBLE;
    y_range_opt->key_desc = "min,max";
    y_range_opt->required = NO;

    ytics_opt = G_define_option();
    ytics_opt->key = "y_tics";
    ytics_opt->description = _("Tic values for the Y axis");
    ytics_opt->type = TYPE_DOUBLE;
    ytics_opt->required = NO;
    ytics_opt->multiple = YES;

    x_scale_opt = G_define_option();
    x_scale_opt->key = "x_scale";
    x_scale_opt->description = _("Scale for X values");
    x_scale_opt->type = TYPE_DOUBLE;
    x_scale_opt->required = NO;

    y_scale_opt = G_define_option();
    y_scale_opt->key = "y_scale";
    y_scale_opt->description = _("Scale for Y values");
    y_scale_opt->type = TYPE_DOUBLE;
    y_scale_opt->required = NO;

    x_scale_labels_flg = G_define_flag();
    x_scale_labels_flg->key = 'x';
    x_scale_labels_flg->description = "Scale only X labels, not values";

    y_scale_labels_flg = G_define_flag();
    y_scale_labels_flg->key = 'y';
    y_scale_labels_flg->description = "Scale only Y labels, not values";

    point_symbol_opt = G_define_option();
    /* TODO: name must be icon to get GUI dialog */
    point_symbol_opt->key = "icon";
    point_symbol_opt->type = TYPE_STRING;
    point_symbol_opt->required = NO;
    point_symbol_opt->multiple = NO;
    point_symbol_opt->answer = "basic/circle";
    /* This could also use ->gisprompt = "old,symbol,symbol" instead of ->options */
    point_symbol_opt->options = icon_files();
    point_symbol_opt->description = _("Symbol for point");
    point_symbol_opt->guisection = _("Points");

    point_size_opt = G_define_option();
    point_size_opt->key = "point_size";
    point_size_opt->type = TYPE_DOUBLE;
    point_size_opt->required = NO;
    point_size_opt->multiple = NO;
    point_size_opt->answer = "5";
    point_size_opt->label = _("Point size");
    point_size_opt->guisection = _("Points");

    /* theoretically for other things than points */
    point_color2_opt = G_define_standard_option(G_OPT_CN);
    point_color2_opt->key = "secondary_color";
    point_color2_opt->type = TYPE_STRING;
    point_color2_opt->required = NO;
    point_color2_opt->multiple = NO;
    point_color2_opt->description = _("Color for point symbol edge color");
    point_color2_opt->guisection = _("Points");

    /* theoretically for other things than points */
    secondary_width_opt = G_define_option();
    secondary_width_opt->key = "secondary_width";
    secondary_width_opt->description = _("Width of point symbol lines");
    secondary_width_opt->type = TYPE_DOUBLE;
    secondary_width_opt->required = NO;
    secondary_width_opt->multiple = YES;
    secondary_width_opt->answer = "0.1";

    /* TODO: change this to option, use filled/empty symbol option for this */
    do_points_flg = G_define_flag();
    do_points_flg->key = 's';
    do_points_flg->description = "Draw points";
    do_points_flg->guisection = _("Points");

    no_lines_flg = G_define_flag();
    no_lines_flg->key = 'l';
    no_lines_flg->description = "Do not draw lines";

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* TODO: put this to variables, and avoid -Wsign-compare */
    for (i = 0; i < 3; i++) {
        for (j = 0; j < strlen(title[i]->answer); j++)
            if (title[i]->answer[j] == '_')
                title[i]->answer[j] = ' ';
    }

    /* build path to X data file and open for reading
       notice that in[0] will be the X file, and in[1-10]
       will be the Y file(s) */

    if (dir_opt->answer != NULL) {
        sprintf(in[0].full_name, "%s/%s", dir_opt->answer, x_opt->answer);
    }
    else {
        sprintf(in[0].full_name, "%s", x_opt->answer);
    }
    sprintf(in[0].name, "%s", x_opt->answer);

    if ((in[0].fp = fopen(in[0].full_name, "r")) == NULL)
        G_fatal_error(_("Unable to open input file <%s>"), in[0].full_name);

    num_y_files = 0;

    /* open all Y data files */

    for (i = 0, j = 1; (name = y_opt->answers[i]); i++, j++) {

        if (dir_opt->answer != NULL) {
            sprintf(in[j].full_name, "%s/%s", dir_opt->answer, name);
        }
        else {
            sprintf(in[j].full_name, "%s", name);
        }
        sprintf(in[j].name, "%s", name);

        if ((in[j].fp = fopen(in[j].full_name, "r")) == NULL)
            G_fatal_error(_("Unable to open input file <%s>"),
                          in[j].full_name);

        num_y_files++;
        if (num_y_files > 10)
            G_fatal_error(_("Maximum of 10 Y data files exceeded"));
    }

    /* scales */

    int scale_x_values = 0;
    int scale_y_values = 0;
    int scale_x_labels = 0;
    int scale_y_labels = 0;
    double x_scale = 1;
    double y_scale = 1;

    if (x_scale_opt->answer) {
        sscanf(x_scale_opt->answer, "%lf", &x_scale);
        if (x_scale_labels_flg->answer)
            scale_x_labels = 1;
        else
            scale_x_values = 1;
    }
    if (y_scale_opt->answer) {
        sscanf(y_scale_opt->answer, "%lf", &y_scale);
        if (y_scale_labels_flg->answer)
            scale_y_labels = 1;
        else
            scale_y_values = 1;
    }

    /* set colors  */

    title_color = D_translate_color(t_color_opt->answer);

    int draw_lines = TRUE;
    int draw_points = FALSE;

    if (do_points_flg->answer)
        draw_points = TRUE;
    if (no_lines_flg->answer)
        draw_lines = FALSE;

    SYMBOL *point_symbol = NULL;
    double symbol_size;
    double symbol_rotation = 0; /* not supported here */
    int symbol_tolerance = 0;   /* not supported by S_stroke */
    double symbol_line_width;

    if (point_size_opt->answer)
        symbol_size = atof(point_size_opt->answer);

    if (secondary_width_opt->answer)
        symbol_line_width = atof(secondary_width_opt->answer);

    /* TODO: symbol vs point in iface and vars */
    /* there seems there is no free for symbol */
    if (draw_points && point_symbol_opt->answer) {
        point_symbol = S_read(point_symbol_opt->answer);
        /* S_read gives warning only */
        if (!point_symbol)
            G_fatal_error(_("Cannot find/open symbol: '%s'"),
                          point_symbol_opt->answer);
    }
    RGBA_Color primary_color;
    RGBA_Color secondary_color;

    if (draw_points) {
        S_stroke(point_symbol, symbol_size, symbol_rotation,
                 symbol_tolerance);
        primary_color.a = RGBA_COLOR_OPAQUE;

        if (point_color2_opt->answer) {
            int rgb_r, rgb_g, rgb_b;
            int ret =
                G_str_to_color(point_color2_opt->answer, &rgb_r, &rgb_g,
                               &rgb_b);
            if (ret == 0)
                G_fatal_error(_("Color <%s> cannot for option %s be parsed"),
                              point_color2_opt->answer,
                              point_color2_opt->key);
            else if (ret == 2)
                secondary_color.a = RGBA_COLOR_TRANSPARENT;
            else
                secondary_color.a = RGBA_COLOR_OPAQUE;
            secondary_color.r = rgb_r;
            secondary_color.g = rgb_g;
            secondary_color.b = rgb_b;

        }
    }

    /* TODO: use parser for the following and avoid -Wsign-compare */
    /* I had an argument with the parser, and couldn't get a neat list of
       the input colors as I thought I should. I did a quick hack to get
       my list from the answer var, which gives us the colors input
       separated by commas. at least we know that they have been checked against
       the list of possibles */
    c = 0;
    j = 1;
    if (y_color_opt->answer != NULL) {
        for (i = 0; i <= (strlen(y_color_opt->answer)); i++) {
            if ((y_color_opt->answer[i] == ',') ||
                (i == (strlen(y_color_opt->answer)))) {
                color_name[c] = '\0';
                in[j].color = D_translate_color(color_name);
                j++;
                c = 0;
            }
            else {
                color_name[c++] = y_color_opt->answer[i];
            }
        }
        /* in theory we could repeat the colors but that may seem random */
        /* TODO: repeat the colors if only one provided (as with width) */
        if (j - 1 < num_y_files)
            G_fatal_error(_("Only <%d> colors given for <%d> lines"),
                          j - 1, num_y_files);
    }
    else if (color_table_opt->answer) {
        struct Colors colors;

        Rast_init_colors(&colors);
        Rast_make_colors(&colors, color_table_opt->answer, 1, num_y_files);

        int *values = G_malloc(num_y_files * sizeof(int));
        unsigned char *rbuf = G_malloc(num_y_files * sizeof(unsigned char));
        unsigned char *gbuf = G_malloc(num_y_files * sizeof(unsigned char));
        unsigned char *bbuf = G_malloc(num_y_files * sizeof(unsigned char));
        unsigned char *set = G_malloc(num_y_files * sizeof(unsigned char));

        for (i = 0; i < num_y_files; i++)
            values[i] = i + 1;
        Rast_lookup_c_colors(values, rbuf, gbuf, bbuf, set, num_y_files,
                             &colors);
        /* no need to check 'set' because we generated the range */
        for (i = 0; i < num_y_files; i++) {
            /* the in list is indexed from 1 */
            in[i + 1].r = rbuf[i];
            in[i + 1].g = gbuf[i];
            in[i + 1].b = bbuf[i];
        }
        G_free(rbuf);
        G_free(gbuf);
        G_free(bbuf);
        G_free(set);
    }
    else
        /* no colors given on command line, use default list */
    {
        for (i = 1; i <= num_y_files; i++) {
            in[i].color = default_y_colors[i];
        }
    }

    if (line_width_opt->answer) {
        i = 0;
        while (line_width_opt->answers[i]) {
            /* we could relax this and just stop/warn reading as with the colors */
            if (i + 1 > num_y_files)
                G_fatal_error(_("Number of widths (%d) is higher then"
                                " the number of files (%d)"), i + 1,
                              num_y_files);
            /* TODO: remove indexing from 1 in the whole file */
            in[i + 1].width = atof(line_width_opt->answers[i]);
            i++;
        }
        if (i == 1) {
            for (j = 1; j <= num_y_files; j++) {
                in[j].width = atof(line_width_opt->answer);
            }
        }
        else if (num_y_files != i)
            G_fatal_error(_("Number of widths (%d) is lower then"
                            " the number of files (%d)"), i, num_y_files);
    }

    /* get coordinates of current screen window, in pixels */
    D_open_driver();

    D_setup_unity(0);
    D_get_src(&t, &b, &l, &r);

    /* this seems to be the width when none set */
    double default_width = 2;

    D_line_width(default_width);

    /* create axis lines, to be drawn later */
    height = b - t;
    width = r - l;
    x_line[0] = x_line[1] = l + (ORIGIN_X * width);
    x_line[2] = l + (XAXIS_END * width);
    y_line[0] = b - (YAXIS_END * height);
    y_line[1] = y_line[2] = b - (ORIGIN_Y * height);
    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH;
    D_text_size(text_width, text_height);

    /* read thru each data file in turn, find max and min values for
       each, count lines, find x min and max, find overall y min and
       max */

    max_y = -99999.9;
    min_y = 99999.9;

    for (i = 0; i <= num_y_files; i++) {

        in[i].min = 99999.9;
        in[i].max = -99999.9;
        in[i].value = 0.0;
        in[i].num_pnts = 0;

        while ((err = fscanf(in[i].fp, "%f", &in[i].value)) != EOF) {
            if (scale_y_values)
                in[i].value *= y_scale;
            in[i].num_pnts++;
            in[i].max = MAX(in[i].max, in[i].value);
            in[i].min = MIN(in[i].min, in[i].value);
            if (i > 0) {        /* if we have a y file */
                min_y = MIN(min_y, in[i].value);
                max_y = MAX(max_y, in[i].value);
            }
        }
        if ((i > 0) && (in[0].num_pnts != in[i].num_pnts)) {
            if (in[i].num_pnts < in[0].num_pnts) {
                G_warning(_("Y input file <%s> contains fewer data points than the X input file"),
                          in[i].name);
            }
            else {
                G_warning(_("Y input file <%s> contains more data points than the X input file"),
                          in[i].name);
            }

            if (in[i].num_pnts > in[0].num_pnts)
                G_message(n_("The last point will be ignored",
                             "The last %d points will be ignored",
                             (in[i].num_pnts - in[0].num_pnts)),
                          (in[i].num_pnts - in[0].num_pnts));
        }
    }

    /* TODO: parse range option function */
    /* parse and set y min max */
    if (y_range_opt->answer != NULL) {
        /* all checks should be done by the parser */
        sscanf(y_range_opt->answers[0], "%f", &min_y);
        sscanf(y_range_opt->answers[1], "%f", &max_y);
        if (min_y > max_y) {
            /* swap values to tolerate some errors */
            double d_tmp = max_y;

            max_y = min_y;
            min_y = d_tmp;
        }
    }

    /* close all files */

    for (i = 0; i <= num_y_files; i++)
        fclose(in[i].fp);

    /* figure scaling factors and offsets */

    xscale = ((double)(x_line[2] - x_line[1]) / (double)(in[0].num_pnts));
    yscale = ((double)(y_line[1] - y_line[0]) / (max_y - min_y));
    yoffset = (double)(y_line[1]);
    xoffset = (double)(x_line[1]);

    /* figure tic_every and tic_units for the x-axis of the bar-chart.
       tic_every tells how often to place a tic-number.  tic_unit tells
       the unit to use in expressing tic-numbers. */

    if (xscale < XTIC_DIST) {
        max_tics = (x_line[2] - x_line[1]) / XTIC_DIST;
        i = 1;
        while (((in[0].max - in[0].min) / tics[i].every) > max_tics)
            i++;
        tic_every = tics[i].every;
        tic_unit = tics[i].unit;
        strcpy(tic_name, tics[i].name);
    }
    else {
        tic_every = 1;
        tic_unit = 1;
        strcpy(tic_name, "");
    }

    if (tic_unit != 1 && scale_x_labels)
        G_fatal_error(_("Scale X labels cannot be used with this range"
                        " of data (%f, %f)"), in[0].min, in[0].max);

    /* open all the data files again */

    for (i = 0; i <= num_y_files; i++) {
        if ((in[i].fp = fopen(in[i].full_name, "r")) == NULL) {
            D_close_driver();
            G_fatal_error(_("Unable to open input file <%s>"),
                          in[i].full_name);
        }
    }

    /* loop through number of lines in x data file, 
       then loop thru for each y file, drawing a piece of each line and a
       legend bar on each iteration evenly divisible, a tic-mark
       on those evenly divisible by tic_unit, and a tic_mark
       number on those evenly divisible by tic_every   */

    /* read the info from the inputs */

    for (line = 0; line < in[0].num_pnts; line++) {
        /* scan in an X value */
        err = fscanf(in[0].fp, "%f", &in[0].value);
        if (scale_x_values)
            in[0].value *= x_scale;

        /* didn't find a number or hit EOF before our time */
        if ((err != 1) || (err == EOF)) {
            D_close_driver();
            G_fatal_error(_("Problem reading X data file at line %d"), line);
        }

        /* for each Y data file, get a value and compute where to draw it */
        for (i = 1; i <= num_y_files; i++) {
            /* check to see that we do indeed have data for this point */
            if (line < in[i].num_pnts) {
                err = fscanf(in[i].fp, "%f", &in[i].value);
                if (scale_y_values)
                    in[i].value *= y_scale;
                if ((in[i].num_pnts >= line) && (err != 1)) {
                    D_close_driver();
                    G_fatal_error(_("Problem reading <%s> data file at line %d"),
                                  in[i].name, line);
                }

                /* in case the Y file has fewer lines than the X file, we will skip
                   trying to draw when we run out of data */

                /* draw increment of each Y file's data */

                /* find out position of where Y should be drawn. */
                /* if our minimum value of y is not negative, this is easy */

                if (min_y >= 0)
                    new_y[i] = (yoffset - yscale * (in[i].value - min_y));

                /* if our minimum value of y is negative, then we have two
                   cases:  our current value to plot is pos or neg */

                else {
                    if (in[i].value < 0)
                        new_y[i] = (yoffset - yscale * (-1 *
                                                        (min_y -
                                                         in[i].value)));
                    else
                        new_y[i] = (yoffset - yscale * (in[i].value +
                                                        (min_y * -1)));
                }

                new_x = xoffset + (line * xscale);

                /* draw only when we the previous point to start from */
                if (draw_lines && line > 0) {
                    if (color_table_opt->answer)
                        D_RGB_color(in[i].r, in[i].g, in[i].b);
                    else
                        D_use_color(in[i].color);
                    if (line_width_opt->answer)
                        D_line_width(in[i].width);
                    D_line_abs(prev_x, prev_y[i], new_x, new_y[i]);
                }
                /* draw points after lines, last point after last line */
                if (draw_points && line > 0) {
                    if (color_table_opt->answer) {
                        primary_color.r = in[i].r;
                        primary_color.g = in[i].g;
                        primary_color.b = in[i].b;
                    }
                    else {
                        /* TODO: do this ahead. store in .r .g .b in .rgb */
                        int rgb_r, rgb_g, rgb_b;

                        D_color_number_to_RGB(in[i].color, &rgb_r, &rgb_g,
                                              &rgb_b);
                        primary_color.r = rgb_r;
                        primary_color.g = rgb_g;
                        primary_color.b = rgb_b;
                    }
                    D_line_width(symbol_line_width);
                    D_symbol2(point_symbol, prev_x, prev_y[i], &primary_color,
                              &secondary_color);
                    /* last point */
                    if (line == in[i].num_pnts - 1)
                        D_symbol2(point_symbol, new_x, new_y[i],
                                  &primary_color, &secondary_color);
                }
                prev_y[i] = new_y[i];
            }
        }
        prev_x = new_x;

        /* draw x-axis tic-marks and numbers */

        /* default width for the tics */
        D_line_width(default_width);

        if (rem((long int)in[0].value, tic_every) == 0.0) {

            /* draw a numbered tic-mark */

            D_use_color(title_color);
            D_begin();
            D_move_abs(xoffset + line * xscale, b - ORIGIN_Y * (b - t));
            D_cont_rel(0, BIG_TIC * (b - t));
            D_end();
            D_stroke();

            double value = in[0].value;

            /* the scale goes against the auto units scaling
             * (but doing the scaling before would place the values
             * differently which is not what we want with label scaling
             */
            if (scale_x_labels)
                value *= x_scale;
            if ((value >= 1) || (value <= -1) || (value == 0))
                sprintf(txt, "%.0f", (value / tic_unit));
            else
                sprintf(txt, "%.2f", (value));
            text_height = (b - t) * TEXT_HEIGHT;
            text_width = (r - l) * TEXT_WIDTH;
            D_text_size(text_width, text_height);
            D_get_text_box(txt, &tt, &tb, &tl, &tr);
            while ((tr - tl) > XTIC_DIST) {
                text_width *= 0.75;
                text_height *= 0.75;
                D_text_size(text_width, text_height);
                D_get_text_box(txt, &tt, &tb, &tl, &tr);
            }
            D_pos_abs((xoffset + (line * xscale - (tr - tl) / 2)),
                      (b - XNUMS_Y * (b - t)));
            D_text(txt);
        }
        else if (rem(line, tic_unit) == 0.0) {

            /* draw a tic-mark */

            D_use_color(title_color);
            D_begin();
            D_move_abs(xoffset + line * xscale, b - ORIGIN_Y * (b - t));
            D_cont_rel(0, SMALL_TIC * (b - t));
            D_end();
            D_stroke();
        }
    }

    /* reset so the following doesn't use the special width */
    D_line_width(default_width);

    /* close all input files */
    for (i = 0; i <= num_y_files; i++) {
        fclose(in[i].fp);
    }

    /* draw the x-axis label */
    if ((strcmp(title[0]->answer, "") == 0) && (strcmp(tic_name, "") == 0))
        *xlabel = '\0';
    else
        sprintf(xlabel, "X: %s %s", title[0]->answer, tic_name);
    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH * 1.5;
    D_text_size(text_width, text_height);
    D_get_text_box(xlabel, &tt, &tb, &tl, &tr);
    D_pos_abs((l + (r - l) / 2 - (tr - tl) / 2), (b - LABEL_1 * (b - t)));
    D_use_color(title_color);
    D_text(xlabel);

    /* DRAW Y-AXIS TIC-MARKS AND NUMBERS
       first, figure tic_every and tic_units for the x-axis of the bar-chart.
       tic_every tells how often to place a tic-number.  tic_unit tells
       the unit to use in expressing tic-numbers. */

    /* user versus automatic Y tics */
    if (ytics_opt->answer) {
        /* user-provided Y tics, no intermediate tics supported */
        char *text;

        i = 0;
        while ((text = ytics_opt->answers[i])) {
            i++;

            double val = atof(text);

            /* using original user's text for the text later */

            /* for scripting convenience ignore out of range */
            if (val < min_y || val > max_y) {
                G_debug(2, "tic %f out of range %f,%f", val, min_y, max_y);
                continue;
            }
            /* we don't care about order, so just continue, not break */

            D_begin();
            D_move_abs(x_line[0], yoffset - yscale * val);
            D_cont_rel((-(r - l) * BIG_TIC), 0);
            D_end();
            D_stroke();

            /* draw a tic-mark number */
            text_height = (b - t) * TEXT_HEIGHT;
            text_width = (r - l) * TEXT_WIDTH;
            /* this would be useful, but with some other numbers */
            set_optimal_text_size(text_width, text_height, txt, &tt, &tb, &tl,
                                  &tr);
            D_pos_abs(l + (r - l) * YNUMS_X - (tr - tl) / 2,
                      yoffset - (yscale * val + 0.5 * (tt - tb)));
            D_text(text);
        }
        /* no automatic tics comment */
        strcpy(tic_name, "");
    }
    else {
        /* automatic Y tics, decimal places (e.g. range 0-1) not supported */
        if (yscale < YTIC_DIST) {
            max_tics = (y_line[1] - y_line[0]) / YTIC_DIST;
            i = 1;
            while (((max_y - min_y) / tics[i].every) > max_tics)
                i++;
            tic_every = tics[i].every;
            tic_unit = tics[i].unit;
            strcpy(tic_name, tics[i].name);
        }
        else {
            tic_every = 1;
            tic_unit = 1;
            strcpy(tic_name, "");
        }
        if (tic_unit != 1 && scale_y_labels)
            G_fatal_error(_("Scale Y labels cannot be used with this"
                            " range of data (%f, %f)"), min_y, max_y);
        /* Y-AXIS LOOP */
        for (i = (int)min_y; i <= (int)max_y; i += tic_unit) {
            if (rem(i, tic_every) == 0.0) {
                /* draw a tic-mark */

                D_begin();
                D_move_abs(x_line[0], yoffset - yscale * (i - min_y));
                D_cont_rel(-(r - l) * BIG_TIC, 0);
                D_end();
                D_stroke();

                /* draw a tic-mark number */

                if (scale_y_labels)
                    sprintf(txt, "%f.0", (i / tic_unit * y_scale));
                else
                    sprintf(txt, "%d", (i / tic_unit));
                text_height = (b - t) * TEXT_HEIGHT;
                text_width = (r - l) * TEXT_WIDTH;
                set_optimal_text_size(text_width, text_height, txt, &tt, &tb,
                                      &tl, &tr);
                D_pos_abs(l + (r - l) * YNUMS_X - (tr - tl) / 2,
                          yoffset - (yscale * (i - min_y) + 0.5 * (tt - tb)));
                D_text(txt);
            }
            else if (rem(i, tic_unit) == 0.0) {
                /* draw a tic-mark */
                D_begin();
                D_move_abs(x_line[0], (yoffset - yscale * (i - min_y)));
                D_cont_rel(-(r - l) * SMALL_TIC, 0);
                D_end();
                D_stroke();
            }
        }
    }

    /* draw the y-axis label */
    if ((strcmp(title[1]->answer, "") == 0) && (strcmp(tic_name, "") == 0))
        *xlabel = '\0';
    else
        sprintf(xlabel, "Y: %s %s", title[1]->answer, tic_name);
    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH * 1.5;
    D_text_size(text_width, text_height);
    D_get_text_box(xlabel, &tt, &tb, &tl, &tr);
    D_pos_abs(l + (r - l) / 2 - (tr - tl) / 2, b - LABEL_2 * (b - t));
    D_use_color(title_color);
    D_text(xlabel);

    /* top label */
    sprintf(xlabel, "%s", title[2]->answer);
    text_height = (b - t) * TEXT_HEIGHT;
    text_width = (r - l) * TEXT_WIDTH * 2.0;
    D_text_size(text_width, text_height);
    D_get_text_box(xlabel, &tt, &tb, &tl, &tr);
    /*
       D_move_abs((int)(((r-l)/2)-(tr-tl)/2),
       (int) (t+ (b-t)*.07) );
     */
    D_pos_abs(l + (r - l) / 2 - (tr - tl) / 2, t + (b - t) * .07);
    D_use_color(title_color);
    D_text(xlabel);

    /* draw x and y axis lines */
    D_use_color(title_color);
    D_polyline_abs(x_line, y_line, 3);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
