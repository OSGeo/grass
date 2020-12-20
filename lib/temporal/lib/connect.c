/*!
  \file lib/temporal/lib/connect.c
  
  \brief Temporal GIS Library - connect to TGIS DB
  
  (C) 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Soeren Gebbert 
          Code is based on the dbmi library written by 
          Joel Jones (CERL/UIUC) and Radim Blazek
*/
#include <grass/temporal.h>
#include <grass/glocale.h>

/*!
 * \brief Get TGIS driver name
 * 
 * \return pointer to TGIS driver name
 * \return NULL if not set
 */
char *tgis_get_driver_name(void)
{
    const char *drv;
    
    if ((drv = G_getenv_nofatal2("TGISDB_DRIVER", G_VAR_MAPSET)))
        return G_store(drv);
    
    return NULL;
}

/*!
 * \brief Get TGIS database name
 * 
 * \return pointer to TGIS database name
 * \return NULL if not set
 */
char *tgis_get_database_name(void)
{
    const char *drv;
    
    if ((drv = G_getenv_nofatal2("TGISDB_DATABASE", G_VAR_MAPSET)))
        return G_store(drv);
    
    return NULL;
}

/*!
  \brief Set Temporal GIS DB connection settings

  This function sets environmental variables as TGISDB_DRIVER, TGISDB_DATABASE.

  \param connection pointer to dbConnection with default settings
  
  \return DB_OK
 */
int tgis_set_connection(dbConnection * connection)
{
    if (connection->driverName)
	G_setenv2("TGISDB_DRIVER", connection->driverName, G_VAR_MAPSET);

    if (connection->databaseName)
	G_setenv2("TGISDB_DATABASE", connection->databaseName, G_VAR_MAPSET);

    return DB_OK;
}

/*!
  \brief Get Temporal GIS DB connection settings
  
  \param[out] connection pointer to dbConnection to be modified

  \return DB_OK
 */
int tgis_get_connection(dbConnection * connection)
{
    G_zero(connection, sizeof(dbConnection));
    
    connection->driverName = (char *)G_getenv_nofatal2("TGISDB_DRIVER", G_VAR_MAPSET);
    connection->databaseName = (char *)G_getenv_nofatal2("TGISDB_DATABASE", G_VAR_MAPSET);

    return DB_OK;
}

#define DRIVER_NAME 0
#define DATABASE_NAME 1

static char *get_subproject_connection_name(const char *subproject, int contype)
{
    const char *val = NULL;
    char *ret_val = NULL;;
    const char *gisdbase = G_getenv_nofatal("GISDBASE");
    const char *project = G_getenv_nofatal("LOCATION_NAME");
    int ret;

    G_debug(1,"Checking subproject <%s>", subproject);
    ret = G_subproject_permissions2(gisdbase, project, subproject);
    switch (ret) {
    case 0: /* Check if the subproject exists and user is owner */
        /* We suppress this warning, since G_subproject_permission2() does not
         * properly check the access privileges to the subproject of a different user.
         * TODO: develop a dedicated G_subproject_permission3() for that
        G_warning(_("You are not the owner of subproject <%s>"),
                      subproject);
        */
        break;
    case -1:
        G_warning(_("Subproject <%s> does not exist."),
	              subproject);
        return ret_val;
    default:
        break;
    }    

    G_create_alt_env();
    G_setenv_nogisrc("GISDBASE", gisdbase);
    G_setenv_nogisrc("LOCATION_NAME", project);
    G_setenv_nogisrc("MAPSET", subproject);
    G__read_subproject_env();
 
    if(contype == DATABASE_NAME) {
        if ((val = G_getenv_nofatal2("TGISDB_DATABASE", G_VAR_MAPSET)))
            ret_val = G_store(val);
    } else if(contype == DRIVER_NAME) {
        if ((val = G_getenv_nofatal2("TGISDB_DRIVER", G_VAR_MAPSET)))
            ret_val = G_store(val);
    }

    G_switch_env();
    
    return ret_val;
}


/*!
 * \brief Get TGIS driver name from a specific subproject
 *
 * This function give a warning in case the subproject does not exists
 * or it is not allowed to access the subproject. NULL is returned in this case.
 * 
 * \param subproject The name of the subproject to receive the driver name from
 *
 * \return pointer to TGIS driver name
 * \return NULL if not set 
 */
char *tgis_get_subproject_driver_name(const char *subproject)
{
    return get_subproject_connection_name(subproject, DRIVER_NAME);
}

/*!
 * \brief Get TGIS database name
 * 
 * This function give a warning in case the subproject does not exists
 * or it is not allowed to access the subproject. NULL is returned in this case..
 * 
 * \param subproject The name of the subproject to receive the driver name from
 
 * \return pointer to TGIS database name
 * \return NULL if not set
 */
char *tgis_get_subproject_database_name(const char *subproject)
{
    return get_subproject_connection_name(subproject, DATABASE_NAME);
}

