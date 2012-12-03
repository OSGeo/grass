
/**
 * \file plus_area.c
 *
 * \brief Vector library - update topo for areas (lower level functions)
 *
 * Lower level functions for reading/writing/manipulating vectors.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author CERL (probably Dave Gerdes), Radim Blazek
 *
 * \date 2001-2006
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static int debug_level = -1;

/*!
 * \brief Build topo for area from lines
 *
 * Area is built in clockwise order.
 * Take a given line and start off to the RIGHT/LEFT and try to complete
 * an area. 
 * 
 * Possible Scenarios:
 *  - I.    path runs into first line.                              : AREA!
 *  - II.   path runs into a dead end (no other area lines at node) : no area
 *  - III.  path runs into a previous line that is not 1st line or to 1st line but not to start node : no area
 *
 * After we find an area then we call point_in_area() to see if the
 * specified point is w/in the area
 *
 * Old returns  -1:  error   0:  no area    (1:  point in area)
 *              -2: island  !!
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] first_line line id of first line
 * \param[in] side side of line to build area on (GV_LEFT | GV_RIGHT)
 * \param[in] lines pointer to array of lines
 *
 * \return  -1 on error   
 * \return   0 no area
 * \return   number of lines
 */
int
dig_build_area_with_line(struct Plus_head *plus, plus_t first_line, int side,
			 plus_t ** lines)
{
    register int i;
    int prev_line, next_line;
    static plus_t *array;
    char *p;
    static int array_size;	/* 0 on startup */
    int n_lines;
    struct P_line *Line;
    struct P_topo_b *topo;
    int node;

    if (debug_level == -1) {
	const char *dstr = G__getenv("DEBUG");

	if (dstr != NULL)
	    debug_level = atoi(dstr);
	else
	    debug_level = 0;
    }

    G_debug(3, "dig_build_area_with_line(): first_line = %d, side = %d",
	    first_line, side);

    /* First check if line is not degenerated (degenerated lines have angle -9) 
     *  Following degenerated lines are skip by dig_angle_next_line() */
    Line = plus->Line[first_line];
    if (Line->type != GV_BOUNDARY)
	return -1;

    topo = (struct P_topo_b *)Line->topo;
    node = topo->N1;		/* to check one is enough, because if degenerated N1 == N2 */
    if (dig_node_line_angle(plus, node, first_line) == -9.) {
	G_debug(3, "First line degenerated");
	return (0);
    }

    if (array_size == 0) {	/* first time */
	array_size = 1000;
	array = (plus_t *) dig__falloc(array_size, sizeof(plus_t));
	if (array == NULL)
	    return (dig_out_of_memory());
    }

    if (side == GV_LEFT) {
	first_line = -first_line;	/* start at node1, reverse direction */
    }
    array[0] = first_line;
    prev_line = -first_line;	/* start at node2 for direct and node1 for
				   reverse direction */
    /* angle of first line */
    n_lines = 1;
    while (1) {
	next_line =
	    dig_angle_next_line(plus, prev_line, GV_RIGHT, GV_BOUNDARY, NULL);
	G_debug(3, "next_line = %d", next_line);

	if (next_line == 0)
	    return (-1);	/* Not found */

	/* Check if adjacent lines do not have the same angle */
	if (!dig_node_angle_check(plus, next_line, GV_BOUNDARY)) {
	    G_debug(3,
		    "Cannot build area, a neighbour of the line %d has the same angle at the node",
		    next_line);
	    return 0;
	}

	/*  I. Area closed. This also handles the problem w/ 1 single area line */
	if (first_line == next_line) {
	    /* GOT ONE!  fill area struct  and return */
	    G_debug(3, "Got one! :");

	    /* avoid loop when not debugging */
	    if (debug_level > 2) {
		for (i = 0; i < n_lines; i++) {
		    G_debug(3, " area line (%d) = %d", i, array[i]);
		}
	    }

	    *lines = array;
	    return (n_lines);
	}

	/* II. Note this is a dead end */
	/* ( if prev_line != -first_line so it goes after the previous test) ? */
	if (prev_line == next_line) {
	    G_debug(3, "Dead_end:");
	    return (0);		/* dead end */
	}

	/* III. Unclosed ?, I would say started from free end */
	for (i = 0; i < n_lines; i++)
	    if (abs(next_line) == abs(array[i])) {
		G_debug(3, "Unclosed area:");
		return (0);	/* ran into a different area */
	    }

	/* otherwise keep going */
	if (n_lines >= array_size) {
	    p = dig__frealloc(array, array_size + 100, sizeof(plus_t),
			      array_size);
	    if (p == NULL)
		return (dig_out_of_memory());
	    array = (plus_t *) p;
	    array_size += 100;
	}
	array[n_lines++] = next_line;
	prev_line = -next_line;
    }

    return 0;
}

/*!
 * \brief Allocate space for new area and create boundary info from array.
 *
 * Then for each line in area, update line (right,left) info.
 *
 * Neither islands nor centroids area filled.
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] n_lines number of lines
 * \param[in] lines array of lines, negative for reverse direction
 * \param[in] box bounding box
 *
 * \return number of new area
 * \return -1 on error
 */
int dig_add_area(struct Plus_head *plus, int n_lines, plus_t * lines,
		 struct bound_box *box)
{
    register int i;
    register int area, line;
    struct P_area *Area;
    struct P_line *Line;
    struct P_topo_b *topo;

    G_debug(3, "dig_add_area():");
    /* First look if we have space in array of pointers to areas
     *  and reallocate if necessary */
    if (plus->n_areas >= plus->alloc_areas) {	/* array is full */
	if (dig_alloc_areas(plus, 1000) == -1)
	    return -1;
    }

    /* allocate area structure */
    area = plus->n_areas + 1;
    G_debug(3, "    new area = %d", area);
    Area = dig_alloc_area();
    if (Area == NULL)
	return -1;

    if (dig_area_alloc_line(Area, n_lines) == -1)
	return -1;

    for (i = 0; i < n_lines; i++) {
	line = lines[i];
	Area->lines[i] = line;
	Line = plus->Line[abs(line)];
	topo = (struct P_topo_b *)Line->topo;
	if (plus->uplist.do_uplist)
	    dig_line_add_updated(plus, abs(line));
	if (line < 0) {		/* revers direction -> area on left */
	    if (topo->left != 0) {
		G_warning(_("Line %d already has area/isle %d to left"), line,
			  topo->left);
		return -1;
	    }

	    G_debug(3, "  Line %d left set to %d.", line, area);
	    topo->left = area;
	}
	else {
	    if (topo->right != 0) {
		G_warning(_("Line %d already has area/isle %d to right"),
			  line, topo->right);
		return -1;
	    }

	    G_debug(3, "  Line %d right set to %d.", line, area);
	    topo->right = area;
	}
    }
    Area->n_lines = n_lines;
    Area->centroid = 0;

    plus->Area[area] = Area;

    dig_spidx_add_area(plus, area, box);

    plus->n_areas++;

    return (area);
}

/*!
 * \brief Add isle to area if does not exist yet.
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] area area id
 * \param[in] isle isle id
 *
 * \return 0
 */
int dig_area_add_isle(struct Plus_head *plus, int area, int isle)
{
    int i;
    struct P_area *Area;

    G_debug(3, "dig_area_add_isle(): area = %d isle = %d", area, isle);

    if (debug_level == -1) {
	const char *dstr = G__getenv("DEBUG");

	if (dstr != NULL)
	    debug_level = atoi(dstr);
	else
	    debug_level = 0;
    }

    Area = plus->Area[area];
    if (Area == NULL)
	G_fatal_error("Attempt to add isle to dead area");

    if (debug_level > 0) {
	for (i = 0; i < Area->n_isles; i++) {
	    if (Area->isles[i] == isle) {
		/* Already exists: bug in vector libs */
		G_warning(_("Isle already registered in area"));
		return 0;
	    }
	}
    }

    if (Area->alloc_isles <= Area->n_isles)	/* array is full */
	dig_area_alloc_isle(Area, 1);

    Area->isles[Area->n_isles] = isle;
    Area->n_isles++;
    G_debug(3, "  -> n_isles = %d", Area->n_isles);

    return 0;
}

/*!
 * \brief Delete isle from area.
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] area area id
 * \param[in] isle isle id
 *
 * \return 0
 */
int dig_area_del_isle(struct Plus_head *plus, int area, int isle)
{
    int i, mv;
    struct P_area *Area;

    G_debug(3, "dig_area_del_isle(): area = %d isle = %d", area, isle);

    Area = plus->Area[area];
    if (Area == NULL)
	G_fatal_error(_("Attempt to delete isle from dead area"));

    mv = 0;
    for (i = 0; i < Area->n_isles; i++) {
	if (mv) {
	    Area->isles[i - 1] = Area->isles[i];
	}
	else {
	    if (Area->isles[i] == isle)
		mv = 1;
	}
    }

    if (mv) {
	Area->n_isles--;
    }
    else {
	G_fatal_error(_("Attempt to delete not registered isle %d from area %d"),
		      isle, area);
    }

    return 0;
}

/*!
 * \brief Delete area from Plus_head structure
 *
 *  This function deletes area from the topo structure and resets references
 *  to this area in lines, isles (within) to 0. 
 *  Possible new area is not created by this function, so that
 *  old boundaries participating in this area are left without area information
 *  even if form new area.
 *  Not enabled now: If area is inside other area, area info for islands within 
 *                   deleted area is reset to that area outside.
 *  (currently area info of isles is set to 0)
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] area area id
 *
 * \return 0 on error
 * \return 1 on success
 */
int dig_del_area(struct Plus_head *plus, int area)
{
    int i, line;

    struct P_area *Area;
    struct P_line *Line;
    struct P_isle *Isle;
    struct P_topo_b *btopo;
    struct P_topo_c *ctopo;
    
    G_debug(3, "dig_del_area() area =  %d", area);
    Area = plus->Area[area];

    if (Area == NULL) {
	G_warning(_("Attempt to delete dead area"));
	return 0;
    }

    dig_spidx_del_area(plus, area);

    /* Set area for all lines to 0 */
    /* isle = 0; */
    for (i = 0; i < Area->n_lines; i++) {
	line = Area->lines[i];	/* >0 = clockwise -> right, <0 = counterclockwise ->left */
	Line = plus->Line[abs(line)];
	btopo = (struct P_topo_b *)Line->topo;
	if (plus->uplist.do_uplist)
	    dig_line_add_updated(plus, abs(line));
	if (line > 0) {
	    G_debug(3, "  Set line %d right side to 0", line);
	    btopo->right = 0;
	}
	else {
	    G_debug(3, "  Set line %d left side to 0", line);
	    btopo->left = 0;
	}

	/* Find the isle this area is part of (used late below) */
	/*
	   if ( line > 0 ) {
	   if ( Line->left < 0 ) isle = Line->left; 
	   } else {
	   if ( Line->right < 0 ) isle = Line->right; 
	   }
	 */
    }

    /* Unset area information of centroid */
    /* TODO: duplicate centroids have also area information ->
     *        1) do not save such info
     *        2) find all by box and reset info */
    line = Area->centroid;
    if (line > 0) {
	Line = plus->Line[line];
	if (!Line) {
	    G_warning(_("Dead centroid %d registered for area (bug in the vector library)"),
		      line);
	}
	else {
	    ctopo = (struct P_topo_c *)Line->topo;
	    ctopo->area = 0;
	    if (plus->uplist.do_uplist)
		dig_line_add_updated(plus, line);
	}
    }

    /* Find the area this area is within */
    /*
       area_out = 0;
       if ( isle > 0 ) { 
       Isle =  plus->Isle[abs(isle)];
       area_out = Isle->area;
       }
     */

    /* Reset information about area outside for isles within this area */
    G_debug(3, "  n_isles = %d", Area->n_isles);
    for (i = 0; i < Area->n_isles; i++) {
	Isle = plus->Isle[Area->isles[i]];
	if (Isle == NULL) {
	    G_fatal_error(_("Attempt to delete area %d info from dead isle %d"),
			  area, Area->isles[i]);
	}
	else {
	    /* Isle->area = area_out; */
	    Isle->area = 0;
	}
    }

    /* free structures */
    dig_free_area(Area);
    plus->Area[area] = NULL;
    return 1;
}


/*!
 * \brief Find number line of next angle to follow an line
 *
 * Assume that lines are sorted in increasing angle order and angles
 * of points and degenerated lines are set to -9 (ignored).
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] current_line current line id, negative if request for end node
 * \param[in] side side GV_RIGHT or GV_LEFT
 * \param[in] type line type (GV_LINE, GV_BOUNDARY or both)
 * \param[in] angle
 *
 * \return line number of next angle to follow an line (negative if connected by end node)
 *               (number of current line may be return if dangle - this is used in build)
 * \return 0 on error or not found
 */
int
dig_angle_next_line(struct Plus_head *plus, plus_t current_line, int side,
		    int type, float *angle)
{
    int i, next;
    int current;
    int line;
    plus_t node;
    struct P_node *Node;
    struct P_line *Line;

    if (debug_level == -1) {
	const char *dstr = G__getenv("DEBUG");

	if (dstr != NULL)
	    debug_level = atoi(dstr);
	else
	    debug_level = 0;
    }

    G_debug(3, "dig__angle_next_line: line = %d, side = %d, type = %d",
	    current_line, side, type);

    Line = plus->Line[abs(current_line)];
    
    if (!(Line->type & GV_LINES)) {
	if (angle)
	    *angle = -9.;
	return 0;
    }

    node = 0;
    if (current_line > 0) {
	if (Line->type == GV_LINE) {
	    struct P_topo_l *topo = (struct P_topo_l *)Line->topo;
	    node = topo->N1;
	}
	else if (Line->type == GV_BOUNDARY) {
	    struct P_topo_b *topo = (struct P_topo_b *)Line->topo;
	    node = topo->N1;
	}
    }
    else {
	if (Line->type == GV_LINE) {
	    struct P_topo_l *topo = (struct P_topo_l *)Line->topo;
	    node = topo->N2;
	}
	else if (Line->type == GV_BOUNDARY) {
	    struct P_topo_b *topo = (struct P_topo_b *)Line->topo;
	    node = topo->N2;
	}
    }

    G_debug(3, " node = %d", node);

    Node = plus->Node[node];
    G_debug(3, "  n_lines = %d", Node->n_lines);
    /* avoid loop when not debugging */
    if (debug_level > 2) {
	for (i = 0; i < Node->n_lines; i++) {
	    G_debug(3, "  i = %d line = %d angle = %f", i, Node->lines[i],
		    Node->angles[i]);
	}
    }

    /* first find index for that line */
    next = -1;
    for (current = 0; current < Node->n_lines; current++) {
	if (Node->lines[current] == current_line)
	    next = current;
    }
    if (next == -1) {
	if (angle)
	    *angle = -9.;
	return 0;		/* not found */
    }

    G_debug(3, "  current position = %d", next);
    while (1) {
	if (side == GV_RIGHT) {	/* go up (greater angle) */
	    if (next == Node->n_lines - 1)
		next = 0;
	    else
		next++;
	}
	else {			/* go down (smaller angle) */
	    if (next == 0)
		next = Node->n_lines - 1;
	    else
		next--;
	}
	G_debug(3, "  next = %d line = %d angle = %f", next,
		Node->lines[next], Node->angles[next]);

	if (Node->angles[next] == -9.) {	/* skip points and degenerated */
	    G_debug(3, "  point/degenerated -> skip");
	    if (Node->lines[next] == current_line)
		break;		/* Yes, that may happen if input line is degenerated and isolated and this breaks loop */
	    else
		continue;
	}

	line = abs(Node->lines[next]);
	Line = plus->Line[line];
        
	if (Line->type & type) {	/* line found */
	    G_debug(3, "  this one");
	    if (angle)
		*angle = Node->angles[next];
	    return (Node->lines[next]);
	}

	/* input line reached, this must be last, because current_line may be correct return value (dangle) */
	if (Node->lines[next] == current_line)
	    break;
    }
    G_debug(3, "  Line NOT found at node %d", (int)node);
    if (angle)
	*angle = -9.;

    return 0;
}

/*!
 * \brief Checks if angles of adjacent lines differ.
 *
 * Negative line number for end point. Assume that lines are sorted
 * in increasing angle order and angles of points and degenerated
 * lines are set to 9 (ignored).
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] line current line id, negative if request for node 2
 * \param[in] type line type (GV_LINE, GV_BOUNDARY or both)
 *
 * \return 1 angles differ
 * \return 0 angle of a line up or down is identical
 */
int dig_node_angle_check(struct Plus_head *plus, plus_t line, int type)
{
    int next, prev;
    float angle1, angle2;
    plus_t node = 0;
    struct P_line *Line;

    G_debug(3, "dig_node_angle_check: line = %d, type = %d", line, type);

    Line = plus->Line[abs(line)];
    if (!(Line->type & GV_LINES))
	return 0;

    if (line > 0) {
	if (Line->type == GV_LINE) {
	    struct P_topo_l *topo = (struct P_topo_l *)Line->topo;
	    node = topo->N1;
	}
	else if (Line->type == GV_BOUNDARY) {
	    struct P_topo_b *topo = (struct P_topo_b *)Line->topo;
	    node = topo->N1;
	}
    }
    else {
	if (Line->type == GV_LINE) {
	    struct P_topo_l *topo = (struct P_topo_l *)Line->topo;
	    node = topo->N2;
	}
	else if (Line->type == GV_BOUNDARY) {
	    struct P_topo_b *topo = (struct P_topo_b *)Line->topo;
	    node = topo->N2;
	}
    }

    angle1 = dig_node_line_angle(plus, node, line);

    /* Next */
    next = dig_angle_next_line(plus, line, GV_RIGHT, type, &angle2);
    /* angle2 = dig_node_line_angle(plus, node, next); */
    if (angle1 == angle2) {
	G_debug(3,
		"  The line to the right has the same angle: node = %d, line = %d",
		node, next);
	return 0;
    }

    /* Previous */
    prev = dig_angle_next_line(plus, line, GV_LEFT, type, &angle2);
    /* angle2 = dig_node_line_angle(plus, node, prev); */
    if (angle1 == angle2) {
	G_debug(3,
		"  The line to the left has the same angle: node = %d, line = %d",
		node, next);
	return 0;
    }

    return 1;			/* OK */
}

/*!
 * \brief Allocate space for new island and create boundary info from array.
 *
 * The order of input lines is expected to be counter clockwise.
 * Then for each line in isle, update line (right,left) info.
 *
 *  Area number the island is within is not filled.
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] n_lines number of lines
 * \param[in] lines array of lines, negative for reverse direction 
 * \param[in] box bounding box
 *
 * \return number of new isle
 * \return -1 on error
 */
int dig_add_isle(struct Plus_head *plus, int n_lines, plus_t * lines,
		 struct bound_box *box)
{
    register int i;
    register int isle, line;
    struct P_isle *Isle;
    struct P_line *Line;
    struct P_topo_b *topo;

    G_debug(3, "dig_add_isle():");
    /* First look if we have space in array of pointers to isles
     *  and reallocate if necessary */
    if (plus->n_isles >= plus->alloc_isles) {	/* array is full */
	if (dig_alloc_isles(plus, 1000) == -1)
	    return -1;
    }

    /* allocate isle structure */
    isle = plus->n_isles + 1;
    Isle = dig_alloc_isle();
    if (Isle == NULL)
	return -1;

    if ((dig_isle_alloc_line(Isle, n_lines)) == -1)
	return -1;

    Isle->area = 0;

    for (i = 0; i < n_lines; i++) {
	line = lines[i];
	G_debug(3, " i = %d line = %d", i, line);
	Isle->lines[i] = line;
	Line = plus->Line[abs(line)];
	topo = (struct P_topo_b *)Line->topo;
	if (plus->uplist.do_uplist)
	    dig_line_add_updated(plus, abs(line));
	if (line < 0) {		/* revers direction -> isle on left */
	    if (topo->left != 0) {
		G_warning(_("Line %d already has area/isle %d to left"), line,
			  topo->left);
		return -1;
	    }
	    topo->left = -isle;
	}
	else {
	    if (topo->right != 0) {
		G_warning(_("Line %d already has area/isle %d to right"), line,
			  topo->right);
		return -1;
	    }

	    topo->right = -isle;
	}
    }

    Isle->n_lines = n_lines;

    plus->Isle[isle] = Isle;

    dig_spidx_add_isle(plus, isle, box);

    plus->n_isles++;

    return (isle);
}


/*!
 * \brief Delete island from Plus_head structure
 *
 * Reset references to it in lines and area outside.
 *
 * \param[in] plus pointer to Plus_head structure
 * \param[in] isle isle id
 *
 * \return 1
 */
int dig_del_isle(struct Plus_head *plus, int isle)
{
    int i, line;
    struct P_line *Line;
    struct P_isle *Isle;
    struct P_topo_b *topo;

    G_debug(3, "dig_del_isle() isle =  %d", isle);
    Isle = plus->Isle[isle];

    dig_spidx_del_isle(plus, isle);

    /* Set area for all lines to 0 */
    for (i = 0; i < Isle->n_lines; i++) {
	line = Isle->lines[i];	/* >0 = clockwise -> right, <0 = counterclockwise ->left */
	Line = plus->Line[abs(line)];
	topo = (struct P_topo_b *)Line->topo;
	if (plus->uplist.do_uplist)
	    dig_line_add_updated(plus, abs(line));
	if (line > 0)
	    topo->right = 0;
	else
	    topo->left = 0;
    }

    /* Delete reference from area it is within */
    G_debug(3, "  area outside isle = %d", Isle->area);
    if (Isle->area > 0) {
	if (plus->Area[Isle->area] == NULL) {
	    G_fatal_error(_("Attempt to delete isle %d info from dead area %d"),
			  isle, Isle->area);
	}
	else {
	    dig_area_del_isle(plus, Isle->area, isle);
	}
    }

    /* free structures */
    dig_free_isle(Isle);
    plus->Isle[isle] = NULL;

    return 1;
}
