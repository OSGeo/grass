/****************************************************************************
 *
 * MODULE:       r.carve
 *
 * AUTHOR(S):    Original author Bill Brown, UIUC GIS Laboratory
 *               Brad Douglas <rez touchofmadness com>
 *               Tomas Zigo <tomas zigo slovanet sk> (adding the option
 *               to read width, depth values from vector map table columns)
 *
 * PURPOSE:      Takes vector stream data, converts it to 3D raster and
 *               subtracts a specified depth
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "enforce.h"


#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

/* function prototypes */
static void clear_bitmap(struct BM *bm);
void process_line(struct Map_info *Map, struct Map_info *outMap,
                  const struct parms *parm, void *rbuf, const int *line);
static void traverse_line_flat(Point2 * pgpts, const int pt, const int npts);
static void traverse_line_noflat(Point2 * pgpts, const double depth,
                                 const int pt, const int npts);
static void set_min_point(void *buf, const int col, const int row,
                          const double elev, const double depth,
                          const RASTER_MAP_TYPE rtype);
static double lowest_cell_near_point(void *data, const RASTER_MAP_TYPE rtype,
                                     const double px, const double py,
                                     const double rad);
static void process_line_segment(const int npts, void *rbuf, Point2 * pgxypts,
                                 Point2 * pgpts, struct BM *bm,
                                 struct Map_info *outMap,
                                 const struct parms *parm);
double get_value(unsigned short int *ctype, dbColumn *col);
void set_value(dbColumn *col, unsigned short int *ctype, char *answer,
               double *parm, double *def_value, dbTable *table,
               struct Cell_head *wind, value_type type);
struct sql_statement create_select_sql_statement(struct Map_info *Map,
                                                 struct field_info *Fi,
                                                 struct boxlist *box_list,
                                                 char *columns[2],
                                                 unsigned short int *field,
                                                 char *keycol);


void enforce_downstream(int infd, int outfd,
                       struct Map_info *Map, struct Map_info *outMap,
                       struct parms *parm, struct field_info *Fi,
                       int *width_col_pos, int *depth_col_pos,
                       char *columns[2], dbDriver *driver)
/*
 * Function: enforce_downstream
 * -------------------------
 * Enforce downstream
 *
 * infd: input raster elevation map
 * outfd: output raster elevation map with carved streams
 * Map: input streams vector map
 * outMap: output vector map for adjusted stream points
 * parm: module parameters
 * Fi: layer (old: field) information
 * width_col_pos: width column position in the columns array
 * depth_col_pos: depth column position in the columns array
 * columns: array of width and depth column names
 * driver: db driver
 *
 */
{
    struct Cell_head wind;
    void *rbuf = NULL;
    struct bound_box box;
    struct boxlist *box_list;
    int more;
    unsigned int c;
    unsigned short int field, width_col_type, depth_col_type;
    /* Width used here is actually distance to center of stream */
    unsigned short int const distance = 2;
    double *def_depth = NULL, *def_width = NULL;
    dbCursor cursor;
    dbTable *table = NULL;
    dbColumn *width_col = NULL, *depth_col = NULL, *cat_col = NULL;
    struct field_info *finfo = NULL;
    struct sql_statement sql;

    G_get_window(&wind);

    Vect_set_constraint_region(Map, wind.north, wind.south, wind.east,
                               wind.west, wind.top, wind.bottom);

    /* allocate and clear memory for entire raster */
    rbuf = G_calloc(Rast_window_rows() * Rast_window_cols(),
                    Rast_cell_size(parm->raster_type));

    /* first read whole elevation file into buf */
    read_raster(rbuf, infd, parm->raster_type);

    G_message(_("Processing lines... "));

    box_list = Vect_new_boxlist(0);

    box.N = wind.north;
    box.E = wind.east;
    box.S = wind.south;
    box.W = wind.west;
    box.B = wind.bottom;
    box.T = wind.top;

    Vect_select_lines_by_box(Map, &box, GV_LINE, box_list);
    field = Vect_get_field_number(Map, parm->field->answer);

    /* Get key col */
    finfo = Vect_get_field(Map, field);

    if (parm->width_col->answer || parm->depth_col->answer)
    {
        sql = create_select_sql_statement(Map, Fi, box_list, columns,
                                          &field, finfo->key);

        if (db_open_select_cursor(driver, sql.sql,
                                  &cursor, DB_SEQUENTIAL) != DB_OK)
            G_fatal_error(_("Unable to open select cursor"));

        def_width = G_malloc(sizeof(double));
        def_depth = G_malloc(sizeof(double));
        memcpy(def_width, &parm->swidth, sizeof(double));
        memcpy(def_depth, &parm->sdepth, sizeof(double));

        table = db_get_cursor_table(&cursor);
        G_debug(1, "Default width: %.2f, depth: %.2f",
                *def_width / distance, *def_depth);

        cat_col = db_get_table_column_by_name(table,
                                              finfo->key);
        if (parm->width_col->answer)
        {
            width_col = db_get_table_column_by_name(table,
                                                    columns[*width_col_pos]);
            width_col_type = db_get_column_sqltype(width_col);
        }
        if (parm->depth_col->answer)
        {
            depth_col = db_get_table_column_by_name(table,
                                                    columns[*depth_col_pos]);
            depth_col_type = db_get_column_sqltype(depth_col);
        }

        while (1)
        {
            if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
                G_fatal_error(_("Unable to fetch data from table <%s>"),
                              Fi->table);

            if (!more)
                break;

            int cat = db_get_value_int(db_get_column_value(cat_col));

            /* Set width */
            set_value(width_col, &width_col_type, parm->width_col->answer,
                      &parm->swidth, def_width, table, &wind, WIDTH);
            parm->swidth /= distance;
            /* Set depth */
            set_value(depth_col, &depth_col_type, parm->depth_col->answer,
                      &parm->sdepth, def_depth, table, &wind, DEPTH);

            for (c = 0; c < sql.ncats; c++)
            {
                /* Line cat has an entry in the db table */
                if (cat == sql.id_cat_map[c].cat)
                {
                    G_debug(3, "Process line with id: %d, cat: %d, "
                            "width: %.2f, depth: %.2f",
                            sql.id_cat_map[c].id, cat, parm->swidth,
                            parm->sdepth);

                    process_line(Map, outMap, parm, rbuf,
                                 &sql.id_cat_map[c].id);
                }
            }
        }
        db_free_column(cat_col);
        if (parm->width_col->answer)
        {
            db_free_column(width_col);
        }
        if (parm->depth_col->answer)
        {
            db_free_column(depth_col);
        }
        if (db_close_cursor(&cursor) != DB_OK)
            G_fatal_error(_("Unable to close select cursor"));
        db_close_database_shutdown_driver(driver);
        db_free_string(sql.sql);
        G_free(sql.id_cat_map);
        G_free(def_width);
        G_free(def_depth);
    }

    /* write output raster map */
    write_raster(rbuf, outfd, parm->raster_type);

    G_free(rbuf);
}

void process_line(struct Map_info *Map, struct Map_info *outMap,
                  const struct parms *parm, void *rbuf, const int *line)
/*
 * Function: process_line
 * -------------------------
 * Process line
 *
 * Map: input streams vector map
 * outfd: output raster elevation map with carved streams
 * outMap: output vector map for adjusted stream points
 * parm: module parameters
 * rbuf: raster elevation map buffer
 * line: streams vector map line id
 *
 */
{
    int i, do_warn = 0, first_in = -1, in_out = 0, npts = 0;
    double totdist = 0.;
    struct Cell_head wind;
    static struct line_pnts *points = NULL;
    static struct line_cats *cats = NULL;
    static struct BM *bm = NULL;
    Point2 *pgpts = NULL, *pgxypts = NULL;
    PointGrp pg;
    PointGrp pgxy; /* copy any points in region to this one */

    G_get_window(&wind);

    if (!points)
      points = Vect_new_line_struct();
    if (!cats)
      cats = Vect_new_cats_struct();

    if (!(Vect_read_line(Map, points, cats, *line) & GV_LINE))
      return;

    if (!bm)
    bm = BM_create(Rast_window_cols(), Rast_window_rows());
    clear_bitmap(bm);

    pg_init(&pg);
    pg_init(&pgxy);

    G_percent(*line, Vect_get_num_lines(Map), 10);

    for (i = 0; i < points->n_points; i++) {
    Point2 pt, ptxy;
    double elev;
    int row = Rast_northing_to_row(points->y[i], &wind);
    int col = Rast_easting_to_col(points->x[i], &wind);

    /* rough clipping */
    if (row < 0 || row > Rast_window_rows() - 1 ||
        col < 0 || col > Rast_window_cols() - 1) {
        if (first_in != -1)
        in_out = 1;

        G_debug(1, "outside region - row:%d col:%d", row, col);

        continue;
    }

    if (first_in < 0)
        first_in = i;
    else if (in_out)
        do_warn = 1;

    elev = lowest_cell_near_point(rbuf, parm->raster_type, points->x[i],
                      points->y[i], parm->swidth);

    ptxy[0] = points->x[i];
    ptxy[1] = points->y[i];
    pt[1] = elev;

    /* get distance from this point to previous point */
    if (i)
        totdist += G_distance(points->x[i - 1], points->y[i - 1],
                  points->x[i], points->y[i]);

    pt[0] = totdist;
    pg_addpt(&pg, pt);
    pg_addpt(&pgxy, ptxy);
    npts++;
    }

    if (do_warn) {
    G_warning(_("Vect runs out of region and re-enters - "
            "this case is not yet implemented."));
    }

    /* now check to see if points go downslope(inorder) or upslope */
    if (pg_y_from_x(&pg, 0.0) > pg_y_from_x(&pg, totdist)) {
    pgpts = pg_getpoints(&pg);
    pgxypts = pg_getpoints(&pgxy);
    }
    else {
    /* pgpts is now high to low */
    pgpts = pg_getpoints_reversed(&pg);

    for (i = 0; i < npts; i++)
        pgpts[i][0] = totdist - pgpts[i][0];

    pgxypts = pg_getpoints_reversed(&pgxy);
    }

    for (i = 0; i < (npts - 1); i++) {
    if (parm->noflat)
        /* make sure there are no flat segments in line */
        traverse_line_noflat(pgpts, parm->sdepth, i, npts);
    else
        /* ok to have flat segments in line */
        traverse_line_flat(pgpts, i, npts);
    }
    process_line_segment(npts, rbuf, pgxypts, pgpts, bm, outMap, parm);
}


static void clear_bitmap(struct BM *bm)
{
    int i, j;

    for (i = 0; i < Rast_window_rows(); i++)
    for (j = 0; j < Rast_window_cols(); j++)
        BM_set(bm, i, j, 0);
}


static void traverse_line_flat(Point2 * pgpts, const int pt, const int npts)
{
    int j, k;

    if (pgpts[pt + 1][1] <= pgpts[pt][1])
    return;

    for (j = (pt + 2); j < npts; j++)
    if (pgpts[j][1] <= pgpts[pt][1])
        break;

    if (j == npts) {
    /* if we got to the end, level it out */
    for (j = (pt + 1); j < npts; j++)
        pgpts[j][1] = pgpts[pt][1];
    }
    else {
    /* linear interp between point pt and the next < */
    for (k = (pt + 1); k < j; k++)
        pgpts[k][1] = LINTERP(pgpts[j][1], pgpts[pt][1],
                  (pgpts[j][0] - pgpts[k][0]) /
                  (pgpts[j][0] - pgpts[pt][0]));
    }
}


static void traverse_line_noflat(Point2 * pgpts, const double depth,
                 const int pt, const int npts)
{
    int j, k;

    if (pgpts[pt + 1][1] < pgpts[pt][1])
    return;

    for (j = (pt + 2); j < npts; j++)
    if (pgpts[j][1] < pgpts[pt][1])
        break;

    if (j == npts) {
    /* if we got to the end, lower end by depth OR .01 */
    --j;
    pgpts[j][1] = pgpts[pt][1] - (depth > 0 ? depth : 0.01);
    }

    /* linear interp between point pt and the next < */
    for (k = (pt + 1); k < j; k++)
    pgpts[k][1] = LINTERP(pgpts[j][1], pgpts[pt][1],
                  (pgpts[j][0] - pgpts[k][0]) /
                  (pgpts[j][0] - pgpts[pt][0]));
}


/* sets value for a cell */
static void set_min_point(void *data, const int col, const int row,
              const double elev, const double depth,
              const RASTER_MAP_TYPE rtype)
{
    switch (rtype) {
    case CELL_TYPE:
    {
        CELL *cbuf = data;

        cbuf[row * Rast_window_cols() + col] =
        MIN(cbuf[row * Rast_window_cols() + col], elev) - (int)depth;
    }
    break;
    case FCELL_TYPE:
    {
        FCELL *fbuf = data;

        fbuf[row * Rast_window_cols() + col] =
        MIN(fbuf[row * Rast_window_cols() + col], elev) - depth;
    }
    break;
    case DCELL_TYPE:
    {
        DCELL *dbuf = data;

        dbuf[row * Rast_window_cols() + col] =
        MIN(dbuf[row * Rast_window_cols() + col], elev) - depth;
    }
    break;
    }
}


/* returns the lowest value cell within radius rad of px, py */
static double lowest_cell_near_point(void *data, const RASTER_MAP_TYPE rtype,
                     const double px, const double py,
                     const double rad)
{
    int r, row, col, row1, row2, col1, col2, rowoff, coloff;
    int rastcols, rastrows;
    double min;
    struct Cell_head wind;

    G_get_window(&wind);
    rastrows = Rast_window_rows();
    rastcols = Rast_window_cols();

    Rast_set_d_null_value(&min, 1);

    /* kludge - fix for lat_lon */
    rowoff = rad / wind.ns_res;
    coloff = rad / wind.ew_res;

    row = Rast_northing_to_row(py, &wind);
    col = Rast_easting_to_col(px, &wind);

    /* get bounding box of line segment */
    row1 = MAX(0, row - rowoff);
    row2 = MIN(rastrows - 1, row + rowoff);
    col1 = MAX(0, col - coloff);
    col2 = MIN(rastcols - 1, col + coloff);

    switch (rtype) {
    case CELL_TYPE:
    {
        CELL *cbuf = data;

        if (!(Rast_is_c_null_value(&cbuf[row1 * rastcols + col1])))
        min = cbuf[row1 * rastcols + col1];
    }
    break;
    case FCELL_TYPE:
    {
        FCELL *fbuf = data;

        if (!(Rast_is_f_null_value(&fbuf[row1 * rastcols + col1])))
        min = fbuf[row1 * rastcols + col1];
    }
    break;
    case DCELL_TYPE:
    {
        DCELL *dbuf = data;

        if (!(Rast_is_d_null_value(&dbuf[row1 * rastcols + col1])))
        min = dbuf[row1 * rastcols + col1];
    }
    break;
    }

    for (r = row1; r < row2; r++) {
    double cy = Rast_row_to_northing(r + 0.5, &wind);
    int c;

    for (c = col1; c < col2; c++) {
        double cx = Rast_col_to_easting(c + 0.5, &wind);

        if (G_distance(px, py, cx, cy) <= SQR(rad)) {
        switch (rtype) {
        case CELL_TYPE:
            {
            CELL *cbuf = data;

            if (Rast_is_d_null_value(&min)) {
                if (!(Rast_is_c_null_value(&cbuf[r * rastcols + c])))
                min = cbuf[r * rastcols + c];
            }
            else {
                if (!(Rast_is_c_null_value(&cbuf[r * rastcols + c])))
                if (cbuf[r * rastcols + c] < min)
                    min = cbuf[r * rastcols + c];
            }
            }
            break;
        case FCELL_TYPE:
            {
            FCELL *fbuf = data;

            if (Rast_is_d_null_value(&min)) {
                if (!(Rast_is_f_null_value(&fbuf[r * rastcols + c])))
                min = fbuf[r * rastcols + c];
            }
            else {
                if (!(Rast_is_f_null_value(&fbuf[r * rastcols + c])))
                if (fbuf[r * rastcols + c] < min)
                    min = fbuf[r * rastcols + c];
            }
            }
            break;
        case DCELL_TYPE:
            {
            DCELL *dbuf = data;

            if (Rast_is_d_null_value(&min)) {
                if (!(Rast_is_d_null_value(&dbuf[r * rastcols + c])))
                min = dbuf[r * rastcols + c];
            }
            else {
                if (!(Rast_is_d_null_value(&dbuf[r * rastcols + c])))
                if (dbuf[r * rastcols + c] < min)
                    min = dbuf[r * rastcols + c];
            }
            }
            break;
        }
        }
    }
    }

    G_debug(3, "min:%.2lf", min);

    return min;
}


/* Now for each segment in the line, use distance from segment
 * to find beginning row from northernmost point, beginning
 * col from easternmost, ending row & col, then loop through
 * bounding box and use distance from segment to emboss
 * new elevations */
static void process_line_segment(const int npts, void *rbuf,
                 Point2 * pgxypts, Point2 * pgpts,
                 struct BM *bm, struct Map_info *outMap,
                 const struct parms *parm)
{
    int i, row1, row2, col1, col2;
    int prevrow, prevcol;
    double cellx, celly, cy;
    struct Cell_head wind;
    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();
    int rowoff, coloff;

    G_get_window(&wind);

    Vect_cat_set(cats, 1, 1);

    /* kludge - fix for lat_lon */
    rowoff = parm->swidth / wind.ns_res;
    coloff = parm->swidth / wind.ew_res;

    /* get prevrow and prevcol for iteration 0 of following loop */
    prevrow = Rast_northing_to_row(pgxypts[0][1], &wind);
    prevcol = Rast_easting_to_col(pgxypts[0][0], &wind);

    for (i = 1; i < npts; i++) {
    int c, r;

    int row = Rast_northing_to_row(pgxypts[i][1], &wind);
    int col = Rast_easting_to_col(pgxypts[i][0], &wind);

    /* get bounding box of line segment */
    row1 = MAX(0, MIN(row, prevrow) - rowoff);
    row2 = MIN(Rast_window_rows() - 1, MAX(row, prevrow) + rowoff);
    col1 = MAX(0, MIN(col, prevcol) - coloff);
    col2 = MIN(Rast_window_cols() - 1, MAX(col, prevcol) + coloff);

    for (r = row1; r <= row2; r++) {
        cy = Rast_row_to_northing(r + 0.5, &wind);

        for (c = col1; c <= col2; c++) {
        double distance;

        cellx = Rast_col_to_easting(c + 0.5, &wind);
        celly = cy;	/* gets written over in distance2... */

        /* Thought about not going past endpoints (use
         * status to check) but then pieces end up missing
         * from outside corners - if it goes past ends,
         * should probably do some interp or will get flats.
         * Here we use a bitmap and only change cells once
         * on the way down */

        distance = sqrt(dig_distance2_point_to_line(cellx, celly, 0,
                        pgxypts[i - 1][0],
                        pgxypts[i - 1][1], 0,
                        pgxypts[i][0], pgxypts[i][1],
                        0, 0, &cellx, &celly, NULL,
                        NULL, NULL));

        if (distance <= parm->swidth && !BM_get(bm, c, r)) {
            double dist, elev;

            Vect_reset_line(points);

            dist = G_distance(pgxypts[i][0], pgxypts[i][1],
                      cellx, celly);

            elev = LINTERP(pgpts[i][1], pgpts[i - 1][1],
                   (dist / (pgpts[i][0] - pgpts[i - 1][0])));

            BM_set(bm, c, r, 1);

            /* TODO - may want to use a function for the
             * cross section of stream */
            set_min_point(rbuf, c, r, elev, parm->sdepth,
                  parm->raster_type);

            /* Add point to output vector map */
            if (parm->outvect->answer) {
            Vect_append_point(points, cellx, celly,
                      elev - parm->sdepth);
            Vect_write_line(outMap, GV_POINT, points, cats);
            }
        }
        }
    }

    prevrow = row;
    prevcol = col;
    }
}

double get_value(unsigned short int *ctype, dbColumn *col)
/*
 * Function:  get_value
 * --------------------
 * Get value from column
 *
 * ctype: column type
 * col: column
 *
 * return: column value
 */
{
    if (*ctype == DB_SQL_TYPE_INTEGER)
        return (double)(db_get_value_int(db_get_column_value(col)));
    else
        return db_get_value_double(db_get_column_value(col));
}

void set_value(dbColumn *col, unsigned short int *ctype, char *answer,
               double *parm, double *def_value, dbTable *table,
               struct Cell_head *wind, value_type type)
/*
 * Function:  set_value
 * --------------------
 * Set correct column (with or depth) value
 *
 * col: db table column
 * ctype: db table column type
 * answer: column parameter answer
 * parm: module parameters
 * def_value: default column value
 * table: streams vector map table
 * wind: current region settings
 * type: column type (width or depth)
 */
{
  double value;

  if (answer)
  {
      if (!db_get_column_value(col)->isNull)
      {
          value = get_value(ctype, col);
          switch (type)
          {
              case WIDTH:
                  adjust_swidth(wind, &value);
                  break;
              case DEPTH:
                  adjust_sdepth(&value);
                  break;
          }
          *parm = value;
      }
      else
          *parm = *def_value;
  }
  else
      *parm = *def_value;
}


void adjust_swidth(struct Cell_head *win, double *value)
/*
 * Function:  adjust_swidth
 * ------------------------
 * Adjust width value
 *
 * win: current region settings
 * value: input width value
 */
{
    double width = 0.0;

    if (*value <= width)
    {
        double def_width = G_distance((win->east + win->west) / 2,
                                      (win->north + win->south) / 2,
                                      ((win->east + win->west) / 2) +
                                      win->ew_res,
                                      (win->north + win->south) / 2);
        *value = def_width;
    }
}


void adjust_sdepth(double *value)
/*
 * Function: adjust_sdepth
 * -----------------------
 * Adjust depth value
 *
 * value: input depth value
 */
{
    double def_depth = 0.0;

    if (*value < def_depth)
        *value = def_depth;
}


struct sql_statement create_select_sql_statement(struct Map_info *Map,
                                                 struct field_info *Fi,
                                                 struct boxlist *box_list,
                                                 char *columns[2],
                                                 unsigned short int *field,
                                                 char *keycol)
/*
 * Function: create_select_sql_statement
 * -------------------------------------
 * Creates sql string
 *
 * Map: input streams vector line map
 * Fi: layer (old: field) information
 * box_list: list of bounding boxes with id (line streams inside region)
 * columns: width and depth column names
 * field: layer (old: field) number)
 * keycol: key column name
 *
 * return ret: sql_statement struct
 *             sql_statement.sql: sql string
 *             sql.sql: number of cats
 *             sql.id_cat_map: line id with matching cat (
 *             vect_id_cat_map struct)
 *
 */
{
    int cat_buf_len, size, line, ncats, next_cat, prev_line = -2;
    bool no_cat = false;
    char query[DB_SQL_MAX];
    char *cat_buf = NULL, *where_buf = NULL;
    dbString sql; /* value_string; */
    struct sql_statement ret;
    struct ptr *p = G_malloc(sizeof(struct ptr));

    db_init_string(&sql);

    sprintf(query, "SELECT %s, %s, %s FROM ", keycol, columns[0],
            columns[1]);
    db_set_string(&sql, query);
    if (db_append_string(&sql, Fi->table) != DB_OK)
        G_fatal_error(_("Unable to append string"));

    cat_buf_len = 0;
    ncats = 0;

    /* Create WHERE clause ("WHERE keycol in (value, ....)") */
    for (line = 0; line < box_list->n_values; line++)
    {
        int cat = Vect_get_line_cat(Map, box_list->id[line], *field);
        /* Filter out cat = -1 */
        if (cat < 0)
        {
            no_cat = true;
            prev_line = line;

            if (line == box_list->n_values - 1 && !no_cat)
            {
                size = snprintf(NULL, 0, "%d)", cat);
                cat_buf = G_realloc(cat_buf, strlen(cat_buf) + size + 1);
                p->type = P_CHAR;
                p->p_char = cat_buf;
                check_mem_alloc(p);
                cat_buf_len += sprintf(cat_buf + cat_buf_len, "%d)", cat);
            }
            continue;
        }
        else
        {
            ncats += 1;
            if (ncats == 1)
                ret.id_cat_map = G_malloc(sizeof(struct vect_id_cat_map));
            else
                ret.id_cat_map = G_realloc(ret.id_cat_map,
                                           sizeof(struct vect_id_cat_map) *
                                           ncats);

            p->type = P_VECT_ID_CAT_MAP;
            p->p_vect_id_cat_map = ret.id_cat_map;
            check_mem_alloc(p);
            /* Saving id with corresponding cat value */
            ret.id_cat_map[ncats - 1].cat = cat;
            ret.id_cat_map[ncats - 1].id = box_list->id[line];
        }
        if (line == 0 || (prev_line == 0 && no_cat))
        {
            size = snprintf(NULL, 0, "(%d, ", cat);
            cat_buf = G_malloc(size + 1);
            p->type = P_CHAR;
            p->p_char = cat_buf;
            check_mem_alloc(p);
            cat_buf_len += sprintf(cat_buf + cat_buf_len, "(%d, ", cat);

            if (prev_line == 0)
                prev_line = -2;
        }
        else if (line > 0 && line < box_list->n_values - 1)
        {
            next_cat = Vect_get_line_cat(Map, box_list->id[line + 1], *field);
            if (next_cat < 0 && line + 1 == box_list->n_values - 1)
            {
                size = snprintf(NULL, 0, "%d)", cat);
                cat_buf = G_realloc(cat_buf, strlen(cat_buf) + size + 1);
                p->type = P_CHAR;
                p->p_char = cat_buf;
                check_mem_alloc(p);
                cat_buf_len += sprintf(cat_buf + cat_buf_len, "%d)", cat);
            }
            else
            {
                size = snprintf(NULL, 0, "%d, ", cat);
                cat_buf = G_realloc(cat_buf, strlen(cat_buf) + size + 1);
                p->type = P_CHAR;
                p->p_char = cat_buf;
                check_mem_alloc(p);
                cat_buf_len += sprintf(cat_buf + cat_buf_len, "%d, ", cat);
            }
        }
        else if (line == box_list->n_values - 1)
        {
            size = snprintf(NULL, 0, "%d)", cat);
            cat_buf = G_realloc(cat_buf, strlen(cat_buf) + size + 1);
            p->type = P_CHAR;
            p->p_char = cat_buf;
            check_mem_alloc(p);
            cat_buf_len += sprintf(cat_buf + cat_buf_len, "%d)", cat);
        }
    }

    size = snprintf(NULL, 0, " WHERE %s in %s", keycol, cat_buf);
    where_buf = G_malloc(size + 1);
    p->type = P_CHAR;
    p->p_char = where_buf;
    check_mem_alloc(p);

    sprintf(where_buf, " WHERE %s in %s", keycol, cat_buf);
    if (db_append_string(&sql, where_buf) != DB_OK)
        G_fatal_error(_("Unable to append string"));
    G_free(cat_buf);
    G_free(where_buf);

    ret.sql = G_malloc(sizeof(sql));
    p->type = P_DBSTRING;
    check_mem_alloc(p);
    p->p_dbString = ret.sql;
    ret.sql = &sql;

    ret.ncats = ncats;

    G_debug(1, "Sql statement: %s", ret.sql->string);
    G_debug(1, "Number of cats: %d", ret.ncats);

    return ret;
}
