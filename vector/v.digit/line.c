#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/form.h>
#include "global.h"
#include "proto.h"

int write_line(struct Map_info *Map, int type, struct line_pnts *Points)
{
    int i, field, cat, ret;
    static int first_form = 1;
    char *form;
    struct line_cats *Cats;
    struct field_info *Fi;
    dbString html;

    Cats = Vect_new_cats_struct();
    db_init_string(&html);

    cat = var_geti(VAR_CAT);
    if (var_geti(VAR_CAT_MODE) != CAT_MODE_NO && cat > 0 &&
	var_geti(VAR_FIELD) > 0) {
	Vect_cat_set(Cats, var_geti(VAR_FIELD), cat);

	G_debug(2, "write field = %d cat = %d", var_geti(VAR_FIELD),
		var_geti(VAR_CAT));

	if (cat_max_get(var_geti(VAR_FIELD)) < var_geti(VAR_CAT)) {
	    cat_max_set(var_geti(VAR_FIELD), var_geti(VAR_CAT));
	}
    }

    ret = Vect_write_line(Map, type, Points, Cats);

    for (i = 0; i < Vect_get_num_updated_lines(Map); i++)
	G_debug(2, "Updated line: %d", Vect_get_updated_line(Map, i));

    for (i = 0; i < Vect_get_num_updated_nodes(Map); i++)
	G_debug(2, "Updated node: %d", Vect_get_updated_node(Map, i));

    /* Reset category (this automaticaly resets cat for next not used) */
    var_seti(VAR_FIELD, var_geti(VAR_FIELD));

    if (var_geti(VAR_CAT_MODE) != CAT_MODE_NO && var_geti(VAR_INSERT) &&
	cat > 0) {
	G_debug(2, "Insert new record");
	db_set_string(&html, "<HTML><HEAD><TITLE>Form</TITLE><BODY>");

	field = var_geti(VAR_FIELD);
	ret = new_record(field, cat);
	if (ret == -1) {
	    return -1;
	}
	else if (ret == 0) {
	    db_append_string(&html, "New record was created.<BR>");
	}
	else {			/* record already existed */
	    db_append_string(&html,
			     "Record for this category already existed.<BR>");
	}

	/* Open form */
	Fi = Vect_get_field(Map, field);
	if (Fi == NULL) {
	    return -1;
	}
	F_generate(Fi->driver, Fi->database, Fi->table, Fi->key, cat, NULL,
		   NULL, F_EDIT, F_HTML, &form);
	db_append_string(&html, form);
	db_append_string(&html, "</BODY></HTML>");

	/* Note: F_open() must be run first time with closed monitor, otherwise next
	 *        attempt to open driver hangs until form child process is killed */
	if (first_form)
	    driver_close();
	F_clear();
	F_open("Attributes", db_get_string(&html));
	if (first_form) {
	    driver_open();
	    first_form = 0;
	}

	G_free(form);
	db_free_string(&html);

    }

    return 0;
}

/* Snap to node */
int snap(double *x, double *y)
{
    int node;
    double thresh;

    G_debug(2, "snap(): x = %f, y = %f", *x, *y);

    thresh = get_thresh();

    node = Vect_find_node(&Map, *x, *y, 0, thresh, 0);

    if (node > 0)
	Vect_get_node_coor(&Map, node, x, y, NULL);

    G_debug(2, "node = %d x = %f, y = %f", node, *x, *y);
    return node;
}

/* Digitize new line */
struct new_line
{
    int type;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int first;
};

int new_line_begin(void *closure)
{
    struct new_line *nl = closure;
    char buf[1000];

    G_debug(2, "new_line(): type = %d", nl->type);

    nl->Points = Vect_new_line_struct();
    nl->Cats = Vect_new_cats_struct();

    sprintf(buf, "Digitize new %s:", get_line_type_name(nl->type));
    i_prompt(buf);
    i_prompt_buttons("New point", "", "Quit tool");

    i_new_line_options(1);

    nl->first = 1;

    set_mode(MOUSE_POINT);

    return 0;
}

int new_line_update(void *closure, int sxn, int syn, int button)
{
    struct new_line *nl = closure;
    double x = D_d_to_u_col(sxn);
    double y = D_d_to_u_row(syn);

    G_debug(3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    if (nl->first && button == 3) {	/* Quit tool ( points & lines ), first is always for points */
	Tool_next = TOOL_NOTHING;
	return 1;
    }

    if (nl->type & GV_POINTS) {
	/* We can get here with button = 1 or 2 -> the same write point */
	snap(&x, &y);
	Vect_append_point(nl->Points, x, y, 0);

	write_line(&Map, nl->type, nl->Points);
	updated_lines_and_nodes_erase_refresh_display();
	return 1;
    }
    else {			/* GV_LINES */
	/* Button may be 1,2,3 */
	if (button == 1) {	/* New point */
	    snap(&x, &y);
	    Vect_append_point(nl->Points, x, y, 0);

	    if (nl->type == GV_LINE)
		symb_set_driver_color(SYMB_LINE);
	    else
		symb_set_driver_color(SYMB_BOUNDARY_0);

	    display_points(nl->Points, 1);
	    set_location(D_u_to_d_col(x), D_u_to_d_row(y));
	    nl->first = 0;
	    set_mode(MOUSE_LINE);
	}
	else if (button == 2) {	/* Undo last point */
	    if (nl->Points->n_points >= 1) {
		symb_set_driver_color(SYMB_BACKGROUND);
		display_points(nl->Points, 1);
		nl->Points->n_points--;

		if (nl->type == GV_LINE)
		    symb_set_driver_color(SYMB_LINE);
		else
		    symb_set_driver_color(SYMB_BOUNDARY_0);

		display_points(nl->Points, 1);
		set_location(D_u_to_d_col
			     (nl->Points->x[nl->Points->n_points - 1]),
			     D_u_to_d_row(nl->Points->
					  y[nl->Points->n_points - 1])
		    );
	    }
	    if (nl->Points->n_points == 0) {
		i_prompt_buttons("New point", "", "Quit tool");
		nl->first = 1;
		set_mode(MOUSE_POINT);
	    }
	}
	else {			/* button = 3 -> write the line and quit */
	    if (nl->Points->n_points > 1) {
		/* Before the line is written, we must check if connected to existing nodes, if yes,
		 * such nodes must be add to update list before! the line is written (areas/isles */
		int node1 =
		    Vect_find_node(&Map, nl->Points->x[0], nl->Points->y[0],
				   nl->Points->z[0], 0, Vect_is_3d(&Map));
		int i = nl->Points->n_points - 1;
		int node2 =
		    Vect_find_node(&Map, nl->Points->x[i], nl->Points->y[i],
				   nl->Points->z[i], 0, Vect_is_3d(&Map));

		G_debug(2, "  old node1 = %d  old node2 = %d", node1, node2);
		write_line(&Map, nl->type, nl->Points);
		updated_lines_and_nodes_erase_refresh_display();
	    }
	    else
		G_warning("Less than 2 points for line -> nothing written");

	    return 1;
	}
	G_debug(2, "n_points = %d", nl->Points->n_points);
    }

    i_prompt_buttons("New point", "Undo last point", "Close line");
    return 0;
}

int new_line_end(void *closure)
{
    i_prompt("");
    i_prompt_buttons("", "", "");
    i_coor(COOR_NULL, COOR_NULL);
    i_new_line_options(0);

    G_debug(3, "new_line(): End");
    return 1;
}

void new_line(int type)
{
    static struct new_line nl;

    nl.type = type;

    set_tool(new_line_begin, new_line_update, new_line_end, &nl);
}

/* Continue work on the end of a line */

struct edit_line
{
    int phase;
    double thresh;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int line;
    int line_type;
    int reversed;
};

int edit_line_begin(void *closure)
{
    struct edit_line *el = closure;

    G_debug(2, "edit_line()");

    i_prompt("Edit line or boundary:");
    i_prompt_buttons("Select", "", "Quit tool");

    el->thresh = get_thresh();
    G_debug(2, "thresh = %f", el->thresh);

    el->phase = 1;
    set_mode(MOUSE_POINT);

    return 0;
}

void edit_line_phase2(struct edit_line *el, double x, double y)
{
    int node1, node2;
    double nodex, nodey, nodez, dist;

    el->phase = 2;

    el->Points = Vect_new_line_struct();
    el->Cats = Vect_new_cats_struct();
    el->line_type = Vect_read_line(&Map, el->Points, el->Cats, el->line);

    el->reversed = 0;

    /* Find out the node nearest to the line */
    Vect_get_line_nodes(&Map, el->line, &node1, &node2);

    Vect_get_node_coor(&Map, node2, &nodex, &nodey, &nodez);
    dist = (x - nodex) * (x - nodex) + (y - nodey) * (y - nodey);

    Vect_get_node_coor(&Map, node1, &nodex, &nodey, &nodez);
    if ((x - nodex) * (x - nodex) + (y - nodey) * (y - nodey) < dist) {
	/* The first node is the nearest => reverse the line and remember
	 * doing so. */
	Vect_line_reverse(el->Points);
	el->reversed = 1;
    }

    display_node(node1, SYMB_BACKGROUND, 1);
    display_node(node2, SYMB_BACKGROUND, 1);
    i_prompt_buttons("New Point", "Undo Last Point", "Close line");

    set_location(D_u_to_d_col(el->Points->x[el->Points->n_points - 1]),
		 D_u_to_d_row(el->Points->y[el->Points->n_points - 1])
	);
    set_mode(MOUSE_LINE);
}

int edit_line_update(void *closure, int sxn, int syn, int button)
{
    struct edit_line *el = closure;
    double x = D_d_to_u_col(sxn);
    double y = D_d_to_u_row(syn);

    G_debug(3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    if (button == 3)		/* Tool broken by GUI */
	return 1;

    switch (el->phase) {
    case 1:
	if (button != 1)
	    return 0;

	/* Find nearest point or line */
	el->line =
	    Vect_find_line(&Map, x, y, 0, GV_LINE | GV_BOUNDARY, el->thresh,
			   0, 0);
	G_debug(2, "line found = %d", el->line);

	/* Display new selected line if any */
	if (el->line > 0) {
	    display_line(el->line, SYMB_HIGHLIGHT, 1);
	    edit_line_phase2(el, x, y);
	}
	break;

    case 2:
	if (button == 1) {	/* New point */
	    snap(&x, &y);
	    Vect_append_point(el->Points, x, y, 0);

	    if (el->line_type == GV_LINE)
		symb_set_driver_color(SYMB_LINE);
	    else
		symb_set_driver_color(SYMB_BOUNDARY_0);

	    display_points(el->Points, 1);
	    set_location(sxn, syn);
	    i_prompt_buttons("New Point", "Undo Last Point", "Close line");
	}
	else if (button == 2) {	/* Undo last point */
	    if (el->Points->n_points > 1) {
		symb_set_driver_color(SYMB_BACKGROUND);
		display_points(el->Points, 1);

		el->Points->n_points--;

		if (el->line_type == GV_LINE)
		    symb_set_driver_color(SYMB_LINE);
		else
		    symb_set_driver_color(SYMB_BOUNDARY_0);

		display_points(el->Points, 1);
		set_location(D_u_to_d_col
			     (el->Points->x[el->Points->n_points - 1]),
			     D_u_to_d_row(el->Points->
					  y[el->Points->n_points - 1])
		    );
		if (el->Points->n_points == 1)
		    i_prompt_buttons("New Point", "", "Delete line and exit");
	    }
	}
	break;
    }

    return 0;
}

int edit_line_end(void *closure)
{
    struct edit_line *el = closure;

    if (el->phase > 1) {
	if (el->reversed)
	    Vect_line_reverse(el->Points);

	if (el->Points->n_points > 1) {
	    Vect_rewrite_line(&Map, el->line, el->line_type, el->Points,
			      el->Cats);
	    updated_lines_and_nodes_erase_refresh_display();
	}
	else {
	    int i;

	    /* delete lines with less than two points */
	    Vect_delete_line(&Map, el->line);
	    for (i = 0; i < el->Cats->n_cats; i++) {
		check_record(el->Cats->field[i], el->Cats->cat[i]);
	    }
	}


	Vect_destroy_line_struct(el->Points);
	Vect_destroy_cats_struct(el->Cats);
    }

    i_prompt("");
    i_prompt_buttons("", "", "");
    i_coor(COOR_NULL, COOR_NULL);

    G_debug(3, "edit_line(): End");

    return 1;
}

void edit_line(void)
{
    static struct edit_line el;

    set_tool(edit_line_begin, edit_line_update, edit_line_end, &el);
}

/* Delete line */
struct delete_line
{
    double thresh;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int first;
    int line;
    int last_line;
};

int delete_line_begin(void *closure)
{
    struct delete_line *dl = closure;

    G_debug(2, "delete_line()");

    dl->Points = Vect_new_line_struct();
    dl->Cats = Vect_new_cats_struct();

    i_prompt("Delete point, line, boundary, or centroid:");
    i_prompt_buttons("Select", "Unselect", "Quit tool");

    dl->thresh = get_thresh();
    G_debug(2, "thresh = %f", dl->thresh);

    dl->line = 0;
    dl->first = 1;
    dl->last_line = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int delete_line_update(void *closure, int sxn, int syn, int button)
{
    struct delete_line *dl = closure;
    double x = D_d_to_u_col(sxn);
    double y = D_d_to_u_row(syn);

    G_debug(3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    /* Display last highlighted in normal color */
    if (dl->last_line > 0) {
	display_line(dl->last_line, SYMB_DEFAULT, 1);
    }

    if (button == 3)		/* Quit tool */
	return 1;

    if (button == 1) {		/* Confirm / select */
	/* Delete last if any */
	if (dl->last_line > 0) {
	    int node1, node2;
	    int i;

	    /* Erase line and nodes !!! (because if the line is not connected to any other, nodes will die */
	    display_line(dl->last_line, SYMB_BACKGROUND, 1);
	    Vect_get_line_nodes(&Map, dl->line, &node1, &node2);
	    G_debug(2, "delete line = %d node1 = %d node2 = %d",
		    dl->last_line, node1, node2);

	    display_node(node1, SYMB_BACKGROUND, 1);
	    display_node(node2, SYMB_BACKGROUND, 1);

	    Vect_read_line(&Map, NULL, dl->Cats, dl->last_line);
	    Vect_delete_line(&Map, dl->last_line);
	    for (i = 0; i < dl->Cats->n_cats; i++) {
		check_record(dl->Cats->field[i], dl->Cats->cat[i]);
	    }

	    for (i = 0; i < Vect_get_num_updated_lines(&Map); i++)
		G_debug(2, "Updated line: %d",
			Vect_get_updated_line(&Map, i));

	    for (i = 0; i < Vect_get_num_updated_nodes(&Map); i++)
		G_debug(2, "Updated node: %d",
			Vect_get_updated_node(&Map, i));

	    updated_lines_and_nodes_erase_refresh_display();
	}

	/* Find neares point or line */
	dl->line =
	    Vect_find_line(&Map, x, y, 0, GV_POINT | GV_CENTROID, dl->thresh,
			   0, 0);
	G_debug(2, "point found = %d", dl->line);
	if (dl->line == 0)
	    dl->line =
		Vect_find_line(&Map, x, y, 0, GV_LINE | GV_BOUNDARY,
			       dl->thresh, 0, 0);
	G_debug(2, "line found = %d", dl->line);

	/* Display new selected line if any */
	if (dl->line > 0) {
	    display_line(dl->line, SYMB_HIGHLIGHT, 1);
	}
    }
    else {			/* button == 2 -> unselect */
	dl->line = 0;
    }

    if (dl->line > 0)
	i_prompt_buttons("Confirm and select next", "Unselect", "Quit tool");
    else
	i_prompt_buttons("Select", "Unselect", "Quit tool");

    dl->last_line = dl->line;
    dl->first = 0;

    return 0;
}

int delete_line_end(void *closure)
{
    struct delete_line *dl = closure;

    /* Display last highlighted in normal color */
    if (dl->last_line > 0) {
	display_line(dl->last_line, SYMB_DEFAULT, 1);
    }

    i_prompt("");
    i_prompt_buttons("", "", "");
    i_coor(COOR_NULL, COOR_NULL);

    G_debug(3, "delete_line(): End");

    return 1;
}

void delete_line(void)
{
    static struct delete_line dl;

    set_tool(delete_line_begin, delete_line_update, delete_line_end, &dl);
}

/* Move line */
struct move_line
{
    double thresh;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int line;
    int last_line;
    double xo, yo;
};

int move_line_begin(void *closure)
{
    struct move_line *ml = closure;

    G_debug(2, "move_line()");

    ml->Points = Vect_new_line_struct();
    ml->Cats = Vect_new_cats_struct();

    i_prompt("Move point, line, boundary, or centroid:");
    i_prompt_buttons("Select", "", "Quit tool");

    ml->thresh = get_thresh();
    G_debug(2, "thresh = %f", ml->thresh);

    ml->last_line = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int move_line_update(void *closure, int sxn, int syn, int button)
{
    struct move_line *ml = closure;
    double x = D_d_to_u_col(sxn);
    double y = D_d_to_u_row(syn);

    G_debug(3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    if (ml->last_line > 0) {
	display_line(ml->last_line, SYMB_DEFAULT, 1);
    }

    if (button == 3)
	return 1;

    if (button == 1) {		/* Select / new location */
	int type;

	if (ml->last_line == 0) {	/* Select line */
	    ml->line =
		Vect_find_line(&Map, x, y, 0, GV_POINT | GV_CENTROID,
			       ml->thresh, 0, 0);
	    G_debug(2, "point found = %d", ml->line);
	    if (ml->line == 0)
		ml->line =
		    Vect_find_line(&Map, x, y, 0, GV_LINE | GV_BOUNDARY,
				   ml->thresh, 0, 0);
	    G_debug(2, "line found = %d", ml->line);

	    /* Display new selected line if any */
	    if (ml->line > 0) {
		display_line(ml->line, SYMB_HIGHLIGHT, 1);

		/* Find the nearest point on the line */
		type = Vect_read_line(&Map, ml->Points, NULL, ml->line);
		Vect_line_distance(ml->Points, x, y, 0, 0, &ml->xo, &ml->yo,
				   NULL, NULL, NULL, NULL);
		set_location(D_u_to_d_col(ml->xo), D_u_to_d_row(ml->yo));

		i_prompt_buttons("New location", "Unselect", "Quit tool");
	    }
	    ml->last_line = ml->line;
	}
	else {			/* Line is already selected */
	    int node1, node2;
	    int i;

	    display_line(ml->last_line, SYMB_BACKGROUND, 1);
	    Vect_get_line_nodes(&Map, ml->last_line, &node1, &node2);
	    display_node(node1, SYMB_BACKGROUND, 1);
	    display_node(node2, SYMB_BACKGROUND, 1);

	    type = Vect_read_line(&Map, ml->Points, ml->Cats, ml->last_line);
	    for (i = 0; i < ml->Points->n_points; i++) {
		ml->Points->x[i] = ml->Points->x[i] + x - ml->xo;
		ml->Points->y[i] = ml->Points->y[i] + y - ml->yo;
	    }

	    Vect_rewrite_line(&Map, ml->last_line, type, ml->Points,
			      ml->Cats);

	    updated_lines_and_nodes_erase_refresh_display();
	    ml->last_line = 0;
	}

    }
    if (button == 2) {		/* Unselect */
	if (ml->last_line > 0) {
	    ml->last_line = 0;
	}
    }

    if (ml->last_line == 0) {
	i_prompt_buttons("Select", "", "Quit tool");
	set_mode(MOUSE_POINT);
    }
    else
	set_mode(MOUSE_LINE);

    return 0;
}

int move_line_end(void *closure)
{
    struct move_line *ml = closure;

    /* Display last highlighted in normal color */
    if (ml->last_line > 0) {
	display_line(ml->last_line, SYMB_DEFAULT, 1);
    }

    i_prompt("");
    i_prompt_buttons("", "", "");
    i_coor(COOR_NULL, COOR_NULL);

    G_debug(3, "move_line(): End");

    return 1;
}

void move_line(void)
{
    static struct move_line ml;

    set_tool(move_line_begin, move_line_update, move_line_end, &ml);
}
