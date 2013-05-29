
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
    
    int ilayer, use_ogr;
    char buf[GPATH_MAX], *dsn;
    const char *output;
    
    G_gisinit(argv[0]);
    
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("input"));
    G_add_keyword(_("external")); 
    G_add_keyword("OGR");
    G_add_keyword("PostGIS");
    module->description = _("Creates a new pseudo-vector map as a link to an OGR-supported layer "
                            "or a PostGIS feature table.");
    parse_args(argc, argv,
               &options, &flags);

    use_ogr = TRUE;
    G_debug(1, "GRASS_VECTOR_OGR defined? %s",
            getenv("GRASS_VECTOR_OGR") ? "yes" : "no");
    if(options.dsn->answer &&
       G_strncasecmp(options.dsn->answer, "PG:", 3) == 0) {
        /* -> PostgreSQL */
#if defined HAVE_OGR && defined HAVE_POSTGRES
        if (getenv("GRASS_VECTOR_OGR"))
            use_ogr = TRUE;
        else
            use_ogr = FALSE;
#else
#ifdef HAVE_POSTGRES
        if (getenv("GRASS_VECTOR_OGR"))
            G_warning(_("Environment variable GRASS_VECTOR_OGR defined, "
                        "but GRASS is compiled with OGR support. "
                        "Using GRASS-PostGIS data driver instead."));
        use_ogr = FALSE;
#else /* -> force using OGR */
        G_warning(_("GRASS is not compiled with PostgreSQL support. "
                    "Using OGR-PostgreSQL driver instead of native "
                    "GRASS-PostGIS data driver."));
        use_ogr = TRUE;
#endif /* HAVE_POSTRES */
#endif /* HAVE_OGR && HAVE_POSTGRES */
    }
    
#ifdef HAVE_OGR
    if (use_ogr)
        OGRRegisterAll();
#endif

    if (flags.format->answer) {
        /* list formats */
        list_formats(stdout);
        exit(EXIT_SUCCESS);
    }

    /* be friendly, ignored 'PG:' prefix for PostGIS links */
    dsn = NULL;
    if (options.dsn->answer) {
        if (!use_ogr) {
            int i, length;
            
            length = strlen(options.dsn->answer);
            dsn = (char *) G_malloc(length - 3);
            for (i = 3; i < length; i++)
                dsn[i-3] = options.dsn->answer[i];
            dsn[length-3] = '\0';
        }
        else {
            dsn = G_store(options.dsn->answer);
        }
    }
    
    if (flags.list->answer || flags.tlist->answer) {
        /* list layers */
        if (!dsn)
            G_fatal_error(_("Required parameter <%s> not set"), options.dsn->key);
        list_layers(stdout, dsn, NULL,
                    flags.tlist->answer ? TRUE : FALSE, use_ogr);
        exit(EXIT_SUCCESS);
    }

    /* define name for output */
    if (!options.output->answer)
        output = options.layer->answer;
    else
        output = options.output->answer;
    

    /* get layer index */
    ilayer = list_layers(NULL, dsn, options.layer->answer,
                         FALSE, use_ogr);
    if (ilayer == -1) {
        G_fatal_error(_("Layer <%s> not available"), options.layer->answer);
    }
    
    G_debug(2, "layer '%s' was found", options.layer->answer);

    if (G_find_vector2(output, G_mapset()) && !G_check_overwrite(argc, argv)) {
        G_fatal_error(_("option <%s>: <%s> exists."),
                      options.output->key, output);
    }
    
    /* create new vector map */
    putenv("GRASS_VECTOR_EXTERNAL_IGNORE=1");
    Vect_open_new(&Map, output, WITHOUT_Z); /* dimension is set later from data source */
    Vect_set_error_handler_io(NULL, &Map);
    
    Vect_hist_command(&Map);
    Vect_close(&Map);
    
    /* Vect_open_new created 'head', 'coor', 'hist'
       -> delete 'coor' and create 'frmt' */
    sprintf(buf, "%s/%s/%s/%s/coor", G_location_path(), G_mapset(),
            GV_DIRECTORY, output);
    G_debug(2, "Delete '%s'", buf);
    if (unlink(buf) == -1) {
        G_fatal_error(_("Unable to delete '%s'"), buf);
    }

    /* create frmt file */
    sprintf(buf, "%s/%s", GV_DIRECTORY, output);
    fd = G_fopen_new(buf, GV_FRMT_ELEMENT);
    if (fd == NULL)
        G_fatal_error(_("Unable to create file '%s/%s'"), buf, GV_FRMT_ELEMENT);
    
    if (!use_ogr) {
        char *table_name, *schema_name;
        
        get_table_name(options.layer->answer, &table_name, &schema_name);
        
        fprintf(fd, "format: postgis\n");
        fprintf(fd, "conninfo: %s\n", dsn);
        if (schema_name)
            fprintf(fd, "schema: %s\n", schema_name);
        fprintf(fd, "table: %s\n", table_name);

        G_free(table_name);
        G_free(schema_name);
    }
    else {
        fprintf(fd, "format: ogr\n");
        fprintf(fd, "dsn: %s\n", dsn);
        fprintf(fd, "layer: %s\n", options.layer->answer);
    }
    fclose(fd);
    
    if (!flags.topo->answer) {
        Vect_set_open_level(1);
        Vect_open_old(&Map, output, G_mapset());
        Vect_build(&Map);
        Vect_close(&Map);
    }
    
    G_done_msg(_("Link to vector map <%s> created."), output);
    
    exit(EXIT_SUCCESS);
}
