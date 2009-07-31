
/****************************************************************
 *
 * MODULE:       v.external
 * 
 * AUTHOR(S):    Radim Blazek
 *               Updated by Martin Landa <landa.martin gmail.com> (2009)
 *               
 * PURPOSE:      Create a new vector as a link to OGR layer (read-only)
 *               
 * COPYRIGHT:    (C) 2003-2009 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/

#include <grass/config.h>
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
    char buf[GPATH_MAX];
    FILE *fd;
    struct Map_info Map;

    int layer;
    
    struct {
	struct Option *dsn, *output, *layer;
    } options;

    struct {
      struct Flag *format, *layer, *topo;
    } flags;
    
    G_gisinit(argv[0]);
    
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("external"));
    G_add_keyword(_("ogr"));

    module->description = _("Creates a new vector as a read-only link to OGR layer.");

    options.dsn = G_define_option();
    options.dsn->key = "dsn";
    options.dsn->type = TYPE_STRING;
    options.dsn->gisprompt = "old_file,file,dsn";
    options.dsn->label = _("OGR data source name");
    options.dsn->description = _("Examples:\n"
				 "\t\tESRI Shapefile: directory containing shapefiles\n"
				 "\t\tMapInfo File: directory containing mapinfo files");
    options.dsn->guisection = _("Data source");

    options.layer = G_define_option();
    options.layer->key = "layer";
    options.layer->type = TYPE_STRING;
    options.layer->required = NO;
    options.layer->multiple = NO;
    options.layer->label = _("OGR layer name");
    options.layer->description = _("Examples:\n"
				   "\t\tESRI Shapefile: shapefile name\n"
				   "\t\tMapInfo File: mapinfo file name");
    options.layer->guisection = _("Data source");

    options.output = G_define_standard_option(G_OPT_V_OUTPUT);
    options.output->required = NO;
    options.output->description = _("Name for output vector map");

    flags.format = G_define_flag();
    flags.format->key = 'f';
    flags.format->description = _("List supported formats and exit");
    flags.format->guisection = _("Print");

    flags.layer = G_define_flag();
    flags.layer->key = 'l';
    flags.layer->description = _("List available layers and exit");
    flags.layer->guisection = _("Print");

    flags.topo = G_define_flag();
    flags.topo->key = 'b';
    flags.topo->description = _("Do not build topology");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    OGRRegisterAll();
    
    if (flags.format->answer) {
	list_formats(stdout);
	exit(EXIT_SUCCESS);
    }

    if (flags.layer->answer) {
	if (!options.dsn->answer)
	    G_fatal_error(_("Required parameter <%s> not set"), options.dsn->key);
	list_layers(stdout, options.dsn->answer, NULL);
	exit(EXIT_SUCCESS);
    }

    if (!options.output->answer)
	G_fatal_error(_("Required parameter <%s> not set"), options.output->key);
    
    if (!options.layer->answer)
	G_fatal_error(_("Required parameter <%s> not set"), options.layer->key);
        

    layer = list_layers(NULL, options.dsn->answer, options.layer->answer);
    if (layer == -1) {
	G_fatal_error(_("Layer <%s> not available"), options.layer->answer);
    }
    
    G_debug(2, "layer '%s' was found", options.layer->answer);

    /* TODO: support 3d vector data */
    Vect_open_new(&Map, options.output->answer, WITHOUT_Z);
    Vect_hist_command(&Map);
    Vect_close(&Map);
    
    /* Vect_open_new created 'head', 'coor', 'hist' -> delete 'coor' and create 'frmt' */
    sprintf(buf, "%s/%s/vector/%s/coor", G_location_path(), G_mapset(),
	    options.output->answer);
    G_debug(2, "Delete '%s'", buf);
    if (unlink(buf) == -1) {
	Vect_delete(options.output->answer);
	G_fatal_error(_("Unable to delete '%s'"), buf);
    }

    /* Create frmt */
    sprintf(buf, "%s/%s", GV_DIRECTORY, options.output->answer);
    fd = G_fopen_new(buf, GV_FRMT_ELEMENT);
    if (fd == NULL) {
	Vect_delete(options.output->answer);
	G_fatal_error("Unable to open file '%s'", buf);
    }
    
    fprintf(fd, "FORMAT: ogr\n");
    fprintf(fd, "DSN: %s\n", options.dsn->answer);
    fprintf(fd, "LAYER: %s\n", options.layer->answer);
    
    fclose(fd);
    
    if (!flags.topo->answer) {
      Vect_open_old(&Map, options.output->answer, G_mapset());
      Vect_build(&Map);
      Vect_close(&Map);
    }

    G_done_msg(_("<%s> created."), options.output->answer);

    exit(EXIT_SUCCESS);
}
