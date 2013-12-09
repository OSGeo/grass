/*!
   \file lib/vector/vedit/render.c

   \brief Vedit library - render vector features (used by wxGUI digitizer)

   (C) 2010-2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <math.h>

#include <grass/vedit.h>

static struct _region
{
    double center_easting;
    double center_northing;
    double map_west;
    double map_north;
    int map_width;
    int map_height;
    double map_res;
} region;

static struct _state
{
    int nitems_alloc;

    int type;
    struct line_pnts *Points;
} state;

static struct robject *draw_line(struct Map_info *, int, int);
static struct robject *draw_line_vertices();
static void draw_line_nodes(struct Map_info *, int, int,
			    struct robject_list *);
static int draw_line_dir(struct robject_list *, int);
static void list_append(struct robject_list *, struct robject *);
static struct robject *robj_alloc(int, int);
static void robj_points(struct robject *, const struct line_pnts *);
static double dist_in_px(double);
static void en_to_xy(double, double, int *, int *);
static void draw_arrow(int, int, int, int, double, int, int,
		       struct robject_list *);
static void draw_area(struct Map_info *, int, struct robject_list *);

/*!
  \brief Render vector features into list
  
  \param Map pointer to Map_info structure
  \param box bounding box of region to be rendered
  \param draw_flag types of objects to be rendered (see vedit.h)
  \param center_easing, center_northing, map_width, map_height, map_res values used for conversion en->xy
  
  \return pointer to robject_list structure
*/
struct robject_list *Vedit_render_map(struct Map_info *Map,
				      struct bound_box *box, int draw_flag,
				      double center_easting,
				      double center_northing, int map_width,
				      int map_height, double map_res)
{
    int i, nfeat, fid;
    struct boxlist *list;
    struct robject_list *list_obj;
    struct robject *robj;

    /* define region */
    region.center_easting = center_easting;
    region.center_northing = center_northing;
    region.map_width = map_width;
    region.map_height = map_height;
    region.map_res = map_res;
    region.map_west = center_easting - (map_width / 2.) * map_res;
    region.map_north = center_northing + (map_height / 2.) * map_res;

    list = Vect_new_boxlist(0);
    list_obj = NULL;
    state.nitems_alloc = 1000;

    list_obj = (struct robject_list *)G_malloc(sizeof(struct robject_list));
    list_obj->nitems = 0;
    list_obj->item =
	(struct robject **)G_malloc(state.nitems_alloc *
				    sizeof(struct robject *));

    /* area */
    if (draw_flag & DRAW_AREA) {
	nfeat = Vect_select_areas_by_box(Map, box, list);
	for (i = 0; i < nfeat; i++) {
	    fid = list->id[i];
	    draw_area(Map, fid, list_obj);
	}
    }

    /* draw lines inside of current display region */
    nfeat = Vect_select_lines_by_box(Map, box, GV_POINTS | GV_LINES,	/* fixme */
				     list);
    G_debug(1, "Vedit_render_map(): region: w=%f, e=%f, s=%f, n=%f nlines=%d",
	    box->W, box->E, box->S, box->N, nfeat);

    /* features */
    for (i = 0; i < list->n_values; i++) {
	fid = list->id[i];
	robj = draw_line(Map, fid, draw_flag);
	if (!robj)
	    continue;
	list_append(list_obj, robj);

	if (state.type & GV_LINES) {
	    /* vertices */
	    if (draw_flag & DRAW_VERTEX) {
		robj = draw_line_vertices();
		robj->fid = fid;
		if (robj)
		    list_append(list_obj, robj);
	    }
	    /* nodes */
	    if (draw_flag & (DRAW_NODEONE | DRAW_NODETWO)) {
		draw_line_nodes(Map, fid, draw_flag, list_obj);
	    }
	    /* direction */
	    if (draw_flag & DRAW_DIRECTION) {
		draw_line_dir(list_obj, fid);
	    }
	}
    }

    list_obj->item =
	(struct robject **)G_realloc(list_obj->item,
				     list_obj->nitems *
				     sizeof(struct robject *));

    G_debug(1, "Vedit_render_map(): -> nitems = %d",
	    list_obj->nitems);
    
    Vect_destroy_boxlist(list);

    return list_obj;
}

/*!
   \brief Draw one feature
 */
struct robject *draw_line(struct Map_info *Map, int line, int draw_flag)
{
    int draw;
    struct robject *obj;

    if (!state.Points)
	state.Points = Vect_new_line_struct();

    if (!Vect_line_alive(Map, line))
	return NULL;

    state.type = Vect_read_line(Map, state.Points, NULL, line);

    obj = (struct robject *)G_malloc(sizeof(struct robject));
    obj->fid = line;
    draw = FALSE;
    if (state.type & GV_LINES) {
	if (state.type == GV_LINE) {
	    obj->type = TYPE_LINE;
	    draw = draw_flag & DRAW_LINE;
	}
	else if (state.type == GV_BOUNDARY) {
	    int left, right;

	    Vect_get_line_areas(Map, line, &left, &right);
	    if (left == 0 && right == 0) {
		obj->type = TYPE_BOUNDARYNO;
		draw = draw_flag & DRAW_BOUNDARYNO;
	    }
	    else if (left > 0 && right > 0) {
		obj->type = TYPE_BOUNDARYTWO;
		draw = draw_flag & DRAW_BOUNDARYTWO;
	    }
	    else {
		obj->type = TYPE_BOUNDARYONE;
		draw = draw_flag & DRAW_BOUNDARYONE;
	    }
	}
    }
    else if (state.type & GV_POINTS) {
	if (state.type == GV_POINT) {
	    obj->type = TYPE_POINT;
	    draw = draw_flag & DRAW_POINT;
	}
	else if (state.type == GV_CENTROID) {
	    int cret = Vect_get_centroid_area(Map, line);

	    if (cret > 0) {	/* -> area */
		obj->type = TYPE_CENTROIDIN;
		draw = draw_flag & DRAW_CENTROIDIN;
	    }
	    else if (cret == 0) {
		obj->type = TYPE_CENTROIDOUT;
		draw = draw_flag & DRAW_CENTROIDOUT;
	    }
	    else {
		obj->type = TYPE_CENTROIDDUP;
		draw = draw_flag & DRAW_CENTROIDDUP;
	    }
	}
    }
    G_debug(3, "  draw_line(): type=%d rtype=%d npoints=%d draw=%d",
	    state.type, obj->type, state.Points->n_points, draw);

    if (!draw)
	return NULL;

    obj->npoints = state.Points->n_points;
    obj->point =
	(struct rpoint *)G_malloc(obj->npoints * sizeof(struct rpoint));
    robj_points(obj, state.Points);

    return obj;
}

/*!
   \brief Convert geographic coordinates to the screen
 */
void en_to_xy(double east, double north, int *x, int *y)
{
    double n, w;

    w = region.center_easting - (region.map_width / 2) * region.map_res;
    n = region.center_northing + (region.map_height / 2) * region.map_res;

    if (x)
	*x = (east - w) / region.map_res;
    if (y)
	*y = (n - north) / region.map_res;

    return;
}

/*!
   \brief Draw line nodes
 */
void draw_line_nodes(struct Map_info *Map, int line, int draw_flag,
		     struct robject_list *list)
{
    unsigned int i;
    int type, nodes[2];
    int x, y;
    double east, north;
    struct robject *robj;

    if (Vect_get_line_type(Map, line) & GV_POINTS)
	return;

    Vect_get_line_nodes(Map, line, &(nodes[0]), &(nodes[1]));
    
    for (i = 0; i < sizeof(nodes) / sizeof(int); i++) {
	type = 0;
	if (Vect_get_node_n_lines(Map, nodes[i]) == 1) {
	    if (draw_flag & DRAW_NODEONE) {
		type = TYPE_NODEONE;
	    }
	}
	else {
	    if (draw_flag & DRAW_NODETWO) {
		type = TYPE_NODETWO;
	    }
	}

	if (type == 0)
	    continue;

	Vect_get_node_coor(Map, nodes[i], &east, &north, NULL);

	robj = robj_alloc(type, 1);
	en_to_xy(east, north, &x, &y);
	robj->fid = line;
	robj->point->x = x;
	robj->point->y = y;

	list_append(list, robj);
    }
}

/*!
   \brief Append object to the list
 */
void list_append(struct robject_list *list, struct robject *obj)
{
    if (list->nitems >= state.nitems_alloc) {
	state.nitems_alloc += 1000;
	list->item =
	    (struct robject **)G_realloc(list->item,
					 state.nitems_alloc *
					 sizeof(struct robject *));
    }
    list->item[list->nitems++] = obj;
}

/*!
   \brief Allocate robject 
 */
struct robject *robj_alloc(int type, int npoints)
{
    struct robject *robj;

    robj = (struct robject *)G_malloc(sizeof(struct robject));
    robj->type = type;
    robj->npoints = npoints;
    robj->point = (struct rpoint *)G_malloc(npoints * sizeof(struct rpoint));

    return robj;
}

/*!
   \brief Draw line vertices
 */
struct robject *draw_line_vertices()
{
    int i;
    int x, y;
    struct robject *robj;

    robj = robj_alloc(TYPE_VERTEX, state.Points->n_points - 2);	/* ignore nodes */

    for (i = 1; i < state.Points->n_points - 1; i++) {
	en_to_xy(state.Points->x[i], state.Points->y[i], &x, &y);
	robj->point[i - 1].x = x;
	robj->point[i - 1].y = y;
    }

    return robj;
}

/*!
   \brief Draw line dirs
 */
int draw_line_dir(struct robject_list *list, int line)
{
    int narrows;
    int size;			/* arrow length in pixels */
    int limit;			/* segment length limit for drawing symbol (in pixels) */
    double dist, angle, pos;
    double e, n;
    int x0, y0, x1, y1;

    narrows = 0;
    size = 5;
    limit = 5;			/* 5px for line segment */

    dist = Vect_line_length(state.Points);
    G_debug(5, "  draw_line_dir() line=%d", line);
						  
    if (dist_in_px(dist) >= limit) {
	while (1) {
	    pos = (narrows + 1) * 8 * limit * region.map_res;

	    if (Vect_point_on_line(state.Points, pos,
				   &e, &n, NULL, NULL, NULL) < 1) {
		break;
	    }

	    en_to_xy(e, n, &x0, &y0);

	    if (Vect_point_on_line
		(state.Points, pos - 3 * size * region.map_res, &e, &n, NULL,
		 &angle, NULL) < 1) {
		break;
	    }

	    en_to_xy(e, n, &x1, &y1);

	    draw_arrow(x0, y0, x1, y1, angle, size, line, list);

	    if (narrows > 1e2)	/* low resolution, break */
		break;

	    narrows++;
	}

	/* draw at least one arrow in the middle of line */
	if (narrows < 1) {
	    dist /= 2.;
	    if (Vect_point_on_line(state.Points, dist,
				   &e, &n, NULL, NULL, NULL) > 0) {

		en_to_xy(e, n, &x0, &y0);

		if (Vect_point_on_line
		    (state.Points, dist - 3 * size * region.map_res, &e, &n,
		     NULL, &angle, NULL) > 0) {

		    en_to_xy(e, n, &x1, &y1);

		    draw_arrow(x0, y0, x1, y1, angle, size, line, list);
		}
	    }
	}
    }

    return narrows;
}

/*!
   \brief Calculate distance in pixels (on screen)
 */
double dist_in_px(double dist)
{
    int x, y;

    en_to_xy(region.map_west + dist, region.map_north, &x, &y);

    return sqrt(x * x);
}

/*!
   \brief Draw arrow
 */
void draw_arrow(int x0, int y0, int x1, int y1, double angle, int size, int line,
		struct robject_list *list)
{
    double angle_symb;
    struct robject *robj;

    robj = robj_alloc(TYPE_DIRECTION, 3);
    robj->fid = line;
    
    angle_symb = angle - M_PI / 2.;
    robj->point[0].x = (int)x1 + size * cos(angle_symb);
    robj->point[0].y = (int)y1 - size * sin(angle_symb);

    robj->point[1].x = x0;
    robj->point[1].y = y0;

    angle_symb = M_PI / 2. + angle;
    robj->point[2].x = (int)x1 + size * cos(angle_symb);
    robj->point[2].y = (int)y1 - size * sin(angle_symb);

    list_append(list, robj);
}

/*!
   \brief Draw area
 */
void draw_area(struct Map_info *Map, int area, struct robject_list *list)
{
    int i, centroid, isle;
    int num_isles;
    struct line_pnts *ipoints;

    struct robject *robj;

    if (!state.Points)
	state.Points = Vect_new_line_struct();

    if (!Vect_area_alive(Map, area))
	return;

    /* check for other centroids -- only area with one centroid is valid */
    centroid = Vect_get_area_centroid(Map, area);
    if (centroid <= 0)
	return;

    ipoints = Vect_new_line_struct();
    /* get area's boundary */
    Vect_get_area_points(Map, area, state.Points);
    robj = robj_alloc(TYPE_AREA, state.Points->n_points);
    robj->fid = area;
    robj_points(robj, state.Points);
    list_append(list, robj);

    /* check for isles */
    num_isles = Vect_get_area_num_isles(Map, area);
    for (i = 0; i < num_isles; i++) {
	isle = Vect_get_area_isle(Map, area, i);
	if (!Vect_isle_alive(Map, isle))
	    continue;

	Vect_get_isle_points(Map, isle, ipoints);
	robj = robj_alloc(TYPE_ISLE, ipoints->n_points);
	robj->fid = -1;
	robj_points(robj, ipoints);
	list_append(list, robj);
    }

    Vect_destroy_line_struct(ipoints);
}

/*!
   \brief convert EN -> XY
 */
void robj_points(struct robject *robj, const struct line_pnts *points)
{
    int i;
    int x, y;

    for (i = 0; i < points->n_points; i++) {
	en_to_xy(points->x[i], points->y[i], &x, &y);
	robj->point[i].x = x;
	robj->point[i].y = y;
    }
}
