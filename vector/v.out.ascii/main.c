/*
 ****************************************************************************
 *
 * MODULE:     v.out.ascii
 * AUTHOR(S):  Michael Higgins, U.S. Army Construction Engineering Research Laboratory
 *             James Westervelt, U.S. Army Construction Engineering Research Laboratory
 *             Radim Blazek, ITC-Irst, Trento, Italy
 *             Martin Landa, CTU in Prague, Czech Republic (v.out.ascii.db merged & update (OGR) for GRASS7)
 *
 * PURPOSE:    Writes GRASS vector data as ASCII files
 * COPYRIGHT:  (C) 2000-2009, 2011-2012 by the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2). Read the file COPYING that comes
 *             with GRASS for details.
 *
 ****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Map_info Map;

    FILE *ascii, *att;
    char *input, *output, *delim, **columns, *where, *field_name, *cats;
    int format, dp, field, ret, region, old_format, header, type;
    int ver, pnt;

    struct cat_list *clist;
    
    clist = NULL;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword("ASCII");
    module->label =
	_("Exports a vector map to a GRASS ASCII vector representation.");
    module->description = _("By default only features with category are exported. "
                            "To export all features use 'layer=-1'.");

    parse_args(argc, argv, &input, &output, &format, &dp, &delim,
	       &field_name, &columns, &where, &region, &old_format, &header,
	       &cats, &type);
    
    if (format == GV_ASCII_FORMAT_STD && columns) {
      G_warning(_("Parameter '%s' ignored in standard mode"), "column");
    }

    ver = 5;
    pnt = 0;
    if (old_format)
	ver = 4;
    
    if (ver == 4 && format == GV_ASCII_FORMAT_POINT) {
      G_fatal_error(_("Format '%s' is not supported for old version"), "point");
    }
    
    if (ver == 4 && strcmp(output, "-") == 0) {
        G_fatal_error(_("Parameter '%s' must be given for old version"), "output");
    }

    /* open with topology only if needed */
    if (format == GV_ASCII_FORMAT_WKT ||
        (format == GV_ASCII_FORMAT_STD && (where || clist))) {
	if (Vect_open_old2(&Map, input, "", field_name) < 2) /* topology required for areas */
	    G_warning(_("Unable to open vector map <%s> at topology level. "
			"Areas will not be processed."),
		      input);
    }
    else {
	Vect_set_open_level(1); /* topology not needed */ 
	if (Vect_open_old2(&Map, input, "", field_name) < 0) 
	    G_fatal_error(_("Unable to open vector map <%s>"), input); 
        if (Vect_maptype(&Map) != GV_FORMAT_NATIVE) {
            /* require topological level for external formats
               centroids are read from topo */
            Vect_close(&Map);
            Vect_set_open_level(2);
            if (Vect_open_old2(&Map, input, "", field_name) < 0) 
                G_fatal_error(_("Unable to open vector map <%s>"), input); 
        }
    }

    field = Vect_get_field_number(&Map, field_name);
    if (cats) {
        clist = Vect_new_cat_list();
        
        clist->field = field;
        if (clist->field < 1)
            G_fatal_error(_("Layer <%s> not found"), field_name);
        ret = Vect_str_to_cat_list(cats, clist);
        if (ret > 0)
            G_fatal_error(_("%d errors in <%s> option"), ret, "cats");
    }

    if (strcmp(output, "-") != 0) {
	if (ver == 4) {
	    ascii = G_fopen_new("dig_ascii", output);
	}
	else if (strcmp(output, "-") == 0) {
	    ascii = stdout;
	}
	else {
	    ascii = fopen(output, "w");
	}

	if (ascii == NULL) {
	    G_fatal_error(_("Unable to open file <%s>"), output);
	}
    }
    else {
	ascii = stdout;
    }

    if (format == GV_ASCII_FORMAT_STD) {
	Vect_write_ascii_head(ascii, &Map);
	fprintf(ascii, "VERTI:\n");
    }

    /* Open dig_att */
    att = NULL;
    if (ver == 4 && !pnt) {
	if (G_find_file("dig_att", output, G_mapset()) != NULL)
	    G_fatal_error(_("dig_att file already exist"));

	if ((att = G_fopen_new("dig_att", output)) == NULL)
	    G_fatal_error(_("Unable to open dig_att file <%s>"),
			  output);
    }

    if (where || columns || clist)
	G_message(_("Fetching data..."));
    ret = Vect_write_ascii(ascii, att, &Map, ver, format, dp, delim,
			   region, type, field, clist, (const char *)where,
			   (const char **)columns, header);

    if (ret < 1) {
	if (format == GV_ASCII_FORMAT_POINT) {
	    G_warning(_("No points found, nothing to be exported"));
	}
	else {
	    G_warning(_("No features found, nothing to be exported"));
	}
    }
    
    if (ascii != NULL)
	fclose(ascii);
    if (att != NULL)
	fclose(att);

    Vect_close(&Map);

    if (cats)
        Vect_destroy_cat_list(clist);
    
    exit(EXIT_SUCCESS);
}
