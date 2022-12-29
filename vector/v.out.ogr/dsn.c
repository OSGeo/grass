#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

char *get_datasource_name(const char *opt_dsn, int use_ogr)
{
    char *dsn;

    dsn = G_store(opt_dsn);
    
    return dsn;

    /* input OGR dsn and GRASS db connections are independent of each other */
    /* TODO: remove below code */
    if (G_strncasecmp(opt_dsn, "PG:", 3) == 0) {
        /* PostgreSQL/PostGIS */
        size_t i;
        char connect_str[DB_SQL_MAX], database[GNAME_MAX];
        char *p, *pp;
        const char *user, *passwd, *host, *port;

        /* dbname is mandatory */
        p = G_strcasestr(opt_dsn, "dbname");
        if (!p) 
            G_fatal_error(_("Invalid connection string (dbname missing)"));
        
        /* get dbname */
        p += strlen("dbname=");
        for (i = 0, pp = p; *pp != ' ' && *pp != '\0'; pp++, i++)
            database[i] = *pp;
        database[i] = '\0';
        
        /* build connection string */
        sprintf(connect_str, "dbname=%s", database);
        
        /* add db.login settings (user, password, host, port) */
        if (DB_OK == db_get_login2("pg", database, &user, &passwd, &host, &port)) {
            if (user) {
                if (!G_strcasestr(opt_dsn, "user=")) {
                    strcat(connect_str, " user=");
                    strcat(connect_str, user);
                }
                G_free((char *)user);
            }
            if (passwd) {
                if (!G_strcasestr(opt_dsn, "password=")) {
                    strcat(connect_str, " password=");
                    strcat(connect_str, passwd);
                }
                G_free((char *)passwd);
            }
            if (host) {
                if (!G_strcasestr(opt_dsn, "host=")) {
                    strcat(connect_str, " host=");
                    strcat(connect_str, host);
                }
                G_free((char *)host);
            }
            if (port) {
                if (!G_strcasestr(opt_dsn, "port=")) {
                    strcat(connect_str, " port=");
                    strcat(connect_str, port);
                }
                G_free((char *)port);
            }
        }
        
        if (!use_ogr)
            /* be friendly, ignored 'PG:' prefix for PostGIS links */
            dsn = G_store(connect_str);
        else
            G_asprintf(&dsn, "PG:%s", connect_str);
    }
    else {
        /* other datasources */
        dsn = G_store(opt_dsn);
    }

    G_debug(1, "dsn: %s", dsn);

    return dsn;
}
