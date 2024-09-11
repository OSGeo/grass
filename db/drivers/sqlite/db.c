/**
 * \file db.c
 *
 * \brief DBMI - Low Level SQLite database driver.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author Support for multiple connections by Markus Metz
 *
 * \date 2005-2011
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

/**
 * \brief Open SQLite database.
 *
 * \param[in,out] handle database handle
 *
 * \return DB_FAILED on error
 * \return DB_OK on success
 */
int db__driver_open_database(dbHandle *handle)
{
    char name2[GPATH_MAX], *path;
    const char *name;
    char name3[GPATH_MAX + 14], *env_nolock;
    int i;

    G_debug(3, "\ndb_driver_open_database()");

    name = db_get_handle_dbname(handle);

    /* if name is empty use connection.databaseName */
    if (strlen(name) == 0) {
        dbConnection connection;

        db_get_connection(&connection);
        name = connection.databaseName;
    }

    G_debug(3, "name = '%s'", name);

    /* parse variables in db.name if present */
    if (strchr(name, '$')) {
        char **tokens;
        int no_tokens, n;

        tokens = G_tokenize(name, "/");
        no_tokens = G_number_of_tokens(tokens);

        name2[0] = '\0';
        for (n = 0; n < no_tokens; n++) {
            G_chop(tokens[n]);
            if (n > 0)
                strcat(name2, "/");

            G_debug(3, "tokens[%d] = %s", n, tokens[n]);
            if (tokens[n][0] == '$') {
                G_strchg(tokens[n], '$', ' ');
                G_chop(tokens[n]);
                strcat(name2, G_getenv_nofatal(tokens[n]));
                G_debug(3, "   -> %s", G_getenv_nofatal(tokens[n]));
            }
            else {
                strcat(name2, tokens[n]);
            }
        }
        G_free_tokens(tokens);
    }
    else {
        if (G_strlcpy(name2, name, sizeof(name2)) >= sizeof(name2)) {
            db_d_append_error(_("Database name <%s> is too long"), name);
            db_d_report_error();
            return DB_FAILED;
        }
    }

    G_debug(2, "name2 = '%s'", name2);

    path = G_store(name2);
    path = G_convert_dirseps_to_host(path);
    i = strlen(path);
    while (path[i] != HOST_DIRSEP && i > 0)
        i--;

    path[i] = '\0';
    if (*path) {
        G_debug(2, "path to db is %s", path);

        /* create directory if not existing */
        if (access(path, 0) != 0) {
            if (G_mkdir(path) != 0)
                G_fatal_error(
                    _("Unable to create directory '%s' for sqlite database"),
                    path);
        }
    }
    G_free(path);

    env_nolock = getenv("GRASS_SQLITE_NOLOCK");
    if (env_nolock && *env_nolock && atoi(env_nolock)) {
        if (sqlite3_config(SQLITE_CONFIG_URI, 1) == SQLITE_OK) {
            if (sprintf(name3, "file:%s?nolock=1", name2) < 0) {
                return DB_FAILED;
            }
            G_important_message(_("Disabling SQLite locking"));
        }
        else {
            G_warning(_("The sqlite config option '%s' is not supported"),
                      "SQLITE_CONFIG_URI");
            if (G_strlcpy(name3, name2, sizeof(name3)) >= sizeof(name3)) {
                db_d_append_error(_("Database name <%s> is too long"), name2);
                db_d_report_error();
                return DB_FAILED;
            }
        }
    }
    else {
        if (G_strlcpy(name3, name2, sizeof(name3)) >= sizeof(name3)) {
            db_d_append_error(_("Database name <%s> is too long"), name2);
            db_d_report_error();
            return DB_FAILED;
        }
    }
    if (sqlite3_open(name3, &sqlite) != SQLITE_OK) {
        db_d_append_error("%s %s\n%s", _("Unable to open database:"), name3,
                          (char *)sqlite3_errmsg(sqlite));
        db_d_report_error();
        return DB_FAILED;
    }

    /* enable loading of extensions */
    sqlite3_enable_load_extension(sqlite, 1);

    /* set the sqlite busy handler */
    sqlite3_busy_handler(sqlite, sqlite_busy_callback, NULL);

    return DB_OK;
}

/**
 * \brief Close SQLite database.
 *
 * \return always returns DB_OK
 */
int db__driver_close_database(void)
{
    G_debug(3, "db_close_database()");

    if (sqlite3_close(sqlite) == SQLITE_BUSY)
        G_fatal_error(_("SQLite database connection is still busy"));

    return DB_OK;
}

/**
 * \brief Create new empty SQLite database.
 *
 * \param handle dbHandle
 *
 * \return DB_OK on success
 * \return DB_FAILED on failure
 */
int db__driver_create_database(dbHandle *handle)
{
    const char *name;
    char name2[GPATH_MAX], *env_nolock;

    name = db_get_handle_dbname(handle);

    G_debug(1, "db_create_database(): %s", name);

    if (access(name, F_OK) == 0) {
        db_d_append_error(_("Database <%s> already exists"), name);
        db_d_report_error();
        return DB_FAILED;
    }

    env_nolock = getenv("GRASS_SQLITE_NOLOCK");
    if (env_nolock && *env_nolock && atoi(env_nolock)) {
        if (sqlite3_config(SQLITE_CONFIG_URI, 1) == SQLITE_OK) {
            if (sprintf(name2, "file:%s?nolock=1", name) < 0) {
                return DB_FAILED;
            }
            G_important_message(_("Disabling SQLite locking"));
        }
        else {
            G_warning(_("The sqlite config option '%s' is not supported"),
                      "SQLITE_CONFIG_URI");
            if (G_strlcpy(name2, name, sizeof(name2)) >= sizeof(name2)) {
                db_d_append_error(_("Database name <%s> is too long"), name);
                db_d_report_error();
                return DB_FAILED;
            }
        }
    }
    else {
        if (G_strlcpy(name2, name, sizeof(name2)) >= sizeof(name2)) {
            db_d_append_error(_("Database name <%s> is too long"), name);
            db_d_report_error();
            return DB_FAILED;
        }
    }
    if (sqlite3_open(name2, &sqlite) != SQLITE_OK) {
        db_d_append_error("%s %s\n%s", _("Unable to create database:"), name,
                          (char *)sqlite3_errmsg(sqlite));
        db_d_report_error();
        return DB_FAILED;
    }

    return DB_OK;
}

/**
 * \brief Delete existing SQLite database.
 *
 * \param handle dbHandle
 *
 * \return DB_OK on success
 * \return DB_FAILED on failure
 */
int db__driver_delete_database(dbHandle *handle)
{
    const char *name;

    name = db_get_handle_dbname(handle);

    if (access(name, F_OK) != 0) {
        db_d_append_error(_("Database <%s> not found"), name);
        db_d_report_error();
        return DB_FAILED;
    }

    return remove(name) == 0 ? DB_OK : DB_FAILED;
}
