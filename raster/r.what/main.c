
/****************************************************************************
 *
 * MODULE:       r.what
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,Brad Douglas <rez touchofmadness.com>,
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>, Soeren Gebbert <soeren.gebbert gmx.de>
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006, 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#define NFILES 400

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

struct order
{
    int point;
    int row;
    int col;
    char north_buf[256];
    char east_buf[256];
    char lab_buf[256];
    char clr_buf[NFILES][256];
    CELL value[NFILES];
    DCELL dvalue[NFILES];
};

static int oops(int, const char *, const char *);
static int by_row(const void *, const void *);
static int by_point(const void *, const void *);

static int tty = 0;


int main(int argc, char *argv[])
{
    int i, j;
    int nfiles;
    int fd[NFILES];
    struct Categories cats[NFILES];
    struct Cell_head window;
    struct Colors ncolor[NFILES];
    struct Colors colors;
    RASTER_MAP_TYPE out_type[NFILES];
    CELL *cell[NFILES];
    DCELL *dcell[NFILES];
    struct Map_info Map;
    struct line_pnts *Points;
    
    /*   int row, col; */
    double drow, dcol;
    int row_in_window, in_window;
    double east, north;
    int line, ltype;
    char buffer[1024];
    char **ptr;
    struct _opt {
        struct Option *input, *cache, *null, *coords, *fs, *points;
    } opt;
    struct _flg {
	struct Flag *label, *cache, *cat_int, *color, *header;
    } flg;
    char fs;
    int Cache_size;
    int done = FALSE;
    int point, point_cnt;
    struct order *cache;
    int cur_row;
    int cache_hit = 0, cache_miss = 0;
    int cache_hit_tot = 0, cache_miss_tot = 0;
    int pass = 0;
    int cache_report = FALSE;
    char tmp_buf[500], *null_str;
    int red, green, blue;
    struct GModule *module;


    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("querying"));
    G_add_keyword(_("position"));
    module->description =
	_("Queries raster maps on their category values and category labels.");

    opt.input = G_define_standard_option(G_OPT_R_MAPS);
    opt.input->description = _("Name of existing raster map(s) to query");

    opt.coords = G_define_standard_option(G_OPT_M_COORDS);
    opt.coords->description = _("Coordinates for query");
    opt.coords->guisection = _("Query");

    opt.points = G_define_standard_option(G_OPT_V_MAP);
    opt.points->key = "points";
    opt.points->label = _("Name of vector points map for query");
    opt.points->required = NO;
    opt.points->guisection = _("Query");
    
    opt.null = G_define_option();
    opt.null->key = "null";
    opt.null->type = TYPE_STRING;
    opt.null->required = NO;
    opt.null->answer = "*";
    opt.null->description = _("Char string to represent no data cell");
    opt.null->guisection = _("Print");
    
    opt.fs = G_define_standard_option(G_OPT_F_SEP);
    opt.fs->guisection = _("Print");

    opt.cache = G_define_option();
    opt.cache->key = "cache";
    opt.cache->type = TYPE_INTEGER;
    opt.cache->required = NO;
    opt.cache->multiple = NO;
    opt.cache->description = _("Size of point cache");
    opt.cache->answer = "500";
    opt.cache->guisection = _("Advanced");
    
    flg.header = G_define_flag();
    flg.header->key = 'n';
    flg.header->description = _("Output header row");
    flg.header->guisection = _("Print");

    flg.label = G_define_flag();
    flg.label->key = 'f';
    flg.label->description = _("Show the category labels of the grid cell(s)");
    flg.label->guisection = _("Print");

    flg.color = G_define_flag();
    flg.color->key = 'r';
    flg.color->description = _("Output color values as RRR:GGG:BBB");
    flg.color->guisection = _("Print");

    flg.cat_int = G_define_flag();
    flg.cat_int->key = 'i';
    flg.cat_int->description = _("Output integer category values, not cell values");
    flg.cat_int->guisection = _("Print");

    flg.cache = G_define_flag();
    flg.cache->key = 'c';
    flg.cache->description = _("Turn on cache reporting");
    flg.cache->guisection = _("Advanced");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    tty = isatty(0);

    fs = G_option_to_separator(opt.fs);
    
    null_str = opt.null->answer;

    if (tty)
	Cache_size = 1;
    else
	Cache_size = atoi(opt.cache->answer);

    if (Cache_size < 1)
	Cache_size = 1;

    cache = (struct order *)G_malloc(sizeof(struct order) * Cache_size);

    /* enable cache report */
    if (flg.cache->answer)
	cache_report = TRUE;

    /* open raster maps to query */
    ptr = opt.input->answers;
    nfiles = 0;
    for (; *ptr != NULL; ptr++) {
	char name[GNAME_MAX];

	if (nfiles >= NFILES)
	    G_fatal_error(_("Can only do up to %d raster maps (%d given)"),
			  NFILES, nfiles);

	strcpy(name, *ptr);
	fd[nfiles] = Rast_open_old(name, "");

	out_type[nfiles] = Rast_get_map_type(fd[nfiles]);
	if (flg.cat_int->answer)
	    out_type[nfiles] = CELL_TYPE;

	if (flg.color->answer) {
	    Rast_read_colors(name, "", &colors);
	    ncolor[nfiles] = colors;
	}

	if (flg.label->answer && Rast_read_cats(name, "", &cats[nfiles]) < 0)
	    G_fatal_error(_("Unable to read category file for <%s>"), name);

	nfiles++;
    }

    /* allocate row buffers */
    for (i = 0; i < nfiles; i++) {
	if (flg.cat_int->answer)
	    out_type[i] = CELL_TYPE;

	cell[i] = Rast_allocate_c_buf();
	if (out_type[i] != CELL_TYPE)
	    dcell[i] = Rast_allocate_d_buf();
    }

    /* open vector points map */
    if (opt.points->answer) {
        Vect_set_open_level(1); /* topology not required */
        if (Vect_open_old(&Map, opt.points->answer, "") < 0)
            G_fatal_error(_("Unable to open vector map <%s>"), opt.points->answer);
    }
    Points = Vect_new_line_struct();
    G_get_window(&window);

    /* print header row */
    if(flg.header->answer) {
	fprintf(stdout, "easting%cnorthing%csite_name", fs, fs);

	ptr = opt.input->answers;
	for (; *ptr != NULL; ptr++) {
	    char name[GNAME_MAX];
	    strcpy(name, *ptr);

	    fprintf(stdout, "%c%s", fs, name);

	    if (flg.label->answer)
		fprintf(stdout, "%c%s_label", fs, name);
	    if (flg.color->answer)
		fprintf(stdout, "%c%s_color", fs, name);
	}

	fprintf(stdout, "\n");
    }

    line = 0;
    if (!opt.coords->answers && !opt.points->answers && tty)
	fprintf(stderr, "enter points, \"end\" to quit\n");

    j = 0;
    done = FALSE;
    while (!done) {
	pass++;
	if (cache_report & !tty)
	    fprintf(stderr, "Pass %3d  Line %6d   - ", pass, line);

	cache_hit = cache_miss = 0;

	if (!opt.coords->answers && !opt.points->answers && tty) {
	    fprintf(stderr, "\neast north [label] >  ");
	    Cache_size = 1;
	}
	{
	    point_cnt = 0;
	    for (i = 0; i < Cache_size; i++) {
		if (!opt.coords->answers && !opt.points->answers &&
                    fgets(buffer, 1000, stdin) == NULL)
		    done = TRUE;
		else {
		    line++;
		    if ((!opt.coords->answers && !opt.points->answers &&
			 (strncmp(buffer, "end\n", 4) == 0 ||
			  strncmp(buffer, "exit\n", 5) == 0)) ||
			(opt.coords->answers && !opt.coords->answers[j])) {
			done = TRUE;
                    }
		    else {
                        if (opt.points->answer) {
                            ltype = Vect_read_next_line(&Map, Points, NULL);
                            if (ltype == -1)
                                G_fatal_error(_("Unable to read vector map <%s>"), Vect_get_full_name(&Map));
                            else if (ltype == -2)
                                done = TRUE;
                            else if (!(ltype & GV_POINTS)) {
                                G_warning(_("Line %d is not point or centroid, skipped"), line);
                                continue;
                            }
                            else {
                                east = Points->x[0];
                                north = Points->y[0];
                                sprintf(cache[point_cnt].east_buf, "%f", east);
                                sprintf(cache[point_cnt].north_buf, "%f", north);
                            }
                        }
                        else {
                            *(cache[point_cnt].lab_buf) =
                                *(cache[point_cnt].east_buf) =
                                *(cache[point_cnt].north_buf) = 0;
                            if (!opt.coords->answers)
                                sscanf(buffer, "%s %s %[^\n]",
                                       cache[point_cnt].east_buf,
                                       cache[point_cnt].north_buf,
                                       cache[point_cnt].lab_buf);
                            else {
                                strcpy(cache[point_cnt].east_buf,
                                       opt.coords->answers[j++]);
                                strcpy(cache[point_cnt].north_buf,
                                       opt.coords->answers[j++]);
                            }
                            if (*(cache[point_cnt].east_buf) == 0)
                                continue;	/* skip blank lines */
                            
                            if (*(cache[point_cnt].north_buf) == 0) {
                                oops(line, buffer,
                                     "two coordinates (east north) required");
                                continue;
                            }
                        
                            
                            if (!G_scan_northing(cache[point_cnt].north_buf, &north, window.proj) ||
                                !G_scan_easting(cache[point_cnt].east_buf, &east, window.proj)) {
                                oops(line, buffer, "invalid coordinate(s)");
                                continue;
                            }
                        }
                        
			/* convert north, east to row and col */
			drow = Rast_northing_to_row(north, &window);
			dcol = Rast_easting_to_col(east, &window);
                        
			/* a special case.
			 *   if north falls at southern edge, or east falls on eastern edge,
			 *   the point will appear outside the window.
			 *   So, for these edges, bring the point inside the window
			 */
			if (drow == window.rows)
			    drow--;
			if (dcol == window.cols)
			    dcol--;

			cache[point_cnt].row = (int)drow;
			cache[point_cnt].col = (int)dcol;
			cache[point_cnt].point = point_cnt;
			point_cnt++;
		    }
		}
	    }
	}

	if (Cache_size > 1)
	    qsort(cache, point_cnt, sizeof(struct order), by_row);

	/* extract data from files and store in cache */

	cur_row = -99;

	for (point = 0; point < point_cnt; point++) {
	    row_in_window = 1;
	    in_window = 1;
	    if (cache[point].row < 0 || cache[point].row >= window.rows)
		row_in_window = in_window = 0;
	    if (cache[point].col < 0 || cache[point].col >= window.cols)
		in_window = 0;

	    if (!in_window) {
		if (tty)
		    G_warning(_("%s %s is outside your current region"),
			      cache[point].east_buf, cache[point].north_buf);
	    }

	    if (cur_row != cache[point].row) {
		cache_miss++;
		if (row_in_window)
		    for (i = 0; i < nfiles; i++) {
			Rast_get_c_row(fd[i], cell[i], cache[point].row);

			if (out_type[i] != CELL_TYPE)
			    Rast_get_d_row(fd[i], dcell[i], cache[point].row);
		    }

		cur_row = cache[point].row;
	    }
	    else
		cache_hit++;

	    for (i = 0; i < nfiles; i++) {
		if (in_window)
		    cache[point].value[i] = cell[i][cache[point].col];
		else
		    Rast_set_c_null_value(&(cache[point].value[i]), 1);

		if (out_type[i] != CELL_TYPE) {
		    if (in_window)
			cache[point].dvalue[i] = dcell[i][cache[point].col];
		    else
			Rast_set_d_null_value(&(cache[point].dvalue[i]), 1);
		}
		if (flg.color->answer) {
		    if (out_type[i] == CELL_TYPE)
			Rast_get_c_color(&cell[i][cache[point].col],
					     &red, &green, &blue, &ncolor[i]);
		    else
			Rast_get_d_color(&dcell[i][cache[point].col],
					     &red, &green, &blue, &ncolor[i]);

		    sprintf(cache[point].clr_buf[i], "%03d:%03d:%03d", red,
			    green, blue);
		}

	    }
	}			/* point loop */

	if (Cache_size > 1)
	    qsort(cache, point_cnt, sizeof(struct order), by_point);

	/* report data from re-ordered cache */

	for (point = 0; point < point_cnt; point++) {

	    G_debug(1, "%s|%s at col %d, row %d\n",
		    cache[point].east_buf, cache[point].north_buf,
		    cache[point].col, cache[point].row);


	    fprintf(stdout, "%s%c%s%c%s", cache[point].east_buf, fs,
		    cache[point].north_buf, fs, cache[point].lab_buf);

	    for (i = 0; i < nfiles; i++) {
		if (out_type[i] == CELL_TYPE) {
		    if (Rast_is_c_null_value(&cache[point].value[i])) {
			fprintf(stdout, "%c%s", fs, null_str);
			if (flg.label->answer)
			    fprintf(stdout, "%c", fs);
			if (flg.color->answer)
			    fprintf(stdout, "%c", fs);
			continue;
		    }
		    fprintf(stdout, "%c%ld", fs, (long)cache[point].value[i]);
		}
		else {		/* FCELL or DCELL */

		    if (Rast_is_d_null_value(&cache[point].dvalue[i])) {
			fprintf(stdout, "%c%s", fs, null_str);
			if (flg.label->answer)
			    fprintf(stdout, "%c", fs);
			if (flg.color->answer)
			    fprintf(stdout, "%c", fs);
			continue;
		    }
		    if (out_type[i] == FCELL_TYPE)
			sprintf(tmp_buf, "%.7g", cache[point].dvalue[i]);
		    else /* DCELL */
			sprintf(tmp_buf, "%.15g", cache[point].dvalue[i]);
		    G_trim_decimal(tmp_buf); /* not needed with %g? */
		    fprintf(stdout, "%c%s", fs, tmp_buf);
		}
		if (flg.label->answer)
		    fprintf(stdout, "%c%s", fs,
			    Rast_get_c_cat(&(cache[point].value[i]), &cats[i]));
		if (flg.color->answer)
		    fprintf(stdout, "%c%s", fs, cache[point].clr_buf[i]);
	    }
	    fprintf(stdout, "\n");
	}

	if (cache_report & !tty)
	    fprintf(stderr, "Cache  Hit: %6d  Miss: %6d\n",
		    cache_hit, cache_miss);

	cache_hit_tot += cache_hit;
	cache_miss_tot += cache_miss;
	cache_hit = cache_miss = 0;
    }

    if (!opt.coords->answers && !opt.points->answers && tty)
	fprintf(stderr, "\n");
    if (cache_report & !tty)
	fprintf(stderr, "Total:    Cache  Hit: %6d  Miss: %6d\n",
		cache_hit_tot, cache_miss_tot);

    /* close vector points map */
    if (opt.points->answer) {
        Vect_close(&Map);
    }
    Vect_destroy_line_struct(Points);
    
    exit(EXIT_SUCCESS);
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
static int oops(int line, const char *buf, const char *msg)
{
    static int first = 1;

    if (!tty) {
	if (first) {
	    G_warning("Input errors:");
	    first = 0;
	}
	G_warning("line %d: %s", line, buf);
    }
    G_warning("%s", msg);

    return 0;
}


/* *************************************************************** */
/* for qsort,  order list by row ********************************* */
/* *************************************************************** */


static int by_row(const void *ii, const void *jj)
{
    const struct order *i = ii, *j = jj;

    return i->row - j->row;
}


/* *************************************************************** */
/* for qsort,  order list by point ******************************* */
/* *************************************************************** */

/* for qsort,  order list by point */
static int by_point(const void *ii, const void *jj)
{
    const struct order *i = ii, *j = jj;

    return i->point - j->point;
}
