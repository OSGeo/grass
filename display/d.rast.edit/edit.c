/*
 * Chris Rewerts, Agricultural Engineering, Purdue University April 1991
 * 
 * The material here based originally on Dwhat. Idea is to allow user to mouse
 * on a cell layer on the graphics window, and we figure out which location
 * (row and col) in the raster map corresponds to the location clicked on the
 * screen. We find "real" row and col based on the cellhd window, to allow
 * for the displayed cell map to be in any given "current" window.
 * 
 * December 1993 Chris Rewerts, CERL, Made changes in the method to check the
 * current region against that of the original map's header.
 * 
 */

#include <string.h>
#include <ctype.h>
#include "edit.h"
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

int edit(void)
{
    char string[10], val_str[100], val_str1[100], val_str2[100];
    char line[128];
    int num_ok;
    int t, b, l, r;
    int select_count = 0;
    int i;
    int row = 0, col = 0, real_row = 0, real_col = 0;
    DCELL *buf, c, num;
    struct Cell_head edit_window;
    int screen_x, screen_y;
    double east, north;
    int button;
    int fd;
    int red, green, blue;
    int D_y, D_x;
    double D_ew, D_ns;
    double D_north, D_south, D_east, D_west;

    double x1, y1, x2, y2;
    double edit_win_ewdist, edit_win_nsdist;
    double real_win_ewdist, real_win_nsdist;
    double pct_ewdiff, pct_nsdiff;

    sprintf(val_str1, "%.10f", min_value);
    sprintf(val_str2, "%.10f", max_value);
    G_trim_decimal(val_str1);
    G_trim_decimal(val_str2);
    fd = G_open_cell_old(current_name, current_mapset);
    if (fd < 0) {
	sprintf(line, "unable to open [%s] in [%s]\n",
		current_name, current_mapset);
	error(1, line);
    }
    G__get_window(&edit_window, "", "WIND", user_mapset);

    /*
     * set the window to the user's current region, and do test to check
     * the ns & ew distance of resolution in meters we use G_distance to
     * give us the meters from points plus the resolution (for ew and
     * ns).
     */

    G_set_window(&edit_window);

    x1 = edit_window.west;
    x2 = edit_window.west + edit_window.ew_res;
    y1 = edit_window.south;
    y2 = edit_window.south + edit_window.ns_res;
    edit_win_ewdist = G_distance(x1, y1, x2, y1);
    edit_win_nsdist = G_distance(x1, y1, x1, y2);

    /*
     * now set the window to the map's header, and do test to check the
     * distance of resolution in meters
     */

    G_set_window(&real_window);

    x1 = real_window.west;
    x2 = real_window.west + real_window.ew_res;
    y1 = real_window.south;
    y2 = real_window.south + real_window.ns_res;
    real_win_ewdist = G_distance(x1, y1, x2, y1);
    real_win_nsdist = G_distance(x1, y1, x1, y2);

    /*
     * now see if the difference is significant enough to munge what we
     * have to do in the point/click/edit process. for both ns and ew, we
     * calc the percent difference between the real (cellhd) window and
     * the current (edit) window meters of distance we found. our
     * arbitrary cutoff is 1 percent
     */

    pct_ewdiff = ((MAX(real_win_ewdist, edit_win_ewdist) /
		   MIN(real_win_ewdist, edit_win_ewdist)) * 100) - 100;
    pct_nsdiff = ((MAX(real_win_nsdist, edit_win_nsdist) /
		   MIN(real_win_nsdist, edit_win_nsdist)) * 100) - 100;

    /*
       fprintf (stdout,"\new: %.20f%%\n", pct_ewdiff);
       fprintf (stdout,"\nns: %.20f%%\n", pct_nsdiff);
     */

    /* set window back to the edit window */

    G_set_window(&edit_window);

    /*
     * check to make sure that the current resolution matches the
     * resolution according to that we read from the cell header by
     * comparing the percentage difference we just calculated
     */

    if ((pct_ewdiff > 1.0) || (pct_nsdiff > 1.0)) {
	error(1,
	      "current window resolution does not match cell layer's resolution");
    }
    if (D_check_map_window(&edit_window))
	error(1, "unable to set graphics window");
    /* Determine conversion factors */
    if (D_get_screen_window(&t, &b, &l, &r))
	error(1, "Getting graphics window coordinates");
    if (D_do_conversions(&edit_window, t, b, l, r))
	error(1, "Error in calculating conversions");

    if (G_read_cats(current_name, current_mapset, &cats) < 0) {
	fprintf(stderr, "could not read cats for [%s]\n", current_name);
    }
    draw_grid();

    buf = G_allocate_d_raster_buf();
    screen_x = ((int)D_get_d_west() + (int)D_get_d_east()) / 2;
    screen_y = ((int)D_get_d_north() + (int)D_get_d_south()) / 2;

    /*
     * info needed to draw over an edited cell with the color of the new
     * cat
     */

    D_east = D_get_d_east();
    D_west = D_get_d_west();
    D_north = D_get_d_north();
    D_south = D_get_d_south();

    /* how many screen distance units for each cell */

    D_ew = (D_east - D_west) / edit_window.cols;
    D_ns = (D_south - D_north) / edit_window.rows;

    /*
     * edit mode - taking commands from mouse key presses at locations on
     * the graphic monitor
     */

    edit_mouse_info();

    c = num = 0.;
    while (1) {
	/* find our the screen x,y location of mouse click */
	R_get_location_with_pointer(&screen_x, &screen_y, &button);
	if (select_count > 12) {
	    edit_mouse_info();
	    select_count = 2;
	}
	/* right key means quit edit mode */
	if (button == 3) {
	    G_close_cell(fd);
	    break;
	}
	/* if left or middle key, find out what's there */
	if (button == 1 || button == 2) {
	    /* convert display coords to UTM coords */
	    east = D_d_to_u_col((double)screen_x);
	    north = D_d_to_u_row((double)screen_y);
	    /* find out what row and col for current window */
	    row = (edit_window.north - north) / edit_window.ns_res;
	    col = (east - edit_window.west) / edit_window.ew_res;

	    /*
	     * find out what row and col this is in the cell
	     * layer as a whole
	     */
	    real_row = (real_window.north - north) / real_window.ns_res;
	    real_col = (east - real_window.west) / real_window.ew_res;


	    /*
	     * check to see if we are in the current window on
	     * the screen i.e. there is a cell actually displayed
	     * where they clicked
	     */
	    if (east < edit_window.west || east > edit_window.east
		|| north < edit_window.south || north > edit_window.north) {
		error(0, "Selection off current screen window.\n");
		continue;
	    }
	    /*
	     * check to see if we are within the area of the
	     * window described by the cell header (the "real
	     * window")
	     */

	    if (east < real_window.west || east > real_window.east
		|| north < real_window.south || north > real_window.north) {
		error(0, "Selection off map's window.\n");
		continue;
	    }
	    /* find out what value is currently in that cell */
	    if (G_get_d_raster_row(fd, buf, row) < 0)
		error(1, "could not get values for selected map row");

	    if (G_is_d_null_value(&buf[col]))
		sprintf(val_str, "%s", "Null");
	    else {
		sprintf(val_str, "%.10f", buf[col]);
		G_trim_decimal(val_str);
	    }
	    fprintf(stdout, "\nCurrent value for the selected cell: %s\n",
		    val_str);
	    fprintf(stdout, "%s\n", G_get_d_raster_cat(&buf[col], &cats));
	}
	/* middle button means we want to edit that cell */
	if (button == 2) {
	    int tmpx, tmpy;

	    edit_mouse_info2(c, buf[col]);
	    R_get_location_with_pointer(&tmpx, &tmpy, &button);

	    if (button == 3)
		while (1) {
		    /*
		     * get new value from user, make sure
		     * it is a number within the range of
		     * the current cats
		     */
		    fprintf(stdout,
			    "\n\n   Enter new cell value (N for null) -> ");

		    G_gets(string);

		    /* error check the user's input */

		    num_ok = 1;
		    if (strcmp(string, "N") == 0)
			G_set_d_null_value(&num, 1);
		    else {
			if ((!isdigit(string[0])) && (string[0] != '-') &&
			    !(map_type != CELL_TYPE && string[0] == '.'))
			    num_ok = 0;
			for (i = 1; i < strlen(string); i++) {
			    if (!isdigit(string[i]) &&
				!(map_type != CELL_TYPE && string[i] == '.'))
				num_ok = 0;
			}
			if (!num_ok) {
			    sprintf(line,
				    "enter %s value between %s and %s\n",
				    (map_type ==
				     CELL_TYPE) ? "an integer" : "a",
				    val_str1, val_str2);

			    error(0, line);
			    continue;
			}
			sscanf(string, "%lf", &num);
			if ((num < min_value) || (num > max_value)) {
			    sprintf(line,
				    "enter %s value between %s and %s\n",
				    (map_type ==
				     CELL_TYPE) ? "an integer" : "a",
				    val_str1, val_str2);
			    error(0, line);
			    continue;
			}
		    }
		    break;
		}
	    if (button == 2 || button == 3) {

		select_count = 2;
		c = num;
		do_edit(real_row, real_col, c);
		change_made = 1;

		/*
		 * type mismatch calls to the color routine,
		 * may not work on all compilers
		 */

		G_get_d_raster_color(&c, &red, &green, &blue, &colr);
		R_RGB_color(red, green, blue);
		D_y = (int)(0.5 + (row * D_ns + D_north));
		D_x = (int)(0.5 + (col * D_ew + D_west));

		/* draw filled box */
		R_box_abs(D_x + 1, D_y + 1, (D_x + (int)(0.5 + D_ew)),
			  (D_y + (int)(0.5 + D_ns)));

		/* draw cleanup frame */
		R_move_abs(D_x, D_y);
		R_standard_color(grid_color);
		R_cont_rel(0, (int)(0.5 + D_ns));
		R_cont_rel((int)(0.5 + D_ew), 0);
		R_cont_rel(0, -1 * (int)(0.5 + D_ns));
		R_cont_rel(-1 * (int)(0.5 + D_ew), 0);

		/* draw "X" */
		R_standard_color(grid_color);
		R_move_abs(D_x + (int)(D_ew * 0.2), D_y + (int)(D_ns * 0.2));
		R_cont_rel((int)(D_ew * 0.6), (int)(D_ns * 0.6));
		R_move_rel(0, (int)(D_ns * -0.6));
		R_cont_rel((int)(D_ew * -0.6), (int)(D_ns * 0.6));

		R_stabilize();
	    }
	    edit_mouse_info();
	}
    }

    return 0;
}

int edit_mouse_info(void)
{
    G_clear_screen();
    fprintf(stderr, "\n     +--------EDIT MODE mouse button menu--------+\n");
    fprintf(stderr, "     |       Use mouse on graphics monitor       |\n");
    fprintf(stderr, "     |                                           |\n");
    fprintf(stderr, "     |left   button:     What's here?            |\n");
    fprintf(stderr, "     |middle button:     Edit cell value         |\n");
    fprintf(stderr, "     |right  button:     Quit edit mode          |\n");
    fprintf(stderr, "     +-------------------------------------------+\n\n");

    return 0;
}

int edit_mouse_info2(DCELL def, DCELL current)
{
    char cur_str[100], def_str[100];
    DCELL tmp;

    G_clear_screen();
    tmp = current;
    if (G_is_d_null_value(&tmp))
	sprintf(cur_str, "Null");
    else {
	sprintf(cur_str, "%.10f", current);
	G_trim_decimal(cur_str);
    }
    tmp = def;
    if (G_is_d_null_value(&tmp))
	sprintf(def_str, "Null");
    else {
	sprintf(def_str, "%.10f", def);
	G_trim_decimal(def_str);
    }
    fprintf(stderr, "\n     +----------Select value button menu---------+\n");
    fprintf(stderr, "     |       Use mouse on graphics monitor       |\n");
    fprintf(stderr, "     |                                           |\n");
    fprintf(stderr, "     |       Current value: %5s                |\n",
	    cur_str);
    fprintf(stderr, "     |                                           |\n");
    fprintf(stderr, "     |left   button:     Cancel                  |\n");
    fprintf(stderr, "     |middle button:     Use value %5s?        |\n",
	    def_str);
    fprintf(stderr, "     |right  button:     Select new value        |\n");
    fprintf(stderr, "     +-------------------------------------------+\n\n");

    return 0;
}

int use_mouse(void)
{
    G_clear_screen();
    fprintf(stderr, "\n     +-------------------------------------------+\n");
    fprintf(stderr, "     |       Use mouse on graphics monitor       |\n");
    fprintf(stderr, "     |       to make selection on the menu       |\n");
    fprintf(stderr, "     +-------------------------------------------+\n\n");

    return 0;
}
