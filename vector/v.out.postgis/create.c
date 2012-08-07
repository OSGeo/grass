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
    
    /* create PostGIS table if doesn't exist */
    type = Vect_read_next_line(In, NULL, NULL);
    Vect_rewind(In);
    
    if (V2_open_new_pg(Out, type) < 0)
        G_fatal_error(_("Unable to create PostGIS layer <%s>"),
                      Vect_get_finfo_layer_name(Out));
}

char *create_pgfile(const char *dsn, const char *schema, int topo)
{
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
        int i, length;
        
        length = strlen(dsn);
        conninfo = (char *) G_malloc(length - 3);
        for (i = 3; i < length; i++)
            conninfo[i-3] = dsn[i];
        conninfo[length-3] = '\0';
    }
    else {
        conninfo = G_store(dsn);
    }
    
    G_set_key_value("conninfo", conninfo, key_val);
    if (schema)
        G_set_key_value("schema", schema, key_val);
    if (topo)
        G_set_key_value("topology", "on", key_val);

    if (G_fwrite_key_value(fp, key_val) < 0)
        G_fatal_error(_("Error writing <%s> file"), filename);

    fclose(fp);

    G_free(conninfo);
    
    return filename;
}

void file_handler(void *p) {
    const char *filename = (const char *) p;
    
    G_remove("", filename);
    unsetenv("GRASS_VECTOR_PGFILE");
}
