/*
 ****************************************************************************
 *
 * MODULE:       d.vect
 * AUTHOR(S):    CERL, Radim Blazek, others
 * PURPOSE:      Display the binary vector file that the user wants displayed 
 *               on top of the current image.
 * COPYRIGHT:    (C) 2004-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/Vect.h>
#include <grass/colors.h>
#include <grass/symbol.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "plot.h"
#include "local_proto.h"

/* adopted from r.colors */
static char *icon_files(void)
{
        char *list = NULL, path[4096], path_i[4096];
	size_t len = 0, l;
	DIR *dir, *dir_i;
	struct dirent *d, *d_i;

	sprintf(path, "%s/etc/symbol", G_gisbase());

	dir = opendir(path);
	if (!dir)
		return NULL;

	/*loop over etc/symbol*/
	while ((d = readdir(dir)))
	{
		if (d->d_name[0] == '.')
			continue;
		
		sprintf(path_i, "%s/etc/symbol/%s", G_gisbase(), d->d_name);
		dir_i = opendir(path_i);
		
		if (!dir_i)
			continue;
	   
		/*loop over each directory in etc/symbols*/
		while((d_i = readdir(dir_i)))
		{
			if (d_i->d_name[0] == '.')
				continue;

			l = strlen(d->d_name) + strlen(d_i->d_name) + 3;
			list = G_realloc(list, len + l);
			sprintf(list+len, "%s/%s,", d->d_name, d_i->d_name);
			len += l - 1;
		}

		closedir(dir_i);
	}

	closedir(dir);

	if(len)
		list[len-1] = 0;

	return list;
}

int 
main (int argc, char **argv)
{
	char *mapset;
	int ret, level;
	int i, stat = 0, type, area, display;
	int chcat = 0;
	int r, g, b;
	int has_color, has_fcolor;
	struct color_rgb color, fcolor;
	int size;
	int default_width;
	double width_scale;
	int verbose = FALSE;
	double minreg, maxreg, reg;
	char map_name[128] ;
	struct GModule *module;
	struct Option *map_opt;
	struct Option *color_opt, *fcolor_opt, *rgbcol_opt, *zcol_opt;
	struct Option *type_opt, *display_opt;
	struct Option *icon_opt, *size_opt;
	struct Option *where_opt;
	struct Option *field_opt, *cat_opt, *lfield_opt;
	struct Option *lcolor_opt, *bgcolor_opt, *bcolor_opt;
	struct Option *lsize_opt, *font_opt, *enc_opt, *xref_opt, *yref_opt;
	struct Option *attrcol_opt, *maxreg_opt, *minreg_opt;
	struct Option *width_opt, *wcolumn_opt, *wscale_opt;
	struct Option *render_opt;
	struct Flag   *verbose_flag; /* please remove before GRASS 7 released */
	struct Flag   *id_flag, *table_acolors_flag, *cats_acolors_flag, *x_flag, *zcol_flag;
	struct cat_list *Clist;
	int *cats, ncat;
	LATTR lattr;
        struct Map_info Map;
        struct field_info *fi;
        dbDriver *driver;
        dbHandle handle;
	struct Cell_head window;
	BOUND_BOX box;
	double overlap;
	SYMBOL *Symb;

	/* Initialize the GIS calls */
	G_gisinit(argv[0]) ;

	module = G_define_module();
	module->keywords = _("display, vector");
	module->description =
	    _("Displays vector data in the active "
	      "frame on the graphics monitor.");

	map_opt = G_define_standard_option(G_OPT_V_MAP); 

	display_opt = G_define_option() ;
	display_opt->key        = "display" ;
	display_opt->type       = TYPE_STRING ;
	display_opt->required   = NO ;
	display_opt->multiple   = YES ;
	display_opt->answer     = "shape" ;
	display_opt->options    = "shape,cat,topo,dir,attr,zcoor";
	display_opt->description= _("Display");

	/* Query */
	type_opt = G_define_standard_option(G_OPT_V_TYPE);
	type_opt->answer     = "point,line,boundary,centroid,area,face" ;
	type_opt->options    = "point,line,boundary,centroid,area,face" ;
	type_opt->guisection = _("Query");

	field_opt = G_define_standard_option(G_OPT_V_FIELD) ;
	field_opt->description= _("Layer number. If -1, all layers are displayed.");
	field_opt->guisection = _("Query");
 
	cat_opt = G_define_standard_option(G_OPT_V_CATS) ;
	cat_opt->guisection = _("Query");

	where_opt = G_define_standard_option(G_OPT_WHERE) ;
	where_opt->guisection = _("Query");

	/* Colors */
	color_opt = G_define_option() ;
	color_opt->key        = "color" ;
	color_opt->type       = TYPE_STRING ;
	color_opt->answer     = DEFAULT_FG_COLOR;
	color_opt->description= _("Line color");
	color_opt->guisection = _("Colors");
	color_opt->gisprompt  = GISPROMPT_COLOR;

	fcolor_opt = G_define_option() ;
	fcolor_opt->key        = "fcolor" ;
	fcolor_opt->type       = TYPE_STRING ;
	fcolor_opt->answer     = "200:200:200" ;
	fcolor_opt->description= _("Area fill color");
	fcolor_opt->guisection = _("Colors");
	fcolor_opt->gisprompt  = GISPROMPT_COLOR;

	rgbcol_opt = G_define_option();
	rgbcol_opt->key        = "rgb_column";
	rgbcol_opt->type       = TYPE_STRING ;
	rgbcol_opt->required   = NO ;
	rgbcol_opt->multiple   = NO ;
	rgbcol_opt->guisection = _("Colors");
	rgbcol_opt->description=
	    _("Name of color definition column (for use with -a flag)");
	rgbcol_opt->answer     = "GRASSRGB" ;

	zcol_opt = G_define_option();
	zcol_opt->key          = "zcolor";
	zcol_opt->key_desc     = "style";
	zcol_opt->type         = TYPE_STRING;
	zcol_opt->required     = NO;
	zcol_opt->description  = _("Type of color table (for use with -z flag)");
	zcol_opt->answer       = "terrain";
	zcol_opt->guisection   = _("Colors");

	/* Lines */
	width_opt = G_define_option() ;
	width_opt->key        = "width";
	width_opt->type       = TYPE_INTEGER ;
	width_opt->answer     = "0" ;
	width_opt->guisection = _("Lines");
	width_opt->description= _("Line width");

	wcolumn_opt = G_define_option() ;
	wcolumn_opt->key        = "wcolumn" ;
	wcolumn_opt->type       = TYPE_STRING ;
	wcolumn_opt->guisection = _("Lines");
	wcolumn_opt->description= _("Name of column for line widths (these values will be scaled by wscale)");

	wscale_opt = G_define_option() ;
	wscale_opt->key        = "wscale" ;
	wscale_opt->type       = TYPE_DOUBLE ;
	wscale_opt->answer     = "1" ;
	wscale_opt->guisection = _("Lines");
	wscale_opt->description= _("Scale factor for wcolumn");

	/* Symbols */
	icon_opt = G_define_option() ;
	icon_opt->key        = "icon" ;
	icon_opt->type       = TYPE_STRING ;
	icon_opt->required   = NO ;
	icon_opt->multiple   = NO ;
	icon_opt->guisection = _("Symbols");
	icon_opt->answer     = "basic/x" ;
	/* This could also use ->gisprompt = "old,symbol,symbol" instead of ->options */
	icon_opt->options    = icon_files();
	icon_opt->description= _("Point and centroid symbol");

	size_opt = G_define_option() ;
	size_opt->key        = "size" ;
	size_opt->type       = TYPE_INTEGER ;
	size_opt->answer     = "5" ;
	size_opt->guisection = _("Symbols");
	size_opt->description= _("Symbol size");

	/* Labels */
	attrcol_opt = G_define_option() ;
	attrcol_opt->key        = "attrcol" ;
	attrcol_opt->type       = TYPE_STRING ;
	attrcol_opt->required   = NO ;
	attrcol_opt->multiple   = NO ; /* or fix attr.c, around line 102 */
	attrcol_opt->guisection = _("Labels");
	attrcol_opt->description= _("Name of column to be displayed");

	lfield_opt = G_define_option();
	lfield_opt->key        = "llayer";
	lfield_opt->type       = TYPE_INTEGER;
	lfield_opt->guisection = _("Labels");
	lfield_opt->description= _("Layer for labels (default: the given layer number)");

	lcolor_opt = G_define_option() ;
	lcolor_opt->key        = "lcolor" ;
	lcolor_opt->type       = TYPE_STRING ;
	lcolor_opt->answer     = "red" ;
	lcolor_opt->description= _("Label color");
	lcolor_opt->guisection = _("Labels");
	lcolor_opt->gisprompt  = GISPROMPT_COLOR;

	bgcolor_opt = G_define_option() ;
	bgcolor_opt->key        = "bgcolor" ;
	bgcolor_opt->type       = TYPE_STRING ;
	bgcolor_opt->answer     = "none" ;
	bgcolor_opt->guisection = _("Labels");
	bgcolor_opt->description= _("Label background color");
	bgcolor_opt->gisprompt  = GISPROMPT_COLOR;

	bcolor_opt = G_define_option() ;
	bcolor_opt->key        = "bcolor" ;
	bcolor_opt->type       = TYPE_STRING ;
	bcolor_opt->answer     = "none" ;
	bcolor_opt->guisection = _("Labels");
	bcolor_opt->description= _("Label border color");
	bcolor_opt->gisprompt  = GISPROMPT_COLOR;

	lsize_opt = G_define_option() ;
	lsize_opt->key        = "lsize" ;
	lsize_opt->type       = TYPE_INTEGER ;
	lsize_opt->answer     = "8" ;
	lsize_opt->guisection = _("Labels");
	lsize_opt->description= _("Label size (pixels)");

	font_opt = G_define_option() ;
	font_opt->key        = "font" ;
	font_opt->type       = TYPE_STRING ;
	font_opt->guisection = _("Labels");
	font_opt->description= _("Font name");

	enc_opt = G_define_option() ;
	enc_opt->key        = "encoding" ;
	enc_opt->type       = TYPE_STRING ;
	enc_opt->guisection = _("Labels");
	enc_opt->description= _("Text encoding");

	xref_opt = G_define_option() ;
	xref_opt->key        = "xref" ;
	xref_opt->type       = TYPE_STRING ;
	xref_opt->guisection = _("Labels");
	xref_opt->answer     = "left" ;
	xref_opt->options    = "left,center,right";
	xref_opt->description= _("Label horizontal justification");

	yref_opt = G_define_option() ;
	yref_opt->key        = "yref" ;
	yref_opt->type       = TYPE_STRING ;
	yref_opt->guisection = _("Labels");
	yref_opt->answer     = "center" ;
	yref_opt->options    = "top,center,bottom";
	yref_opt->description= _("Label vertical justification");
	
	minreg_opt = G_define_option() ;
	minreg_opt->key        = "minreg" ;
	minreg_opt->type       = TYPE_DOUBLE ;
	minreg_opt->required   = NO ;
	minreg_opt->description= _("Minimum region size (average from height and width) "
	                         "when map is displayed");

	maxreg_opt = G_define_option() ;
	maxreg_opt->key        = "maxreg" ;
	maxreg_opt->type       = TYPE_DOUBLE ;
	maxreg_opt->required   = NO ;
	maxreg_opt->description=
	    _("Maximum region size (average from height and width) "
	      "when map is displayed");

	render_opt = G_define_option() ;
	render_opt->key        = "render" ;
	render_opt->type       = TYPE_STRING ;
	render_opt->required   = NO;
	render_opt->multiple   = NO;
	render_opt->answer     = "c" ;
	render_opt->options    = "g,r,d,c,l";
	render_opt->description= _("Rendering method for filled polygons");
	render_opt->descriptions= _(
		"g;use the libgis render functions (features: clipping);"
		"r;use the raster graphics library functions (features: polylines);"
		"d;use the display library basic functions (features: polylines);"
		"c;use the display library clipping functions (features: clipping);"
		"l;use the display library culling functions (features: culling, polylines)"
		);

	/* please remove before GRASS 7 released */
	verbose_flag = G_define_flag ();
	verbose_flag->key		= 'v';
	verbose_flag->description	= _("Run verbosely");

	/* Colors */
	table_acolors_flag = G_define_flag ();
	table_acolors_flag->key		= 'a';
	table_acolors_flag->guisection  = _("Colors");
	table_acolors_flag->description	=
	    _("Get colors from map table column (of form RRR:GGG:BBB)");

	cats_acolors_flag = G_define_flag ();
	cats_acolors_flag->key		= 'c';
	cats_acolors_flag->guisection   = _("Colors");
	cats_acolors_flag->description	=
	    _("Random colors according to category number "
	      "(or layer number if 'layer=-1' is given)");

	/* Query */
	id_flag = G_define_flag ();
	id_flag->key		= 'i';
	id_flag->guisection     = _("Query");
	id_flag->description	= _("Use values from 'cats' option as feature id");

	x_flag = G_define_flag ();
	x_flag->key		= 'x';
	x_flag->description	=
	    _("Don't add to list of vectors and commands in monitor "
	      "(it won't be drawn if the monitor is refreshed)");

	zcol_flag = G_define_flag ();
	zcol_flag->key		= 'z';
	zcol_flag->description	=
	    _("Colorize polygons according to z height)");

	/* Check command line */
	if (G_parser(argc, argv))
	    exit(EXIT_FAILURE);

	if (G_strcasecmp(render_opt->answer, "g") == 0)
		render = RENDER_GPP;
	else if (G_strcasecmp(render_opt->answer, "r") == 0)
		render = RENDER_RPA;
	else if (G_strcasecmp(render_opt->answer, "d") == 0)
		render = RENDER_DP;
	else if (G_strcasecmp(render_opt->answer, "c") == 0)
		render = RENDER_DPC;
	else if (G_strcasecmp(render_opt->answer, "l") == 0)
		render = RENDER_DPL;
	else
		render = RENDER_GPP;

	/* please remove -v flag before GRASS 7 released */
	if (verbose_flag->answer) {
	    G_putenv ("GRASS_VERBOSE","3");
	    G_warning(_("The '-v' flag is superseded and will be removed "
			"in future. Please use '--verbose' instead."));
	}
	/* but keep this */
	if(G_verbose() > G_verbose_std())
	    verbose=TRUE;

	G_get_set_window (&window);

	if (R_open_driver() != 0)
	    G_fatal_error(_("No graphics device selected"));

	/* Read map options */

	/* Check min/max region */
	reg = ((window.east - window.west) + (window.north - window.south)) / 2;
	if ( minreg_opt->answer ) {
	    minreg = atof ( minreg_opt->answer );
             
	    if ( reg < minreg ) {
		G_message(_("Region size is lower than minreg, nothing displayed."));
	        D_add_to_list(G_recreate_command()) ;
		exit(EXIT_SUCCESS);
	    }
	}
	if ( maxreg_opt->answer ) {
	    maxreg = atof ( maxreg_opt->answer );
             
	    if ( reg > maxreg ) {
		G_message(_("Region size is greater than maxreg, nothing displayed."));
	        D_add_to_list(G_recreate_command()) ;
		exit(EXIT_SUCCESS);
	    }
	}

	G_strcpy(map_name, map_opt->answer);

	default_width = atoi(width_opt->answer);
	if( default_width < 0 )
		default_width = 0;
	width_scale = atof(wscale_opt->answer);

	if (table_acolors_flag -> answer && cats_acolors_flag -> answer)
 	{
 	    cats_acolors_flag -> answer = '\0';
 	    G_warning (_("The '-c' and '-a' flags cannot be used together, "
			 "the '-c' flag will be ignored!"));
 	}

	color = G_standard_color_rgb(WHITE);
	ret =  G_str_to_color(color_opt->answer, &r, &g, &b);
	if ( ret == 1 ) {
	    has_color = 1;
	    color.r = r;
	    color.g = g;
	    color.b = b;
	} else if ( ret == 2 ) { /* none */
	    has_color = 0;
	} else if ( ret == 0 ) { /* error */
	    G_fatal_error(_("Unknown color: [%s]"), color_opt->answer);
	}
	
	fcolor = G_standard_color_rgb(WHITE);
	ret = G_str_to_color(fcolor_opt->answer, &r, &g, &b);
        if ( ret == 1 ) {
	    has_fcolor = 1;
	    fcolor.r = r;
	    fcolor.g = g;
	    fcolor.b = b;
	} else if ( ret == 2 ) { /* none */
	    has_fcolor = 0;
	} else if ( ret == 0 ) { /* error */
	    G_fatal_error(_("Unknown color: [%s]"), fcolor_opt->answer);
	}

	size = atoi (size_opt->answer);
	Symb = S_read ( icon_opt->answer );
        if ( Symb == NULL ) G_warning(_("Cannot read symbol, cannot display points"));
	else S_stroke ( Symb, size, 0.0, 0 );

	/* Make sure map is available */
	mapset = G_find_vector2 (map_name, "") ; 
	
	if (mapset == NULL)
		G_fatal_error(_("Vector map <%s> not found"), map_name) ;

	/* if where_opt was specified select categories from db 
	 * otherwise parse cat_opt */
        Clist = Vect_new_cat_list ();
	Clist->field = atoi (field_opt->answer);
	
        /* open vector */
        level = Vect_open_old (&Map, map_name, mapset);
	
	if (where_opt->answer)
	  {
	    if ( Clist->field < 1 )
		    G_fatal_error(_("'layer' must be > 0 for 'where'."));
            chcat = 1;  
            if ( (fi = Vect_get_field ( &Map, Clist->field)) == NULL)
                G_fatal_error(_("Database connection not defined"));
            if ( fi != NULL )
	      {
                driver = db_start_driver(fi->driver);
		if (driver == NULL)
		    G_fatal_error(_("Unable to start driver <%s>"), fi->driver) ;
		
 	        db_init_handle (&handle);
	        db_set_handle (&handle, fi->database, NULL);
	        if (db_open_database(driver, &handle) != DB_OK)
		    G_fatal_error(_("Unable to open database <%s>"), fi->database) ;
		
		ncat = db_select_int( driver, fi->table, fi->key, where_opt->answer, &cats);

		db_close_database(driver);
		db_shutdown_driver(driver);
			
	        Vect_array_to_cat_list ( cats, ncat, Clist);
	      }
	  }
        else
	if (cat_opt->answer)
	  {
	    if ( Clist->field < 1 )
		G_fatal_error(_("'layer' must be > 0 for 'cats'."));
	    chcat = 1;  
	    ret = Vect_str_to_cat_list ( cat_opt->answer, Clist);
	    if ( ret > 0 )
	        G_warning (_("%d errors in cat option"), ret);
          }

	i = 0;
        type = 0; area = FALSE;
	while (type_opt->answers[i])
	  {
	    switch ( type_opt->answers[i][0] )
	      {
	        case 'p':
	            type |= GV_POINT;
		    break;
	        case 'l':
	            type |= GV_LINE;
		    break;
	        case 'b':
	            type |= GV_BOUNDARY;
		    break;
	        case 'f':
	            type |= GV_FACE;
		    break;
	        case 'c':
	            type |= GV_CENTROID;
		    break;
	        case 'a':
	            area = TRUE;
		    break;
	      }
	    i++;
	  }

	i = 0;
        display = 0;
	while (display_opt->answers[i])
	  {
	    switch ( display_opt->answers[i][0] )
	      {
	        case 's':
	            display |= DISP_SHAPE;
		    break;
	        case 'c':
	            display |= DISP_CAT;
		    break;
	        case 't':
	            display |= DISP_TOPO;
		    break;
	        case 'd':
	            display |= DISP_DIR;
		    break;
	        case 'a':
	            display |= DISP_ATTR;
		    break;
	        case 'z':
	            display |= DISP_ZCOOR;
		    break;
	      }
	    i++;
	  }
	
	/* Read label options */
	if (lfield_opt -> answer != NULL) 
	    lattr.field = atoi (lfield_opt->answer);
	else
	    lattr.field = Clist->field;

	lattr.color.R = lattr.color.G = lattr.color.B = 255;
	if ( G_str_to_color(lcolor_opt->answer, &r, &g, &b) ) {
	    lattr.color.R = r;
	    lattr.color.G = g;
	    lattr.color.B = b;
	}
	lattr.has_bgcolor = 0;
	if ( G_str_to_color(bgcolor_opt->answer, &r, &g, &b) == 1 ) {
	    lattr.has_bgcolor = 1;
	    lattr.bgcolor.R = r;
	    lattr.bgcolor.G = g;
	    lattr.bgcolor.B = b;
	}
	lattr.has_bcolor = 0;
	if ( G_str_to_color(bcolor_opt->answer, &r, &g, &b) == 1 ) {
	    lattr.has_bcolor = 1;
	    lattr.bcolor.R = r;
	    lattr.bcolor.G = g;
	    lattr.bcolor.B = b;
	}

	lattr.size = atoi(lsize_opt->answer);
	lattr.font = font_opt->answer;
	lattr.enc = enc_opt->answer;
	switch ( xref_opt->answer[0] )
	  {
	    case 'l':
	        lattr.xref = LLEFT;
		break;
	    case 'c':
	        lattr.xref = LCENTER;
		break;
	    case 'r':
	        lattr.xref = LRIGHT;
		break;
	  }
	switch ( yref_opt->answer[0] )
	  {
	    case 't':
	        lattr.yref = LTOP;
		break;
	    case 'c':
	        lattr.yref = LCENTER;
		break;
	    case 'b':
	        lattr.yref = LBOTTOM;
		break;
	  }

	D_setup(0);

        G_setup_plot (D_get_d_north(), D_get_d_south(), 
                      D_get_d_west(), D_get_d_east(),
                      D_move_abs, D_cont_abs);

	if(verbose)
	    G_message(_("Plotting ..."));

	if ( level >= 2 )
	    Vect_get_map_box ( &Map, &box );

	if ( level >= 2 && ( window.north < box.S || window.south >  box.N || 
		             window.east < box.W ||
			     window.west > G_adjust_easting(box.E, &window) ) )
	{
	    G_message(_("The bounding box of the map is outside the current region, "
			"nothing drawn."));
	    stat = 0;
	} else { 
	    overlap =  G_window_percentage_overlap(&window, box.N, box.S, box.E, box.W);
	    G_debug ( 1, "overlap = %f \n", overlap );
	    if ( overlap < 1 ) 
		Vect_set_constraint_region (&Map, window.north, window.south, 
		    window.east, window.west, PORT_DOUBLE_MAX, -PORT_DOUBLE_MAX);

	    /* default line width */
	    if ( !wcolumn_opt->answer )
	        D_line_width(default_width);

	    if ( area ) {
		if ( level >= 2) {
		    if ( display & DISP_SHAPE ) {
			stat = darea ( &Map, Clist,
				       has_color ? &color : NULL, has_fcolor ? &fcolor : NULL, chcat,
				       (int) id_flag->answer, table_acolors_flag->answer,
				       cats_acolors_flag->answer, &window, rgbcol_opt->answer,
				       default_width, wcolumn_opt->answer, width_scale, 
				       zcol_flag->answer, zcol_opt->answer );
		    }
	            if ( wcolumn_opt->answer )
	                D_line_width(default_width);
		} else
		    G_warning(_("Cannot display areas, topology not available"));
	    }

	    if ( display & DISP_SHAPE ) {
		if ( id_flag->answer && level < 2 ) {
		    G_warning(_("Cannot display lines by id, topology not available"));
		} else {
		    stat = plot1 ( &Map, type, area, Clist,
			has_color ? &color : NULL, has_fcolor ? &fcolor : NULL, chcat, Symb,
			size, (int) id_flag->answer, table_acolors_flag->answer,
			cats_acolors_flag->answer, rgbcol_opt->answer, default_width,
			wcolumn_opt->answer, width_scale, zcol_flag->answer, zcol_opt->answer) ;
	            if ( wcolumn_opt->answer )
	                D_line_width(default_width);
		}
	    }

	    if ( has_color ) {
		R_RGB_color(color.r, color.g, color.b);
		if ( display & DISP_DIR )
		    stat = dir ( &Map, type, Clist, chcat );
	    }

	    /* reset line width: Do we need to get line width from display
	     * driver (not implemented)?  It will help restore previous line
	     * width (not just 0) determined by another module (e.g.,
	     * d.linewidth). */
	    if ( !wcolumn_opt->answer )
	        R_line_width(0);

	    if ( display & DISP_CAT )
		stat = label ( &Map, type, area, Clist, &lattr, chcat);
	    
	    if ( display & DISP_ATTR )
		stat = attr ( &Map, type, attrcol_opt->answer, Clist, &lattr, chcat);

	    if ( display & DISP_ZCOOR )
		stat = zcoor ( &Map, type, &lattr);
	    
	    if ( display & DISP_TOPO ) {
		if (level >= 2 )
		    stat = topo ( &Map, type, area, &lattr);
		else
		    G_warning(_("Cannot display topology, not available"));
	    }
	}
	
	if ( !x_flag->answer ) {
	    D_add_to_list(G_recreate_command()) ;

	    D_set_dig_name(G_fully_qualified_name(map_name, mapset));
	    D_add_to_dig_list(G_fully_qualified_name(map_name, mapset));
	}

	R_close_driver();

	if(verbose)
	    G_done_msg (" ");
	
	Vect_close (&Map);
	Vect_destroy_cat_list (Clist);

	exit(stat);
}
