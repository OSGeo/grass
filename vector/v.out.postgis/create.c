#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

static void file_handler(void *);

void create_table(struct Map_info *In, struct Map_info *Out)
{
    int type;
    struct Format_info_pg *pg_info;

    pg_info = &(Out->fInfo.pg);
    if (pg_info->feature_type != SF_UNKNOWN)
        return;
    
    /* create PostGIS table if doesn't exist
       determine feature type from the first feature */
    Vect_rewind(In);
    Vect_set_constraint_type(In, GV_POINT | GV_LINES);
    type = Vect_read_next_line(In, NULL, NULL);
    Vect_rewind(In);
    
    if (Vect_write_line(Out, type, NULL, NULL) < 0)
        G_fatal_error(_("Unable to create PostGIS layer <%s>"),
                      Vect_get_finfo_layer_name(Out));
}

char *create_pgfile(const char *dsn, const char *schema, const char *olink,
                    char **options, int topo,
		    char **fid_column, char **geom_column)
{
    int   i;
    char *filename, *conninfo;
    FILE *fp;
    
    struct Key_Value *key_val;
    
    filename = NULL;
    G_asprintf(&filename, "PG_%d", (int) getpid());
    G_debug(1, "PG file: %s", filename);
    
    fp = G_fopen_new("", filename);
    if (!fp)
        G_fatal_error(_("Unable to create <%s> file"), filename);
    setenv("GRASS_VECTOR_PGFILE", filename, TRUE);
    G_add_error_handler(file_handler, filename);

    key_val = G_create_key_value();
    
   /* be friendly, ignored 'PG:' prefix for GRASS-PostGIS data driver */
    if (G_strncasecmp(dsn, "PG:", 3) == 0) {
        int length;
        
        length = strlen(dsn);
        conninfo = (char *) G_malloc(length - 2);
        for (i = 3; i < length; i++)
            conninfo[i-3] = dsn[i];
        conninfo[length-3] = '\0';
    }
    else {
        conninfo = G_store(dsn);
    }
    
    /* required options */
    G_set_key_value("conninfo", conninfo, key_val);
    if (schema)
        G_set_key_value("schema", schema, key_val);
    if (topo)
        G_set_key_value("topology", "yes", key_val);

    /* extra options */
    if (options) {
	char **tokens;

	for(i = 0; options[i]; i++) {
	    tokens = G_tokenize(options[i], "=");
	    if (G_number_of_tokens(tokens) != 2) {
		G_warning(_("Invalid option skipped: %s"), options[i]);
		continue;
	    }
	    G_debug(1, "option: %s=%s", tokens[0], tokens[1]);
            /* force upper case */
            G_str_to_lower(tokens[0]);
	    G_set_key_value(tokens[0], tokens[1], key_val);
	    
	    if (strcmp(tokens[0], "fid") == 0)
		G_asprintf(fid_column, tokens[1]);
	    if (strcmp(tokens[0], "geometry_name") == 0)
		G_asprintf(geom_column, tokens[1]);

	    G_free_tokens(tokens);
	}
    }
    
    if (olink) {
        /* create a link for output feature table */
        G_set_key_value("link", "yes", key_val);
        G_set_key_value("link_name", olink, key_val);
    }
    else {
        G_set_key_value("link", "no", key_val);
    }

    if (G_fwrite_key_value(fp, key_val) < 0)
        G_fatal_error(_("Error writing <%s> file"), filename);

    fclose(fp);

    G_free(conninfo);
    
    return filename;
}

void file_handler(void *p) {
    const char *filename = (const char *) p;
    
    G_debug(1, "file_handler: %s", filename);
    G_remove("", filename);
    unsetenv("GRASS_VECTOR_PGFILE");
}
