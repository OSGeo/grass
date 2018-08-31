/* Author: Adam Laza, GSoC 2016
 * based on d.vect.thematic/area.c
 *
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/symbol.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "plot.h"
#include "local_proto.h"

int display_lines(struct Map_info *Map, struct cat_list *Clist,
                  int chcat, const char *symbol_name, double size,
                  int default_width, dbCatValArray * cvarr, double *breaks,
                  int nbreaks, const struct color_rgb *colors, const struct
                  color_rgb *bcolor)
{
    int ltype, line, nlines;
    struct line_pnts *Points;
    struct line_cats *Cats;

    int n_points, n_lines, n_centroids, n_boundaries, n_faces;
    RGBA_Color *primary_color, *secondary_color;
    SYMBOL *Symb;

    Symb = NULL;

    double breakval = 0.0;
    int cat;
    dbCatVal *cv = NULL;
    int i;

    primary_color = G_malloc(sizeof(RGBA_Color));
    primary_color->a = RGBA_COLOR_OPAQUE;
    secondary_color = G_malloc(sizeof(RGBA_Color));
    secondary_color->a = RGBA_COLOR_OPAQUE;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* dynamic symbols for points */
    Symb = S_read(symbol_name);
    if (!Symb)
        G_warning(_("Unable to read symbol <%s>, unable to display points"),
                  symbol_name);
    else
        S_stroke(Symb, size, 0.0, 0);

    Vect_rewind(Map);

    nlines = -1;
    line = 0;
    n_points = n_lines = 0;
    n_centroids = n_boundaries = 0;
    n_faces = 0;
    while (TRUE) {
        line++;

        if (nlines > -1) {
            if (line > nlines)
                break;
            ltype = Vect_read_line(Map, Points, Cats, line);
        }
        else {
            ltype = Vect_read_next_line(Map, Points, Cats);

            if (ltype == -1) {
                G_fatal_error(_("Unable to read vector map"));
            }
            else if ((ltype == -2)) {   /* EOF */
                break;
            }
        }

        cat = *Cats->cat;
        if (cat >= 0) {
            G_debug(3, "display line %d, cat %d", line, cat);
            /* Get value of data for this area */
            if (db_CatValArray_get_value(cvarr, cat, &cv) != DB_OK) {
                G_debug(3, "No value found for cat %i", cat);
            }
            else {
                db_CatValArray_get_value(cvarr, cat, &cv);
                breakval = (cvarr->ctype == 2 ? cv->val.i : cv->val.d);
            }
        }

        /* find out into which class breakval falls */
        i = 0;
        while (breakval > breaks[i] && i < nbreaks)
            i++;
        primary_color->r = colors[i].r;
        primary_color->g = colors[i].g;
        primary_color->b = colors[i].b;

        if (bcolor !=NULL) {
            secondary_color->r = bcolor->r;
            secondary_color->g = bcolor->g;
            secondary_color->b = bcolor->b;
        }
        else
            secondary_color->a = 0;

        draw_line(ltype, line, Points, Cats, chcat, size, default_width,
                  Clist, Symb, primary_color, &n_points, &n_lines,
                  &n_centroids, &n_boundaries, &n_faces, secondary_color);
    }

    if (n_points > 0)
        G_verbose_message(n_
                          ("%d point plotted", "%d points plotted", n_points),
                          n_points);
    if (n_lines > 0)
        G_verbose_message(n_("%d line plotted", "%d lines plotted", n_lines),
                          n_lines);
    if (n_centroids > 0)
        G_verbose_message(n_
                          ("%d centroid plotted", "%d centroids plotted",
                           n_centroids), n_centroids);
    if (n_boundaries > 0)
        G_verbose_message(n_
                          ("%d boundary plotted", "%d boundaries plotted",
                           n_boundaries), n_boundaries);
    if (n_faces > 0)
        G_verbose_message(n_("%d face plotted", "%d faces plotted", n_faces),
                          n_faces);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    G_free(primary_color);

    return 0;
}

int draw_line(int ltype, int line,
              const struct line_pnts *Points, const struct line_cats *Cats,
              int chcat, double size, int default_width,
              const struct cat_list *Clist, SYMBOL * Symb,
              RGBA_Color * primary_color,
              int *n_points, int *n_lines, int *n_centroids,
              int *n_boundaries, int *n_faces, RGBA_Color *secondary_color)
{
    double var_size, rotation;
    int i;
    double x0, y0;
    double *x, *y;
    int found, cat;

    rotation = 0.0;
    var_size = size;
    cat = -1;

    if (!ltype)
        return 0;

    if (Points->n_points == 0)
        return 0;

    found = FALSE;
    if (chcat) {
        for (i = 0; i < Cats->n_cats; i++) {
            if (Cats->field[i] == Clist->field &&
                Vect_cat_in_cat_list(Cats->cat[i], Clist)) {
                found = TRUE;
                break;
            }
        }
        if (!found)
            return 0;
    }
    else if (Clist->field > 0) {
        for (i = 0; i < Cats->n_cats; i++) {
            if (Cats->field[i] == Clist->field) {
                found = TRUE;
                break;
            }
        }
        /* lines with no category will be displayed */
        if (Cats->n_cats > 0 && !found)
            return 0;
    }

    G_debug(3, "\tdisplay feature %d, cat %d", line, cat);


    /* enough of the prep work, lets start plotting stuff */
    x = Points->x;
    y = Points->y;

    if ((ltype & GV_POINTS)) {
        x0 = x[0];
        y0 = y[0];

        /* skip if the point is outside of the display window */
        /* xy < 0 tests make it go ever-so-slightly faster */
        if (x0 > D_get_u_east() || x0 < D_get_u_west() ||
            y0 < D_get_u_south() || y0 > D_get_u_north())
            return 0;

        D_line_width(default_width);
        D_symbol2(Symb, x0, y0, primary_color, secondary_color);
    }
    else {
        /* Plot the lines */
        D_line_width(default_width);
        D_RGB_color(primary_color->r, primary_color->g, primary_color->b);
        if (Points->n_points == 1)      /* line with one coor */
            D_polydots_abs(x, y, Points->n_points);
        else                    /* use different user defined render methods */
            D_polyline_abs(x, y, Points->n_points);
    }

    switch (ltype) {
    case GV_POINT:
        (*n_points)++;
        break;
    case GV_LINE:
        (*n_lines)++;
        break;
    case GV_CENTROID:
        (*n_centroids)++;
        break;
    case GV_BOUNDARY:
        (*n_boundaries)++;
        break;
    case GV_FACE:
        (*n_faces)++;
        break;
    default:
        break;
    }

    return 1;
}
