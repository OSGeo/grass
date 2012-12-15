
/****************************************************************************
 *
 * MODULE:       v.to.rast
 * AUTHOR(S):    Original code: Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
 *               Stream directions: Jaro Hofierka and Helena Mitasova
 *               Radim Blazek <radim.blazek gmail.com> (GRASS 6 update)
 *               Brad Douglas <rez touchofmadness.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>, Markus Neteler <neteler itc.it>
 *               OGR support by Martin Landa <landa.martin gmail.com>
 * PURPOSE:      Converts vector map to raster map
 * COPYRIGHT:    (C) 2003-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input, *output, *rows, *col, *use_opt, *val_opt,
		  *field_opt, *type_opt, *where_opt, *cats_opt,
	          *rgbcol_opt, *label_opt;
    int nrows, use, value_type, type;
    double value;
    char *desc;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("conversion"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("rasterization"));
    module->description = _("Converts (rasterize) a vector map into a raster map.");

    input = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,line,area";
    type_opt->answer = "point,line,area";
    type_opt->guisection = _("Selection");
    
    cats_opt = G_define_standard_option(G_OPT_V_CATS);
    cats_opt->guisection = _("Selection");
    
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    
    use_opt = G_define_option();
    use_opt->key = "use";
    use_opt->type = TYPE_STRING;
    use_opt->required = YES;
    use_opt->multiple = NO;
    use_opt->options = "attr,cat,val,z,dir";
    use_opt->description = _("Source of raster values");
    desc = NULL;
    G_asprintf(&desc,
	       "attr;%s;cat;%s;val;%s;z;%s;dir;%s",
	       _("read values from attribute table"),
	       _("use category values"),
	       _("use value specified by value option"),
	       _("use z coordinate (points or contours only)"),
	       _("output as flow direction (lines only)"));
    use_opt->descriptions = desc;

    col = G_define_standard_option(G_OPT_DB_COLUMN);
    col->key = "attrcolumn";
    col->description =
	_("Name of column for 'attr' parameter (data type must be numeric)");
    col->guisection = _("Attributes");

    rgbcol_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    rgbcol_opt->key = "rgbcolumn";
    rgbcol_opt->description =
	_("Name of color definition column (with RRR:GGG:BBB entries)");
    rgbcol_opt->guisection = _("Attributes");

    label_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    label_opt->key = "labelcolumn";
    label_opt->description =
	_("Name of column used as raster category labels");
    label_opt->guisection = _("Attributes");

    val_opt = G_define_option();
    val_opt->key = "value";
    val_opt->type = TYPE_DOUBLE;
    val_opt->required = NO;
    val_opt->multiple = NO;
    val_opt->answer = "1";
    val_opt->description = _("Raster value (for use=val)");
    
    rows = G_define_option();
    rows->key = "rows";
    rows->type = TYPE_INTEGER;
    rows->required = NO;
    rows->multiple = NO;
    rows->answer = "4096";
    rows->description = _("Number of rows to hold in memory");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    type = Vect_option_to_types(type_opt);
    nrows = atoi(rows->answer);

    switch (use_opt->answer[0]) {
    case 'a':
	use = USE_ATTR;
	if (!col->answer)
	    G_fatal_error(_("Column parameter missing (or use value parameter)"));
	break;
    case 'c':
	use = USE_CAT;
	if (col->answer)
	    G_fatal_error(_("Column parameter cannot be combined with use of category values option"));
	break;
    case 'v':
	use = USE_VAL;
	if (col->answer || label_opt->answer || rgbcol_opt->answer)
	    G_fatal_error(_("Column parameter cannot be combined with use of value option"));
	break;
    case 'z':
	use = USE_Z;
	if (col->answer || label_opt->answer || rgbcol_opt->answer)
	    G_fatal_error(_("Column parameter cannot be combined with use of z coordinate"));
	break;
    case 'd':
	use = USE_D;
	break;
    default:
	G_fatal_error(_("Unknown option '%s'"), use_opt->answer);
	break;
    }

    value = atof(val_opt->answer);
    value_type = (strchr(val_opt->answer, '.')) ? USE_DCELL : USE_CELL;

    if (vect_to_rast(input->answer, output->answer, field_opt->answer,
		     col->answer, nrows, use, value, value_type,
		     rgbcol_opt->answer, label_opt->answer, type,
		     where_opt->answer, cats_opt->answer)) {
	exit(EXIT_FAILURE);
    }

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}
