/* obervation_points.c (simlib), 14.mar.2011, SG */

#include <grass/glocale.h>
#include <grass/vector.h>

#include <grass/simlib.h>

static void init_points(ObservationPoints *p, int);
static void realloc_points(ObservationPoints *p, int);
static void insert_next_point(ObservationPoints *p, double x, double y,
                              int cat);

/* ************************************************************************* */

void create_observation_points(ObservationPoints *points)
{
    int if_log = 0;
    struct Map_info Map;
    struct line_pnts *pts;
    struct line_cats *cts;
    double x, y;
    int type, cat, i;
    struct Cell_head cellhd;

    if (points->observation != NULL)
        if_log += 1;

    if (points->logfile != NULL)
        if_log += 1;

    /* Nothing to do */
    if (if_log == 0)
        return;

    /* why both are required ? */
    if (if_log == 1)
        G_fatal_error("Observation vector map and logfile must be provided");

    Vect_set_open_level(1);

    if (Vect_open_old(&Map, points->observation, "") < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), points->observation);

    Vect_rewind(&Map);

    pts = Vect_new_line_struct();
    cts = Vect_new_cats_struct();

    /* Initialize point structure */
    init_points(points, 128);

    /* get the current computational region set in the runtime */
    G_get_set_window(&cellhd);

    /* Read all vector points */
    while (1) {
        type = Vect_read_next_line(&Map, pts, cts);

        if (type == -2) {
            break;
        }

        if (type == -1) {
            Vect_close(&Map);
            G_fatal_error(_("Unable to read points from map %s"),
                          points->observation);
        }

        if (type == GV_POINT) {
            x = pts->x[0];
            y = pts->y[0];
            cat = cts->cat[0];

            /* Check region bounds before inserting point */
            if (x <= cellhd.east && x >= cellhd.west && y <= cellhd.north &&
                y >= cellhd.south) {
                insert_next_point(points, x, y, cat);
            }
        }
    }

    Vect_close(&Map);

    /* Open the logfile */
    points->output = fopen(points->logfile, "w");

    if (points->output == NULL)
        G_fatal_error(_("Unable to open observation logfile %s for writing"),
                      points->logfile);

    points->is_open = 1;

    fprintf(points->output, "STEP   ");

    /* Write the vector cats as header information in the logfile */
    for (i = 0; i < points->npoints; i++) {
        fprintf(points->output, "CAT%.4d ", points->cats[i]);
    }
    fprintf(points->output, "\n");

    return;
}

/* ************************************************************************* */

void init_points(ObservationPoints *p, int size)
{
    p->x = (double *)G_calloc(size, sizeof(double));
    p->y = (double *)G_calloc(size, sizeof(double));
    p->cats = (int *)G_calloc(size, sizeof(int));
    p->npoints = 0;
    p->npoints_alloc = size;
    p->output = NULL;
    p->is_open = 0;
}

/* ************************************************************************* */

void realloc_points(ObservationPoints *p, int add_size)
{
    p->x = (double *)G_realloc(p->x,
                               (p->npoints_alloc + add_size) * sizeof(double));
    p->y = (double *)G_realloc(p->y,
                               (p->npoints_alloc + add_size) * sizeof(double));
    p->cats =
        (int *)G_realloc(p->cats, (p->npoints_alloc + add_size) * sizeof(int));
    p->npoints_alloc += add_size;
}

/* ************************************************************************* */

void insert_next_point(ObservationPoints *p, double x, double y, int cat)
{
    if (p->npoints == p->npoints_alloc)
        realloc_points(p, 128);

    G_debug(3, "Insert point %g %g %i id %i\n", x, y, cat, p->npoints);

    p->x[p->npoints] = x;
    p->y[p->npoints] = y;
    p->cats[p->npoints] = cat;
    p->npoints++;
}
