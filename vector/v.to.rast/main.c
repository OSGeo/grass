/****************************************************************************
 *
 * MODULE:       v.to.rast
 * AUTHOR(S):    Original code: Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
 *               Stream directions: Jaro Hofierka and Helena Mitasova
 *               Radim Blazek <radim.blazek gmail.com> (GRASS 6 update)
 *               Brad Douglas <rez touchofmadness.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_nospam yahoo.com>, Markus Neteler <neteler itc.it>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2003-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h> 
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "local.h"

int main (int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input, *output, *rows, *col, *field_opt, *use_opt, *val_opt,
      *rgbcol_opt, *label_opt, *type_opt;
    int    field, nrows, use, value_type, type;
    double value;

    G_gisinit (argv[0]);

    module = G_define_module();
    module->keywords = _("vector, raster, conversion");
    module->description = _("Converts a binary GRASS vector map layer "
		          "into a GRASS raster map layer.");

    input  = G_define_standard_option(G_OPT_V_INPUT);
    output = G_define_standard_option(G_OPT_R_OUTPUT);

    use_opt = G_define_option();
    use_opt->key            = "use";
    use_opt->type           = TYPE_STRING;
    use_opt->required       = NO;
    use_opt->multiple       = NO;
    use_opt->options        = "attr,cat,val,z,dir";
    use_opt->answer         = "attr";
    use_opt->description    = _("Source of raster values");
    use_opt->descriptions   = _("attr;read values from attribute table;"
				"cat;use category values;"
				"val;use value specified by value option;"
				"z;use z coordinate (points or contours only);"
				"dir;output as flow direction (lines only)");

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt -> options = "point,line,area";
    type_opt -> answer = "point,line,area";

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    /* for GRASS 7, IMHO this should be changed to "attrcolumn" */
    col = G_define_standard_option(G_OPT_COLUMN);
    col->description    = _("Name of column for attr parameter (data type must be numeric)");
    col->guisection = _("Attributes");

    val_opt = G_define_option();
    val_opt->key              = "value";
    val_opt->type             = TYPE_DOUBLE;
    val_opt->required         = NO;
    val_opt->multiple         = NO;
    val_opt->answer           = "1";
    val_opt->description      = _("Raster value");

    rows = G_define_option();
    rows->key              = "rows";
    rows->type             = TYPE_INTEGER;
    rows->required         = NO;
    rows->multiple         = NO;
    rows->answer           = "4096";
    rows->description      = _("Number of rows to hold in memory");

    rgbcol_opt = G_define_standard_option(G_OPT_COLUMN);
    rgbcol_opt->key        = "rgbcolumn";
    rgbcol_opt->description= _("Name of color definition column (with RRR:GGG:BBB entries)");
    rgbcol_opt->guisection = _("Attributes");

    label_opt = G_define_standard_option(G_OPT_COLUMN);
    label_opt->key        = "labelcolumn";
    label_opt->description= _("Name of column used as raster category labels");
    label_opt->guisection = _("Attributes");

    if (G_parser (argc, argv))
	exit(EXIT_FAILURE);

    type  = Vect_option_to_types (type_opt);
    field = atoi (field_opt->answer);
    nrows = atoi (rows->answer);

    switch (use_opt->answer[0])
    {
    case 'a':
        use = USE_ATTR;
        if (!col->answer)
    	    G_fatal_error (_("Column parameter missing (or use value parameter)"));
    break;
    case 'c':
        use = USE_CAT;
        if (col->answer)
    	    G_fatal_error (_("Column parameter cannot be combined with use of category values option"));
    break;
    case 'v':
        use = USE_VAL;
        if (col->answer || label_opt->answer || rgbcol_opt->answer)
    	    G_fatal_error (_("Column parameter cannot be combined with use of value option"));
    break;
    case 'z':
        use = USE_Z;
        if (col->answer || label_opt->answer || rgbcol_opt->answer)
    	    G_fatal_error (_("Column parameter cannot be combined with use of z coordinate"));
    break;
    case 'd':
        use = USE_D;
    break;
    default:
        G_fatal_error (_("Unknown option '%s'"), use_opt->answer);
    break;
    }

    value = atof ( val_opt->answer );
    value_type = (strchr (val_opt->answer, '.')) ? USE_DCELL : USE_CELL;

    return vect_to_rast (input->answer, output->answer, field,
			 col->answer, nrows, use, value, value_type,
			 rgbcol_opt->answer, label_opt->answer, type);
}
