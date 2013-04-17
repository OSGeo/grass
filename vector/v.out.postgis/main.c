/***************************************************************
 *
 * MODULE:       v.out.postgis
 *
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Converts GRASS vector map layer to PostGIS
 *
 * COPYRIGHT:    (C) 2012-2013 by Martin Landa, and the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include <libpq-fe.h>

#include "local_proto.h"

static void link_handler(void *);
static void output_handler(void *);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct params params;
    struct flags flags;
    
    int ret, field;
    char *schema, *olayer, *pg_file;
    char *fid_column, *geom_column;
    
    struct Map_info In, Out;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword(_("PostGIS"));
    G_add_keyword(_("simple features"));
    G_add_keyword(_("topology"));

    module->description =
        _("Exports a vector map layer to PostGIS feature table.");
    module->overwrite = TRUE;
    
    define_options(&params, &flags);
    
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* if olayer not given, use input as the name */
    schema = NULL;
    if (!params.olayer->answer) {
        char name[GNAME_MAX], mapset[GMAPSET_MAX];
        
        /* check for fully qualified name */
        if (G_name_is_fully_qualified(params.input->answer,
                                      name, mapset))
            olayer = G_store(name);
        else
            olayer = G_store(params.input->answer);
        G_debug(1, "olayer=%s", olayer);
    }
    else {
        /* check for schema */
        char **tokens;

        tokens = G_tokenize(params.olayer->answer, ".");
        if (G_number_of_tokens(tokens) == 2) {
            schema = G_store(tokens[0]);
            olayer = G_store(tokens[1]);
        }
        else {
            olayer = G_store(params.olayer->answer);
        }
        
        G_free_tokens(tokens);
    }
    
    /* if schema not defined, use 'public' */
    if (!schema)
        schema = "public";
    G_debug(1, "Database schema: %s", schema);
        
    /* open input for reading */
    ret = Vect_open_old2(&In, params.input->answer, "", params.layer->answer);
    if (ret == -1)
        G_fatal_error(_("Unable to open vector map <%s>"),
                      params.input->answer);
    if (Vect_maptype(&In) != GV_FORMAT_NATIVE)
        G_fatal_error(_("Vector map <%s> is not in native format. Export cancelled."),
                      Vect_get_full_name(&In));
    Vect_set_error_handler_io(&In, NULL);
    if (params.olink->answer)
	G_add_error_handler(link_handler, params.olink->answer);

    if (ret < 2) 
        G_warning(_("Unable to open vector map <%s> on topological level"),
                  params.input->answer);
    
    /* default columns */
    fid_column  = GV_PG_FID_COLUMN;
    geom_column = GV_PG_GEOMETRY_COLUMN;
    
    /* create output for writing */
    pg_file = create_pgfile(params.dsn->answer, schema, params.olink->answer,
                            params.opts->answers, flags.topo->answer ? TRUE : FALSE,
			    &fid_column, &geom_column);
    G_debug(1, "fid_column: %s", fid_column);
    G_debug(1, "geom_column: %s", geom_column);
    
    if (!flags.table->answer) {
	/* check fid column */
	check_columns(&In, params.layer->answer, fid_column, geom_column);
    }

    /* don't use temporary maps, writes vector features immediately to
       the output PostGIS layer */
    putenv("GRASS_VECTOR_EXTERNAL_IMMEDIATE=1");
    if (-1 == Vect_open_new(&Out, olayer, Vect_is_3d(&In)))
        G_fatal_error(_("Unable to create PostGIS layer <%s>"),
                      olayer);
    G_add_error_handler(output_handler, &Out);
    
    /* define attributes */
    field = Vect_get_field_number(&In, params.layer->answer);
    if (!flags.table->answer)
        Vect_copy_map_dblinks(&In, &Out, TRUE);

    /* copy vector features & create PostGIS table*/
    if (Vect_copy_map_lines_field(&In, field, &Out) != 0)
        G_fatal_error(_("Copying features failed"));

    /* close input map */
    Vect_close(&In);
    
    /* build topology for output map */
    if (Vect_build(&Out) != 1)
        G_fatal_error(_("Building %s topology failed"),
                      flags.topo->answer ? "PostGIS" : "pseudo");

    G_done_msg(_("Feature table <%s> created in database <%s>."),
               Vect_get_finfo_layer_name(&Out), Vect_get_finfo_dsn_name(&Out));
    
    /* close output map */
    Vect_close(&Out);

    /* remove PG file */
    G_remove("", pg_file);
    
    exit(EXIT_SUCCESS);
}

void link_handler(void *p)
{
    const char *link = (const char *) p;
    
    G_debug(1, "link_handler: %s", link);
    if (G_find_vector2(link, G_mapset()))
	Vect_delete(link);
}

void output_handler(void *p)
{
    char stmt[DB_SQL_MAX];
       
    struct Map_info *Map;
    struct Format_info_pg *pg_info;
    PGresult *result;
    
    Map = (struct Map_info *) p;
    pg_info = &Map->fInfo.pg;
    
    G_debug(1, "output_handler(): schema = %s; olayer = %s", pg_info->schema_name, pg_info->table_name);
    sprintf(stmt, "SELECT DropGeometryTable('%s', '%s')", pg_info->schema_name, pg_info->table_name);
    result = PQexec(pg_info->conn, stmt);
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        G_warning(_("Unable to drop table <%s.%s>"), pg_info->schema_name, pg_info->table_name);
    }
    PQclear(result);

    if (pg_info->toposchema_name) {
        sprintf(stmt, "SELECT topology.DropTopology('%s')", pg_info->toposchema_name);
        result = PQexec(pg_info->conn, stmt);
        if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
            G_warning(_("Unable to drop topology schema <%s>"), pg_info->toposchema_name);
        }
    }
    PQclear(result);
}
