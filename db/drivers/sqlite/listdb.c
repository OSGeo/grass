/*!
  \file db/drivers/sqlite/listdb.c
 
  \brief DBMI - Low Level SQLite database driver (list databases)
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author  Martin Landa <landa.martin gmail.com>
*/

#include <dirent.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

static int listdb(dbString*, int, dbHandle**, int *);
static void freedb(char **, int);

/*!
  \brief List SQLite databases for given paths.

  List all files with extension <b>.db</b> and it's possible to open
  them as SQLite database.

  \todo Do it better?
  
  \param path list of paths
  \param npaths number of given paths (?)
  \param[out] handles output dbHandles
  \param[out] count number of output handles

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db__driver_list_databases(dbString* path, int npaths,
                              dbHandle** handles, int *count)
{
    if (npaths < 1) {
        db_d_append_error(_("No path given"));
        db_d_report_error();
        return DB_FAILED;
    }

    if (strcmp(db_get_string(path), "") == 0) {
        /* current location */
        char *cpath;
        dbString spath;

        *handles = NULL;
        *count = 0;
        
        cpath = G_mapset_path();
        
        db_init_string(&spath);
        db_set_string(&spath, cpath);
        db_append_string(&spath, "/");
        db_append_string(&spath, "sqlite");
        G_free(cpath);
        
        if (listdb(&spath, 1, handles, count) != DB_OK)
            return DB_FAILED;
    }
    else {
        if (listdb(path, npaths, handles, count) != DB_OK)
            return DB_FAILED;
    }
    
    return DB_OK;
}

/* list .db file in the given path
   TODO: do it better?
*/
int listdb(dbString* path, int npaths,
           dbHandle** handles, int *count)
{
    int i, hcount, len;
    dbHandle *hlist;
    char **dlist;
    
    DIR *dirp;
    struct dirent *dp;

    if (npaths < 0)
        return DB_FAILED;
    
    hcount = 0;
    G_debug(3, "path = %s", db_get_string(path));

    /* count number of dbs */
    dirp = opendir(db_get_string(path));
    if (dirp == NULL) {
        db_d_append_error(_("Unable to open directory '%s'"),
                          db_get_string(path));
        db_d_report_error();
        return DB_FAILED;
    }
    
    dlist = NULL;
    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_name[0] == '.')
            continue;
        len = strlen(dp->d_name) - 3;
        /* try to open file as SQLite database */
        if (len > 0 && G_strcasecmp(dp->d_name + len, ".db") == 0) {
            char fpath[GPATH_MAX];
            
            sprintf(fpath, "%s/%s", db_get_string(path), dp->d_name);
            if (sqlite3_open(fpath, &sqlite) == SQLITE_OK) {
                if (sqlite3_close(sqlite) == SQLITE_BUSY) {
                    db_d_append_error(_("SQLite database connection '%s' is still busy"),
                                      dp->d_name);
                    continue;
                }
                
                hcount++;
                dlist = (char **) G_realloc(dlist, hcount * sizeof(char *));
                G_debug(3, "%s", dp->d_name);
                dlist[hcount-1] = G_store(fpath);
            }
        }
    }
    G_debug(1, "db count = %d", hcount);

    /* allocate handles */
    hlist = db_alloc_handle_array(hcount);
    if (hlist == NULL) {
        db_d_append_error(_("Out of memory"));
        db_d_report_error();

        freedb(dlist, hcount);
        closedir(dirp);
        
        return DB_FAILED;
    }

    /* get db names */
    for (i = 0; i < hcount; i++) {
        db_init_handle(&hlist[i]);
        if (db_set_handle(&hlist[i], dlist[i], NULL) != DB_OK) {
            db_d_append_error(_("Unable to set handle"));
            db_d_report_error();
            
            db_free_handle_array(hlist, hcount);
            freedb(dlist, hcount);
            closedir(dirp);
            
            return DB_FAILED;
        }
    }

    freedb(dlist, hcount);
    closedir(dirp);

    *handles = hlist;
    *count = hcount;

    return DB_OK;
}

/* free allocated memory */
static void freedb(char **dblist, int count)
{
    int i;

    for(i = 0; i < count; i++)
        G_free(dblist[i]);
    
    G_free(dblist);
}
