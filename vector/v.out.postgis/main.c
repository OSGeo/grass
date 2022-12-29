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
    
    int ret, field, otype, verbose;
    char *schema, *olayer, *pg_file;
    char *fid_column, *geom_column;
    
    struct Map_info In, Out;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    G_add_keyword(_("PostGIS"));
    G_add_keyword(_("simple features"));
    G_add_keyword(_("topology"));
    G_add_keyword(_("3D"));
    
    module->description =
        _("Exports a vector map layer to PostGIS feature table.");
    module->overwrite = TRUE;
    
    define_options(&params, &flags);
    
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* parse parameters */
    otype = Vect_option_to_types(params.type);

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
    if (-1 == Vect_open_new(&Out, olayer,
                            !flags.force2d->answer ? Vect_is_3d(&In) : WITHOUT_Z))
        G_fatal_error(_("Unable to create PostGIS layer <%s>"),
                      olayer);
    G_add_error_handler(output_handler, &Out);

    /* copy attributes (must be done before checking output type
       otherwise attributes are not copied) */
    field = Vect_get_field_number(&In, params.layer->answer);

    /* BUG: this works only if the input vector uses for its attributes
     * the same PG connection to be used for the output */
    if (!flags.table->answer)
        Vect_copy_map_dblinks(&In, &Out, TRUE);

    /* check output type */
    if (otype < 1 && Vect_level(&In) > 1) {
        /* type 'auto' -> try to guess output feature type on level 2 */
        if (Vect_get_num_areas(&In) > 0)
            otype = GV_AREA;
        else if (Vect_get_num_primitives(&In, GV_LINE) > 0)
            otype = GV_LINE;
        else if (Vect_get_num_primitives(&In, GV_POINT) > 0)
            otype = GV_POINT;
    }
    if (otype > 0) {
        if (otype & (GV_FACE | GV_KERNEL))
            G_fatal_error(_("Feature type '%s' not supported"),
                          params.type->answer);
        
        /* set up output feature type if possible */
        if (Vect_write_line(&Out, otype, NULL, NULL) < 0)
            G_fatal_error(_("Feature type %d is not supported"), otype);

        Vect_set_constraint_type(&In, otype);
    }
    
    /* copy vector features & create PostGIS table */
    if (Vect_copy_map_lines_field(&In, field, &Out) != 0)
        G_fatal_error(_("Copying features failed"));

    /* close input map */
    Vect_close(&In);

    /* build topology for output map -> write output to DB */
    G_message(_("Writing output..."));
    verbose = G_verbose();
    if (!flags.topo->answer)
        G_set_verbose(0); /* do not print build info when writing simple features */
    
    Vect_build_partial(&Out, GV_BUILD_NONE);
    if (Vect_build(&Out) != 1)
        G_fatal_error(_("Building %s topology failed"),
                      flags.topo->answer ? "PostGIS" : "pseudo");
    G_set_verbose(verbose);
    
    if (Vect_get_num_lines(&Out) < 1)
        G_fatal_error(_("No features exported. PostGIS layer <%s> not created."),
                      Vect_get_name(&Out));

    if (!flags.topo->answer) 
        G_done_msg(n_("%d feature (%s type) written to <%s>.",
                      "%d features (%s type) written to <%s>.",
                      Vect_sfa_get_num_features(&Out)),
                   Vect_sfa_get_num_features(&Out), Vect_get_finfo_geometry_type(&Out),
                   Vect_get_name(&Out));
    else
        G_done_msg(n_("%d primitive written to <%s>.",
                      "%d primitives written to <%s>.",
                      Vect_get_num_lines(&Out)),
                   Vect_get_num_lines(&Out), 
                   Vect_get_name(&Out));
    
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
    /*
      be quiet - table may do not exists

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        G_warning(_("Unable to drop table <%s.%s>"), pg_info->schema_name, pg_info->table_name);
    }
    */
    PQclear(result);

    if (pg_info->toposchema_name) {
        sprintf(stmt, "SELECT topology.DropTopology('%s')", pg_info->toposchema_name);
        result = PQexec(pg_info->conn, stmt);
        if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
            G_warning(_("Unable to drop topology schema <%s>"), pg_info->toposchema_name);
        }
        PQclear(result);
    }
}
