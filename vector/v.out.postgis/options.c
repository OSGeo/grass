#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

static void file_handler(void *);

void define_options(struct params *params, struct flags *flags)
{
    params->input = G_define_standard_option(G_OPT_V_INPUT);
    params->input->description = NULL;
    
    params->layer = G_define_standard_option(G_OPT_V_FIELD);
    params->layer->description = NULL;
    
    params->dsn = G_define_option();
    params->dsn->key = "dsn";
    params->dsn->type = TYPE_STRING;
    params->dsn->required = YES;
    params->dsn->label = _("PostGIS output datasource name");
    params->dsn->description =
        _("Starts with 'PG' prefix, eg. 'PG:dbname=grass'");
    
    params->olayer = G_define_option();
    params->olayer->key = "olayer";
    params->olayer->type = TYPE_STRING;
    params->olayer->required = NO;
    params->olayer->label =
        _("Name for output PostGIS layer");
    params->olayer->description = 
        _("If not specified, input name is used");

    flags->table = G_define_flag();
    flags->table->key = 't';
    flags->table->description =
        _("Don't export attribute table");

    flags->topo = G_define_flag();
    flags->topo->key = 'l';
    flags->topo->description =
        _("Export PostGIS topology instead of simple features");
}

char *create_pgfile(const char *dsn, int topo)
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
