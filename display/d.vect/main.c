/*
 ****************************************************************************
 *
 * MODULE:       d.vect
 * AUTHOR(S):    CERL, Radim Blazek, others
 *               Updated to GRASS7 by Martin Landa <landa.martin gmail.com>
 *               Support for vector legend by Adam Laza <ad.laza32 gmail.com >
 * PURPOSE:      Display the vector map in map display
 * COPYRIGHT:    (C) 2004-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "plot.h"
#include "local_proto.h"

static int cmp(const void *, const void *);
static char *icon_files(void);

int main(int argc, char **argv)
{
    int ret, level;
    int stat, type, display;
    int chcat;
    int has_color, has_fcolor;
    struct color_rgb color, fcolor;
    double size;
    int default_width;
    double width_scale;
    double minreg, maxreg, reg;
    char map_name[GNAME_MAX];
    
    struct GModule *module;
    struct Option *map_opt;
    struct Option *color_opt, *fcolor_opt, *rgbcol_opt, *zcol_opt;
    struct Option *type_opt, *display_opt;
    struct Option *icon_opt, *size_opt, *sizecolumn_opt, *rotcolumn_opt;
    struct Option *where_opt;
    struct Option *field_opt, *cat_opt, *lfield_opt;
    struct Option *lcolor_opt, *bgcolor_opt, *bcolor_opt;
    struct Option *lsize_opt, *font_opt, *enc_opt, *xref_opt, *yref_opt;
    struct Option *attrcol_opt, *maxreg_opt, *minreg_opt;
    struct Option *width_opt, *wcolumn_opt, *wscale_opt;
    struct Option *leglab_opt;
    struct Option *icon_line_opt, *icon_area_opt;
    struct Flag *id_flag, *cats_acolors_flag, *sqrt_flag, *legend_flag;
    char *desc;
    
    struct cat_list *Clist;
    LATTR lattr;
    struct Map_info Map;
    struct Cell_head window;
    struct bound_box box;
    double overlap;

    stat = 0;
    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("vector"));
    G_add_keyword(_("level1"));
    module->description = _("Displays user-specified vector map "
			    "in the active graphics frame.");
    
    map_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field_opt->answer = "1";
    field_opt->guisection = _("Selection");

    display_opt = G_define_option();
    display_opt->key = "display";
    display_opt->type = TYPE_STRING;
    display_opt->required = YES;
    display_opt->multiple = YES;
    display_opt->answer = "shape";
    display_opt->options = "shape,cat,topo,vert,dir,zcoor";
    display_opt->description = _("Display");
    desc = NULL;
    G_asprintf(&desc,
	       "shape;%s;cat;%s;topo;%s;vert;%s;dir;%s;zcoor;%s",
	       _("Display geometry of features"),
	       _("Display category numbers of features"),
	       _("Display topology information (nodes, edges)"),
               _("Display vertices of features"),
	       _("Display direction of linear features"),
	       _("Display z-coordinate of features (only for 3D vector maps)"));
    display_opt->descriptions = desc;
    
    /* Query */
    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->answer = "point,line,area,face";
    type_opt->options = "point,line,boundary,centroid,area,face";
    type_opt->guisection = _("Selection");
    
    cat_opt = G_define_standard_option(G_OPT_V_CATS);
    cat_opt->guisection = _("Selection");

    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");


    /* Colors */
    color_opt = G_define_standard_option(G_OPT_CN);
    color_opt->label = _("Feature color");
    color_opt->answer = "0:29:57";
    color_opt->guisection = _("Colors");
    
    fcolor_opt = G_define_standard_option(G_OPT_CN);
    fcolor_opt->key = "fill_color";
    fcolor_opt->answer = "0:103:204";
    fcolor_opt->label = _("Area fill color");
    fcolor_opt->guisection = _("Colors");

    rgbcol_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    rgbcol_opt->key = "rgb_column";
    rgbcol_opt->guisection = _("Colors");
    rgbcol_opt->label = _("Colorize features according to color definition column");
    rgbcol_opt->description = _("Color definition in R:G:B form");
    
    zcol_opt = G_define_standard_option(G_OPT_M_COLR);
    zcol_opt->key = "zcolor";
    zcol_opt->description = _("Colorize point or area features according to z-coordinate");
    zcol_opt->guisection = _("Colors");

    /* Lines */
    width_opt = G_define_option();
    width_opt->key = "width";
    width_opt->type = TYPE_INTEGER;
    width_opt->answer = "0";
    width_opt->guisection = _("Lines");
    width_opt->description = _("Line width");

    wcolumn_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    wcolumn_opt->key = "width_column";
    wcolumn_opt->guisection = _("Lines");
    wcolumn_opt->label = _("Name of numeric column containing line width");
    wcolumn_opt->description = _("These values will be scaled by width_scale");

    wscale_opt = G_define_option();
    wscale_opt->key = "width_scale";
    wscale_opt->type = TYPE_DOUBLE;
    wscale_opt->answer = "1";
    wscale_opt->guisection = _("Lines");
    wscale_opt->description = _("Scale factor for width_column");

    /* Symbols */
    icon_opt = G_define_option();
    icon_opt->key = "icon";
    icon_opt->type = TYPE_STRING;
    icon_opt->required = NO;
    icon_opt->multiple = NO;
    icon_opt->guisection = _("Symbols");
    icon_opt->answer = "basic/x";
    /* This could also use ->gisprompt = "old,symbol,symbol" instead of ->options */
    icon_opt->options = icon_files();
    icon_opt->description = _("Point and centroid symbol");

    size_opt = G_define_option();
    size_opt->key = "size";
    size_opt->type = TYPE_DOUBLE;
    size_opt->answer = "5";
    size_opt->guisection = _("Symbols");
    size_opt->label = _("Symbol size");
    size_opt->description =
	_("When used with the size_column option this becomes the scale factor");

    sizecolumn_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    sizecolumn_opt->key = "size_column";
    sizecolumn_opt->guisection = _("Symbols");
    sizecolumn_opt->description =
	_("Name of numeric column containing symbol size");

    rotcolumn_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    rotcolumn_opt->key = "rotation_column";
    rotcolumn_opt->guisection = _("Symbols");
    rotcolumn_opt->label =
	_("Name of numeric column containing symbol rotation angle");
    rotcolumn_opt->description =
	_("Measured in degrees CCW from east");

    icon_area_opt = G_define_option();
    icon_area_opt->key = "icon_area";
    icon_area_opt->type = TYPE_STRING;
    icon_area_opt->required = NO;
    icon_area_opt->multiple = NO;
    icon_area_opt->guisection = _("Legend");
    icon_area_opt->answer = "legend/area";
    icon_area_opt->options = icon_files();
    icon_area_opt->description = _("Area/boundary symbol for legend");

    icon_line_opt = G_define_option();
    icon_line_opt->key = "icon_line";
    icon_line_opt->type = TYPE_STRING;
    icon_line_opt->required = NO;
    icon_line_opt->multiple = NO;
    icon_line_opt->guisection = _("Legend");
    icon_line_opt->answer = "legend/line";
    icon_line_opt->options = icon_files();
    icon_line_opt->description = _("Line symbol for legend");

    leglab_opt = G_define_option();
    leglab_opt->key = "legend_label";
    leglab_opt->type = TYPE_STRING;
    leglab_opt->guisection = _("Legend");
    leglab_opt->description = _("Label to display after symbol in vector legend");

    /* Labels */
    lfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    lfield_opt->key = "label_layer";
    lfield_opt->required = NO;
    lfield_opt->guisection = _("Labels");
    lfield_opt->label =
	_("Layer number for labels (default: the given layer number)");
    
    attrcol_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    attrcol_opt->key = "attribute_column";
    attrcol_opt->multiple = NO;	/* or fix attr.c, around line 102 */
    attrcol_opt->guisection = _("Labels");
    attrcol_opt->description = _("Name of column to be displayed as a label");

    lcolor_opt = G_define_standard_option(G_OPT_C);
    lcolor_opt->key = "label_color";
    lcolor_opt->answer = "red";
    lcolor_opt->label = _("Label color");
    lcolor_opt->guisection = _("Labels");

    bgcolor_opt = G_define_standard_option(G_OPT_CN);
    bgcolor_opt->key = "label_bgcolor";
    bgcolor_opt->answer = "none";
    bgcolor_opt->guisection = _("Labels");
    bgcolor_opt->label = _("Label background color");

    bcolor_opt = G_define_standard_option(G_OPT_CN);
    bcolor_opt->key = "label_bcolor";
    bcolor_opt->type = TYPE_STRING;
    bcolor_opt->answer = "none";
    bcolor_opt->guisection = _("Labels");
    bcolor_opt->label = _("Label border color");

    lsize_opt = G_define_option();
    lsize_opt->key = "label_size";
    lsize_opt->type = TYPE_INTEGER;
    lsize_opt->answer = "8";
    lsize_opt->guisection = _("Labels");
    lsize_opt->description = _("Label size (pixels)");

    font_opt = G_define_option();
    font_opt->key = "font";
    font_opt->type = TYPE_STRING;
    font_opt->guisection = _("Labels");
    font_opt->description = _("Font name");

    enc_opt = G_define_option();
    enc_opt->key = "encoding";
    enc_opt->type = TYPE_STRING;
    enc_opt->guisection = _("Labels");
    enc_opt->description = _("Text encoding");

    xref_opt = G_define_option();
    xref_opt->key = "xref";
    xref_opt->type = TYPE_STRING;
    xref_opt->guisection = _("Labels");
    xref_opt->answer = "left";
    xref_opt->options = "left,center,right";
    xref_opt->description = _("Label horizontal justification");

    yref_opt = G_define_option();
    yref_opt->key = "yref";
    yref_opt->type = TYPE_STRING;
    yref_opt->guisection = _("Labels");
    yref_opt->answer = "center";
    yref_opt->options = "top,center,bottom";
    yref_opt->description = _("Label vertical justification");

    minreg_opt = G_define_option();
    minreg_opt->key = "minreg";
    minreg_opt->type = TYPE_DOUBLE;
    minreg_opt->required = NO;
    minreg_opt->description =
	_("Minimum region size (average from height and width) "
	  "when map is displayed");

    maxreg_opt = G_define_option();
    maxreg_opt->key = "maxreg";
    maxreg_opt->type = TYPE_DOUBLE;
    maxreg_opt->required = NO;
    maxreg_opt->description =
	_("Maximum region size (average from height and width) "
	  "when map is displayed");

    /* Colors */
    cats_acolors_flag = G_define_flag();
    cats_acolors_flag->key = 'c';
    cats_acolors_flag->guisection = _("Colors");
    cats_acolors_flag->description =
	_("Random colors according to category number "
	  "(or layer number if 'layer=-1' is given)");

    /* Query */
    id_flag = G_define_flag();
    id_flag->key = 'i';
    id_flag->guisection = _("Selection");
    id_flag->description = _("Use values from 'cats' option as feature id");

    sqrt_flag = G_define_flag();
    sqrt_flag->key = 'r';
    sqrt_flag->label = _("Use square root of the value of size_column");
    sqrt_flag->description =
	_("This makes circle areas proportionate to the size_column values "
	  "instead of circle radius");
    sqrt_flag->guisection = _("Symbols");

    legend_flag = G_define_flag();
    legend_flag->key = 's';
    legend_flag->label = _("Do not show this layer in vector legend");
    legend_flag->guisection = _("Legend");

    G_option_exclusive(zcol_opt, rgbcol_opt, cats_acolors_flag, NULL);
    
    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    D_open_driver();
    
    G_get_set_window(&window);
    
    /* Check min/max region */
    reg = ((window.east - window.west) + (window.north - window.south)) / 2;
    if (minreg_opt->answer) {
	minreg = atof(minreg_opt->answer);

	if (reg < minreg) {
	    G_important_message(_("Region size is lower than minreg, nothing displayed"));
	    exit(EXIT_SUCCESS);
	}
    }
    if (maxreg_opt->answer) {
	maxreg = atof(maxreg_opt->answer);

	if (reg > maxreg) {
	    G_important_message(_("Region size is greater than maxreg, nothing displayed"));
	    exit(EXIT_SUCCESS);
	}
    }

    strcpy(map_name, map_opt->answer);

    default_width = atoi(width_opt->answer);
    if (default_width < 0)
	default_width = 0;
    width_scale = atof(wscale_opt->answer);

    if (cats_acolors_flag->answer && rgbcol_opt->answer) {
	G_warning(_("The -%c flag and <%s> option cannot be used together, "
		    "the -%c flag will be ignored!"), 
                  cats_acolors_flag->key, rgbcol_opt->key, cats_acolors_flag->key);
        cats_acolors_flag->answer = FALSE;
    }

    color = G_standard_color_rgb(WHITE);
    has_color = option_to_color(&color, color_opt->answer);
    fcolor = G_standard_color_rgb(WHITE);
    has_fcolor = option_to_color(&fcolor, fcolor_opt->answer);
    
    size = atof(size_opt->answer);

    /* if where_opt was specified select categories from db 
     * otherwise parse cat_opt */
    Clist = Vect_new_cat_list();
    Clist->field = atoi(field_opt->answer);

    /* open vector */
    level = Vect_open_old2(&Map, map_name, "", field_opt->answer);

    chcat = 0;
    if (where_opt->answer) {
	if (Clist->field < 1)
	    G_fatal_error(_("Option <%s> must be > 0"), field_opt->key);
	chcat = 1;
	option_to_where(&Map, Clist, where_opt->answer);
    }
    else if (cat_opt->answer) {
	if (Clist->field < 1 && !id_flag->answer)
	    G_fatal_error(_("Option <%s> must be > 0"), field_opt->key);
	chcat = 1;
	ret = Vect_str_to_cat_list(cat_opt->answer, Clist);
	if (ret > 0)
	    G_warning(n_("%d error in cat option", "%d errors in cat option", ret), ret);
    }
    
    type = Vect_option_to_types(type_opt);
    
    display = option_to_display(display_opt);

    /* labels */
    options_to_lattr(&lattr, lfield_opt->answer,
		     lcolor_opt->answer, bgcolor_opt->answer, bcolor_opt->answer,
		     atoi(lsize_opt->answer), font_opt->answer, enc_opt->answer,
		     xref_opt->answer, yref_opt->answer);

    D_setup(0);
    D_set_reduction(1.0);

    G_verbose_message(_("Plotting..."));

    overlap = 1;
    if (level >= 2 && window.proj != PROJECTION_LL) {
	Vect_get_map_box(&Map, &box);
	overlap = G_window_percentage_overlap(&window, box.N, box.S,
					      box.E, box.W);
	G_debug(1, "overlap = %f \n", overlap);
    }

    if (overlap == 0) {
	G_warning(_("The bounding box of the map is outside the current region, "
		    "nothing drawn"));
    }
    else {
	if (overlap < 1)
	    Vect_set_constraint_region(&Map, window.north, window.south,
				       window.east, window.west,
				       PORT_DOUBLE_MAX, -PORT_DOUBLE_MAX);

	/* default line width */
	if (!wcolumn_opt->answer)
	    D_line_width(default_width);

	if (display & DISP_SHAPE) {
	    stat += display_shape(&Map, type, Clist, &window,
				  has_color ? &color : NULL, has_fcolor ? &fcolor : NULL, chcat,
				  icon_opt->answer, size, sizecolumn_opt->answer,
				  sqrt_flag->answer ? TRUE : FALSE, rotcolumn_opt->answer,
				  id_flag->answer ? TRUE : FALSE, 
				  cats_acolors_flag->answer ? TRUE : FALSE, rgbcol_opt->answer,
				  default_width,  wcolumn_opt->answer, width_scale,
				  zcol_opt->answer);
	    
	    if (wcolumn_opt->answer)
		D_line_width(default_width);
	}

	if (has_color) {
	    D_RGB_color(color.r, color.g, color.b);
	    if (display & DISP_DIR)
		stat += display_dir(&Map, type, Clist, chcat, size);
	}

	if (!legend_flag->answer) {
		write_into_legfile(&Map, type, leglab_opt->answer, map_name,
			   icon_opt->answer, size_opt->answer, 
			   color_opt->answer, fcolor_opt->answer, 
			   width_opt->answer, icon_area_opt->answer,
			   icon_line_opt->answer, sizecolumn_opt->answer);
	}

	/* reset line width: Do we need to get line width from display
	 * driver (not implemented)?  It will help restore previous line
	 * width (not just 0) determined by another module (e.g.,
	 * d.linewidth). */
	if (!wcolumn_opt->answer)
	    D_line_width(0);
	
	if (display & DISP_CAT)
	    stat += display_label(&Map, type, Clist, &lattr, chcat);

	if (attrcol_opt->answer)
	    stat += display_attr(&Map, type, attrcol_opt->answer, Clist, &lattr, chcat);

	if (display & DISP_ZCOOR)
	    stat += display_zcoor(&Map, type, &lattr);

	if (display & DISP_VERT)
            stat += display_vert(&Map, type, &lattr, size);

	if (display & DISP_TOPO)
            stat += display_topo(&Map, type, &lattr, size);
    }

    D_save_command(G_recreate_command());
    D_close_driver();

    Vect_close(&Map);
    Vect_destroy_cat_list(Clist);

    if (stat != 0) {
	G_fatal_error(_("Rendering failed"));
    }
    
    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}

int cmp(const void *a, const void *b) 
{
    return (strcmp(*(char **)a, *(char **)b));
}

/* adopted from r.colors */
char *icon_files(void)
{
    char **list, *ret;
    char buf[GNAME_MAX], path[GPATH_MAX], path_i[GPATH_MAX];
    int i, count;
    size_t len;
    DIR *dir, *dir_i;
    struct dirent *d, *d_i;

    list = NULL;
    len = 0;
    sprintf(path, "%s/etc/symbol", G_gisbase());

    dir = opendir(path);
    if (!dir)
	return NULL;

    count = 0;
    
    /* loop over etc/symbol */
    while ((d = readdir(dir))) {
	if (d->d_name[0] == '.')
	    continue;

	sprintf(path_i, "%s/etc/symbol/%s", G_gisbase(), d->d_name);
	dir_i = opendir(path_i);

	if (!dir_i)
	    continue;

	/* loop over each directory in etc/symbols */
	while ((d_i = readdir(dir_i))) {
	    if (d_i->d_name[0] == '.')
		continue;
	    
	    list = G_realloc(list, (count + 1) * sizeof(char *));
	    
	    sprintf(buf, "%s/%s", d->d_name, d_i->d_name);
	    list[count++] = G_store(buf);
	    
	    len += strlen(d->d_name) + strlen(d_i->d_name) + 2; /* '/' + ',' */
	}

	closedir(dir_i);
    }

    closedir(dir);

    qsort(list, count, sizeof(char *), cmp);
    
    if (len > 0) {
	ret = G_malloc((len + 1) * sizeof(char)); /* \0 */
	*ret = '\0';
	for (i = 0; i < count; i++) {
	    if (i > 0)
		strcat(ret, ",");
	    strcat(ret, list[i]);
	    G_free(list[i]);
	}
	G_free(list);
    }
    else {
	ret = G_store("");
    }
    
    return ret;
}

