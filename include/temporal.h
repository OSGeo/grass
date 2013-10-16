#ifndef GRASS_TEMPORAL_H
#define GRASS_TEMPORAL_H

#include <grass/dbmi.h>

#define TGISDB_DEFAULT_DRIVER "sqlite"
#define TGISDB_DEFAULT_SQLITE_PATH "PERMANENT/tgis/sqlite.db"

int tgis_set_connection(dbConnection * connection);
int tgis_get_connection(dbConnection * connection);
const char *tgis_get_default_driver_name(void);
char *tgis_get_default_database_name(void);
char *tgis_get_driver_name(void);
char *tgis_get_database_name(void);
int tgis_set_default_connection(void);

#endif
