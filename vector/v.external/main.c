
/****************************************************************
 *
 * MODULE:       v.external
 * 
 * AUTHOR(S):    Radim Blazek
 *               Updated by Martin Landa <landa.martin gmail.com> (2009)
 *               
 * PURPOSE:      Create a new vector as a link to OGR layer (read-only)
 *               
 * COPYRIGHT:    (C) 2003-2011 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "ogr_api.h"
#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct _options options;
    struct _flags   flags;

    struct Map_info Map;
    
    FILE *fd;
    
    int ilayer, is3D;
    char buf[GPATH_MAX];
    const char *output;
    
    G_gisinit(argv[0]);
    
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("input"));
    G_add_keyword(_("external")); 
    module->description = _("Creates a new pseudo-vector map as a link to an OGR-supported layer.");
    
    parse_args(argc, argv,
	       &options, &flags);
    
    OGRRegisterAll();
    
    if (flags.format->answer) {
	list_formats(stdout);
	exit(EXIT_SUCCESS);
    }

    if (flags.layer->answer) {
	if (!options.dsn->answer)
	    G_fatal_error(_("Required parameter <%s> not set"), options.dsn->key);
	list_layers(stdout, options.dsn->answer, NULL, NULL);
	exit(EXIT_SUCCESS);
    }

    if (!options.output->answer)
	output = options.layer->answer;
    else
	output = options.output->answer;
    
    ilayer = list_layers(NULL, options.dsn->answer, options.layer->answer, &is3D);
    if (ilayer == -1) {
	G_fatal_error(_("Layer <%s> not available"), options.layer->answer);
    }
    
    G_debug(2, "layer '%s' was found", options.layer->answer);

    if (G_find_vector2(output, G_mapset()) && !G_check_overwrite(argc, argv)) {
	G_fatal_error(_("option <%s>: <%s> exists."),
		      options.output->key, output);
    }
    
    Vect_open_new(&Map, output, is3D);
    Vect_hist_command(&Map);
    Vect_close(&Map);
    
    /* Vect_open_new created 'head', 'coor', 'hist' -> delete 'coor' and create 'frmt' */
    sprintf(buf, "%s/%s/vector/%s/coor", G_location_path(), G_mapset(),
	    output);
    G_debug(2, "Delete '%s'", buf);
    if (unlink(buf) == -1) {
	Vect_delete(output);
	G_fatal_error(_("Unable to delete '%s'"), buf);
    }

    /* Create frmt */
    sprintf(buf, "%s/%s", GV_DIRECTORY, output);
    fd = G_fopen_new(buf, GV_FRMT_ELEMENT);
    if (fd == NULL) {
	Vect_delete(output);
	G_fatal_error("Unable to open file '%s'", buf);
    }
    
    fprintf(fd, "FORMAT: ogr\n");
    fprintf(fd, "DSN: %s\n", options.dsn->answer);
    fprintf(fd, "LAYER: %s\n", options.layer->answer);
    
    fclose(fd);
    
    if (!flags.topo->answer) {
      Vect_open_old(&Map, output, G_mapset());
      Vect_build(&Map);
      Vect_close(&Map);
    }

    G_done_msg(_("Link to vector map <%s> created."), output);

    exit(EXIT_SUCCESS);
}
