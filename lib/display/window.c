/*
 * D_new_window(name, t, b, l, r)
 *   creates a new window with given coordinates
 *   if "name" is an empty string, the routine returns a unique
 *   string in "name"
 *
 * D_reset_screen_window(t, b, l, r)
 *   resets the edges of the current window
 *
 * D_set_cur_wind(name)
 *   saves "name" in cur_w field in "no-name" pad
 *   outlines previous current window in GRAY
 *   outlines "name" in DEFAULT_FG_COLOR
 *
 * D_get_cur_wind(name)
 *   gets the current name stored in cur_w field in "no_name" pad
 *
 * D_show_window(color)
 *   outlines current window in color (from ../colors.h)
 *
 * D_get_screen_window(t, b, l, r)
 *   returns current window's coordinates 
 *
 * D_check_map_window(wind)
 *   if map window (m_win) already assigned
 *       map window is read into the struct "wind"
 *   else
 *       struct "wind" is written to map window (m_win)
 *
 * D_remove_window()
 *   remove any trace of window
 *
 * D_erase_window()
 *   Erases the window on scree.  Does not affect window contents list.
 */

#include <string.h>
#include <grass/colors.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>


/*!
 * \brief create new graphics frame
 *
 * Creates a new frame <b>name</b> with
 * coordinates <b>top, bottom, left</b>, and <b>right.</b> If <b>name</b>
 * is the empty string '''' (i.e., *<b>name</b> = = 0), the routine returns a
 * unique string in <b>name.</b>
 *
 *  \param name
 *  \param top
 *  \param bottom
 *  \param left
 *  \param right
 *  \return int
 */

int D_new_window(char *name , int t,int  b,int  l,int  r)
{
	int stat;
	char buff[256];

/* If no name was sent, get a unique name for the window */
	if(! *name)
		R_pad_invent(name) ;

/* Create the work pad */
	if ((stat = R_pad_create (name)))
	{
		R_pad_perror (name, stat);
		return(-1) ;
	}

/* Select work pad for use */
	if ((stat = R_pad_select (name)))
		goto pad_error ;

/* Timestamp current pad */
	D_timestamp() ;

	sprintf (buff, "%d %d %d %d", t, b, l, r) ;
	if ((stat = R_pad_set_item ("d_win", buff)))
		goto pad_error ;

/* Display outline of new window */
	D_show_window(GRAY) ;

	R_set_window(t, b, l, r) ;

	return(0) ;

pad_error:
	R_pad_delete();
	sprintf (buff, "window <%s>, item <%s>", name, "d_win");
	R_pad_perror (buff, stat);

	return(-1) ;
}


/*!
 * \brief create new graphics frame, with coordinates in percent
 *
 * Creates a new frame <b>name</b> with coordinates <b>top, bottom,
 * left</b>, and <b>right</b> as percentages of the screen size.
 * If <b>name</b> is the empty string "" (i.e., *<b>name</b> == 0),
 * the routine returns a unique string in <b>name.</b>
 *
 *  \param name
 *  \param bottom
 *  \param top
 *  \param left
 *  \param right
 *  \return int
 */

int D_new_window_percent(char *name, float b, float t, float l, float r)
{
	int scr_t = R_screen_top();
	int scr_b = R_screen_bot();
	int scr_l = R_screen_left();
	int scr_r = R_screen_rite();

	int win_t = 0.5 + scr_t + (scr_b - scr_t) * (100. - t) / 100.0;
	int win_b = 0.5 + scr_t + (scr_b - scr_t) * (100. - b) / 100.0;
	int win_l = 0.5 + scr_l + (scr_r - scr_l) * l / 100.0;
	int win_r = 0.5 + scr_l + (scr_r - scr_l) * r / 100.0;

	if (win_t < scr_t) win_t = scr_t;
	if (win_b > scr_b) win_b = scr_b;
	if (win_l < scr_l) win_l = scr_l;
	if (win_r > scr_r) win_r = scr_r;

	return D_new_window(name, win_t, win_b, win_l, win_r);
}


/*!
 * \brief set current graphics frame
 *
 * Selects the frame <b>name</b> to be the current frame. The previous current frame
 * (if there was one) is outlined in grey. The selected current frame is outlined
 * in white.
 *
 *  \param name
 *  \return int
 */

int D_set_cur_wind(const char *name )
{
	char pad_cur[64] ;
	int stat ;
	int not_same_window ;
	int t, b, l, r ;

	/* Abort if window name is null */
	if (! strlen(name))
		return(-1) ;

	/* Abort if window name is not available */
	if ((stat = R_pad_select(name)))
		return(stat) ;
	
	/* Get name of current window pad */
	D_get_cur_wind(pad_cur) ;

	/* Establish whether it is the same as the currently selected pad */
	if (strlen(pad_cur))
	{
		not_same_window = strcmp(name, pad_cur) ;
		if(not_same_window)
		{
			R_pad_select(pad_cur) ;
			D_show_window(GRAY) ;
		}
	}
	else
	{
		not_same_window = 1 ;
	}

	if(not_same_window)
	{
	/* Delete the current window name in no-name pad */
		R_pad_select("") ;
		if ((stat = R_pad_delete_item("cur_w")))
			return(stat) ;

	/* Update the current window name in no-name pad */
		if ((stat = R_pad_set_item ("cur_w", name)))
			return(stat) ;

	/* Select new window pad */
		if ((stat = R_pad_select(name)))
			return(stat) ;
		
	/* Outline new window in highlight color */
		D_show_window(D_translate_color(DEFAULT_FG_COLOR)) ;

	/* Tell driver of current window */
		D_get_screen_window(&t, &b, &l, &r) ;
		R_set_window(t, b, l, r) ;
	}
	else
	{
	/* Select new window pad */
		if ((stat = R_pad_select(name)))
			return(stat) ;
	}

	return(0) ;
}


/*!
 * \brief identify current graphics frame
 *
 * Captures the name of the current frame in string <b>name.</b>
 *
 *  \param name
 *  \return int
 */

int D_get_cur_wind( char *name )
{
	int count;
	int stat ;
	char **list ;

	if ((stat = R_pad_select("")))
		return(stat) ;

	if ((stat = R_pad_get_item ("cur_w", &list, &count)))
	{
		strcpy(name, "") ;
		return(stat) ;
	}

	strcpy(name, list[0]) ;
	R_pad_freelist (list,count) ;
	R_pad_select(name) ;
	return(0) ;
}


/*!
 * \brief outlines current frame
 *
 * Outlines
 * current frame in <b>color.</b> Appropriate colors are found in
 * $GISBASE/src/D/libes/colors.h\remarks{$GISBASE is the directory where GRASS
 * is installed. See UNIX_Environment for details.} and are spelled
 * with lowercase letters.
 *
 *  \param color
 *  \return int
 */

int D_show_window( int color )
{
	int t, b, l, r ;
	int stat ;

	if ((stat = D_get_screen_window(&t, &b, &l, &r)))
		return(stat) ;

	R_set_window(t-1, b+1, l-1, r+1) ;

	R_standard_color(color) ;
	R_move_abs(l-1, b) ;
	R_cont_abs(l-1, t-1) ;
	R_cont_abs(r, t-1) ;
	R_cont_abs(r, b) ;
	R_cont_abs(l-1, b) ;
	R_flush() ;

	R_set_window(t, b, l, r) ;

	return(0) ;
}


/*!
 * \brief retrieve current frame coordinates
 *
 * Returns current frame's
 * coordinates in the pointers <b>top, bottom, left</b>, and <b>right.</b>
 *
 *  \param top
 *  \param bottom
 *  \param left
 *  \param right
 *  \return int
 */

int D_get_screen_window(int *t,int *b,int *l,int *r)
{
	int stat ;
	int count ;
	char **list ;

	if ((stat = R_pad_get_item ("d_win", &list, &count)))
		return(stat) ;

	sscanf (list[0], "%d %d %d %d", t, b, l, r) ;

	R_pad_freelist (list,count) ;

	return(0) ;
}


/*!
 * \brief assign/retrieve current map region
 *
 * Graphics frames can have GRASS map regions associated with
 * them. This routine passes the map <b>region</b> to the current graphics
 * frame. If a GRASS region is already associated with the graphics frame, its
 * information is copied into <b>region</b> for use by the calling module.
 * Otherwise <b>region</b> is associated with the current graphics frame.
 * Note this routine is called by <i>D_setup.</i>
 *
 *  \param region
 *  \return int
 */

int D_check_map_window(struct Cell_head *wind )
{
	char buff[256] ;
	char ebuf[64], nbuf[64], sbuf[64], wbuf[64];
	int num ;
	int count ;
	char **list ;
	char *err;

	if (0 != R_pad_get_item ("m_win", &list, &count))
	{
		G_format_easting (wind->east, ebuf, wind->proj);
		G_format_easting (wind->west, wbuf, wind->proj);
		G_format_northing (wind->north, nbuf, wind->proj);
		G_format_northing (wind->south, sbuf, wind->proj);
		sprintf (buff, "%d %d %s %s %s %s %d %d",
			wind->proj, wind->zone,
			ebuf, wbuf,
			nbuf, sbuf,
			wind->rows, wind->cols) ;
		if(R_pad_set_item ("m_win", buff))
			return(-1) ;
		return(0) ;
	}
	else
	{
		num = sscanf (list[0], "%d %d %s %s %s %s %d %d",
			&wind->proj, &wind->zone,
			ebuf, wbuf,
			nbuf, sbuf,
			&wind->rows, &wind->cols);

		R_pad_freelist (list,count) ;

		if (num != 8) return -2;

		if (!G_scan_easting(ebuf, &wind->east, wind->proj)) return -2;
		if (!G_scan_easting(wbuf, &wind->west, wind->proj)) return -2;
		if (!G_scan_northing(nbuf, &wind->north, wind->proj)) return -2;
		if (!G_scan_northing(sbuf, &wind->south, wind->proj)) return -2;

		if ((err = G_adjust_Cell_head(wind, 1, 1)))
		    return -2;

		return 0;
	}
}


/*!
 * \brief resets current frame position
 *
 * Re-establishes the screen position of a
 * frame at the location specified by <b>top, bottom, left, and right.</b>
 *
 *  \param top
 *  \param bottom
 *  \param left
 *  \param right
 *  \return int
 */

int D_reset_screen_window(int t,int b,int l,int r)
{
	int stat;
	char buff[256];

	D_show_window(D_translate_color(DEFAULT_BG_COLOR)) ;

	sprintf (buff, "%d %d %d %d", t, b, l, r) ;
	R_pad_delete_item("d_win") ;
	if ((stat = R_pad_set_item("d_win", buff)))
		return(stat) ;

	D_show_window(D_translate_color(DEFAULT_FG_COLOR)) ;

	return(0) ;
}


/*!
 * \brief give current time to frame
 *
 * Timestamp the current
 * frame. This is used primarily to identify which frames are on top of other,
 * specified frames.
 *
 *  \param ~
 *  \return int
 */

int D_timestamp(void)
{
	char buff[128] ;
	int stat ;
	int count ;
	char **list ;
	char cur_pad[64] ;
	int cur_time ;

	R_pad_current(cur_pad) ;

	R_pad_select("") ;
	if ((stat = R_pad_get_item("time", &list, &count)))
	{
		R_pad_set_item("time", "1") ;
		R_pad_select(cur_pad) ;
		R_pad_set_item("time", "1") ;
		return(1) ;
	}

	sscanf(list[0],"%d",&cur_time) ;
	sprintf(buff,"%d",cur_time+1) ;
	R_pad_set_item("time", buff) ;

	R_pad_freelist (list,count) ;

	R_pad_select(cur_pad) ;

	R_pad_delete_item("time") ;
	return (R_pad_set_item ("time", buff)) ;
}


/*!
 * \brief remove a frame
 *
 * Removes any trace of the
 * current frame.
 *
 *  \param ~
 */

void D_remove_window(void)
{
	R_pad_delete();
	R_pad_select("");
	R_pad_delete_item("cur_w") ;
}

/*!
 * \brief erase current frame
 *
 * Erases the frame on the
 * screen using the currently selected color.
 *
 *  \param ~
 */

void D_erase_window(void)
{
	int t, b, l, r;

	D_get_screen_window(&t, &b, &l, &r);
	R_box_abs(l, t, r, b);
	R_flush();
}

void D_erase(const char *color)
{
	int t, b, l, r;
	int colorindex;

	D_get_screen_window(&t, &b, &l, &r);
	D_clear_window();

	/* Parse and select background color */
	colorindex = D_parse_color (color, 0) ;
	D_raster_use_color(colorindex);

	/* Do the plotting */
	R_box_abs(l, t, r, b);

	/* Add erase item to the pad */
	D_set_erase_color(color);
}

void D_remove_windows(void)
{
	char **pads;
	int npads;
	int i;

	R_pad_list (&pads, &npads);

	R_pad_select("");
	R_pad_delete_item("time") ;
	R_pad_delete_item("cur_w") ;

	for (i = 0; i < npads; i++)
	{
		R_pad_select(pads[i]);
		R_pad_delete();
	}
}

void D_full_screen(void)
{
	D_remove_windows();

	D_new_window_percent("full_screen", 0.0, 100.0, 0.0, 100.0);
	if (D_set_cur_wind("full_screen") == 0)
		D_timestamp();

	R_standard_color(D_translate_color(DEFAULT_BG_COLOR)) ;
	R_erase() ;
}
