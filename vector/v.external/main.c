
/****************************************************************
 *
 * MODULE:       v.external
 * 
 * AUTHOR(S):    Radim Blazek
 *               Updated to GRASS 7 by Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Create a new vector as a link to OGR layer
 *               
 * COPYRIGHT:    (C) 2003-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include <ogr_api.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct _options options;
    struct _flags   flags;

    struct Map_info Map;
    
    FILE *fd;
    
    int ilayer, is3D;
    char buf[GPATH_MAX], *dsn;
    const char *output;
    
    G_gisinit(argv[0]);
    
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("input"));
    G_add_keyword(_("external")); 
    
    module->description = _("Creates a new pseudo-vector map as a link to an OGR-supported layer "
			    "or a PostGIS feature table.");
    parse_args(argc, argv,
	       &options, &flags);
    
#ifdef HAVE_OGR
    if (!flags.postgis->answer)
	OGRRegisterAll();
#endif

    if (flags.format->answer) {
	/* list formats */
	if (flags.postgis->answer) {
	    G_fatal_error(_("Flags -%c and -%c are mutually exclusive"),
			  flags.format->key, flags.postgis->key);
	}
	list_formats(stdout);
	exit(EXIT_SUCCESS);
    }

    /* be friendly, ignored 'PG:' prefix for PostGIS links */
    if (flags.postgis->answer &&
	G_strncasecmp(options.dsn->answer, "PG:", 3) == 0) {
	int i, length;
	
	length = strlen(options.dsn->answer);
	dsn = (char *) G_malloc(length - 3);
	for (i = 3; i < length; i++)
	    dsn[i-3] = options.dsn->answer[i];
    }
    else {
	dsn = G_store(options.dsn->answer);
    }
    
    if (flags.list->answer || flags.tlist->answer) {
	/* list layers */
	if (!dsn)
	    G_fatal_error(_("Required parameter <%s> not set"), options.dsn->key);
	list_layers(stdout, dsn, NULL,
		    flags.tlist->answer ? TRUE : FALSE,
		    flags.postgis->answer,
		    NULL);
	exit(EXIT_SUCCESS);
    }

    /* define name for output */
    if (!options.output->answer)
	output = options.layer->answer;
    else
	output = options.output->answer;
    

    /* get layer index */
    ilayer = list_layers(NULL, dsn, options.layer->answer,
			 FALSE, flags.postgis->answer, &is3D);
    if (ilayer == -1) {
	G_fatal_error(_("Layer <%s> not available"), options.layer->answer);
    }
    
    G_debug(2, "layer '%s' was found", options.layer->answer);

    if (G_find_vector2(output, G_mapset()) && !G_check_overwrite(argc, argv)) {
	G_fatal_error(_("option <%s>: <%s> exists."),
		      options.output->key, output);
    }
    
    /* create new vector map */
    Vect_open_new(&Map, output, is3D);
    Vect_set_error_handler_io(NULL, &Map);
    
    Vect_hist_command(&Map);
    Vect_close(&Map);
    
    /* Vect_open_new created 'head', 'coor', 'hist'
       -> delete 'coor' and create 'frmt' */
    sprintf(buf, "%s/%s/%s/%s/coor", G_location_path(), G_mapset(),
	    GV_DIRECTORY, output);
    G_debug(2, "Delete '%s'", buf);
    if (unlink(buf) == -1) {
	Vect_delete(output);
	G_fatal_error(_("Unable to delete '%s'"), buf);
    }

    /* create frmt file */
    sprintf(buf, "%s/%s", GV_DIRECTORY, output);
    fd = G_fopen_new(buf, GV_FRMT_ELEMENT);
    if (fd == NULL) {
	Vect_delete(output);
	G_fatal_error("Unable to create file '%s'", buf);
    }
    
    if (flags.postgis->answer) {
	char *table_name, *schema_name;
	
	get_table_name(options.layer->answer, &table_name, &schema_name);
	
	fprintf(fd, "FORMAT: postgis\n");
	fprintf(fd, "CONNINFO: %s\n", dsn);
	if (schema_name)
	    fprintf(fd, "SCHEMA: %s\n", schema_name);
	fprintf(fd, "TABLE: %s\n", table_name);

	G_free(table_name);
	G_free(schema_name);
    }
    else {
	fprintf(fd, "FORMAT: ogr\n");
	fprintf(fd, "DSN: %s\n", dsn);
	fprintf(fd, "LAYER: %s\n", options.layer->answer);
    }
    fclose(fd);
    
    if (!flags.topo->answer) {
	Vect_open_old(&Map, output, G_mapset());
	Vect_build(&Map);
	Vect_close(&Map);
    }
    
    G_done_msg(_("Link to vector map <%s> created."), output);
    
    exit(EXIT_SUCCESS);
}
