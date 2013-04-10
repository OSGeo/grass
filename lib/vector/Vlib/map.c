/*!
  \file lib/vector/Vlib/map.c
  
  \brief Vector library - Manipulate vector map (copy, rename, delete)
  
  Higher level functions for reading/writing/manipulating vectors.
  
  (C) 2001-2009, 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
*/

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <grass/glocale.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

/*!
   \brief Copy file

   \param src source file
   \param[out] dst destination file

   \return 0 OK
   \return 1 error
 */
static int copy_file(const char *src, const char *dst)
{
    char buf[1024];
    int fd, fd2;
    FILE *f2;
    int len, len2;

    if ((fd = open(src, O_RDONLY)) < 0)
        return 1;

    /* if((fd2 = open(dst, O_CREAT|O_TRUNC|O_WRONLY)) < 0) { */
    if ((f2 = fopen(dst, "w")) == NULL) {
        close(fd);
        return 1;
    }

    fd2 = fileno(f2);

    while ((len = read(fd, buf, 1024)) > 0) {
        while (len && (len2 = write(fd2, buf, len)) >= 0)
            len -= len2;
    }

    close(fd);
    /* close(fd2); */
    fclose(f2);

    if (len == -1 || len2 == -1)
        return 1;

    return 0;
}

/*!
   \brief Copy vector map including attribute tables

   Note: Output vector map is overwritten if exists!

   \param in name if vector map to be copied
   \param mapset mapset name where the input map is located
   \param out name for output vector map (new map is created in current mapset)

   \return -1 error
   \return 0 success
 */
int Vect_copy(const char *in, const char *mapset, const char *out)
{
    int i, n, ret, type;
    struct Map_info In, Out;
    struct field_info *Fi, *Fin;
    char old_path[GPATH_MAX], new_path[GPATH_MAX], buf[GPATH_MAX];
    const char *files[] = { GV_FRMT_ELEMENT, GV_COOR_ELEMENT,
        GV_HEAD_ELEMENT, GV_HIST_ELEMENT,
        GV_TOPO_ELEMENT, GV_SIDX_ELEMENT, GV_CIDX_ELEMENT,
        NULL
    };
    const char *inmapset;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    dbDriver *driver;

    G_debug(2, "Copy vector '%s' in '%s' to '%s'", in, mapset, out);
    /* check for [A-Za-z][A-Za-z0-9_]* in name */
    if (Vect_legal_filename(out) < 0)
        G_fatal_error(_("Vector map name is not SQL compliant"));

    inmapset = G_find_vector2(in, mapset);
    if (!inmapset) {
        G_warning(_("Unable to find vector map <%s> in <%s>"), in, mapset);
        return -1;
    }
    mapset = inmapset;

    /* remove mapset from fully qualified name, confuses G_file_name() */
    if (G_name_is_fully_qualified(in, xname, xmapset)) {
        in = xname;
    }

    /* Delete old vector if it exists */
    if (G_find_vector2(out, G_mapset())) {
        G_warning(_("Vector map <%s> already exists and will be overwritten"),
                  out);
        ret = Vect_delete(out);
        if (ret != 0) {
            G_warning(_("Unable to delete vector map <%s>"), out);
            return -1;
        }
    }

    /* Copy the directory */
    G__make_mapset_element(GV_DIRECTORY);
    sprintf(buf, "%s/%s", GV_DIRECTORY, out);
    G__make_mapset_element(buf);

    i = 0;
    while (files[i]) {
        sprintf(buf, "%s/%s", in, files[i]);
        G_file_name(old_path, GV_DIRECTORY, buf, mapset);
        sprintf(buf, "%s/%s", out, files[i]);
        G_file_name(new_path, GV_DIRECTORY, buf, G_mapset());

        if (access(old_path, F_OK) == 0) {      /* file exists? */
            G_debug(2, "copy %s to %s", old_path, new_path);
            if (copy_file(old_path, new_path)) {
                G_warning(_("Unable to copy vector map <%s> to <%s>"),
                          old_path, new_path);
            }
        }
        i++;
    }

    G_file_name(old_path, GV_DIRECTORY, in, mapset);
    G_file_name(new_path, GV_DIRECTORY, out, G_mapset());

    /* Open input */
    Vect_set_open_level(1);
    Vect_open_old_head(&In, in, mapset);

    if (In.format != GV_FORMAT_NATIVE) {        /* Done */
        Vect_close(&In);
        return 0;
    }

    /* Open output */
    Vect_set_open_level(1);
    Vect_open_update_head(&Out, out, G_mapset());

    /* Copy tables */
    n = Vect_get_num_dblinks(&In);
    type = GV_1TABLE;
    if (n > 1)
        type = GV_MTABLE;
    for (i = 0; i < n; i++) {
        Fi = Vect_get_dblink(&In, i);
        if (Fi == NULL) {
            G_warning(_("Database connection not defined for layer %d"),
                      In.dblnk->field[i].number);
            Vect_close(&In);
            Vect_close(&Out);
            return -1;
        }
        Fin = Vect_default_field_info(&Out, Fi->number, Fi->name, type);
        G_debug(3, "Copy drv:db:table '%s:%s:%s' to '%s:%s:%s'",
                Fi->driver, Fi->database, Fi->table, Fin->driver,
                Fin->database, Fin->table);

        Vect_map_add_dblink(&Out, Fi->number, Fi->name, Fin->table, Fi->key,
                            Fin->database, Fin->driver);

        ret = db_copy_table(Fi->driver, Fi->database, Fi->table,
                            Fin->driver, Vect_subst_var(Fin->database, &Out),
                            Fin->table);
        if (ret == DB_FAILED) {
            G_warning(_("Unable to copy table <%s>"), Fin->table);
            Vect_close(&In);
            Vect_close(&Out);
            return -1;
        }

        driver =
            db_start_driver_open_database(Fin->driver,
                                          Vect_subst_var(Fin->database,
                                                         &Out));
        if (driver == NULL) {
            G_warning(_("Unable to open database <%s> by driver <%s>"),
                      Fin->database, Fin->driver);
        }
        else {
            if (db_create_index2(driver, Fin->table, Fi->key) != DB_OK)
                G_warning(_("Unable to create index for table <%s>, key <%s>"),
                          Fi->table, Fi->key);

            db_close_database_shutdown_driver(driver);
        }
    }

    Vect_close(&In);
    Vect_close(&Out);

    return 0;
}

/*!
   \brief Rename existing vector map (in the current mapset).

   Attribute tables are created in the same database where input tables were stored.

   The origial format (native/OGR) is used.

   Note: Output vector map is overwritten if exists!

   \param in name of vector map to be renamed
   \param out name for output vector map

   \return -1 error
   \return 0 success
 */
int Vect_rename(const char *in, const char *out)
{
    int i, n, ret, type;
    struct Map_info Map;
    struct field_info *Fin, *Fout;
    int *fields;
    dbDriver *driver;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    G_debug(2, "Rename vector '%s' to '%s'", in, out);
    /* check for [A-Za-z][A-Za-z0-9_]* in name */
    if (Vect_legal_filename(out) < 0)
        G_fatal_error(_("Vector map name is not SQL compliant"));

    /* Delete old vector if it exists */
    if (G_find_vector2(out, G_mapset())) {
        G_warning(_("Vector map <%s> already exists and will be overwritten"),
                  out);
        Vect_delete(out);
    }

    /* remove mapset from fully qualified name */
    if (G_name_is_fully_qualified(in, xname, xmapset)) {
        in = xname;
    }

    /* Move the directory */
    ret = G_rename(GV_DIRECTORY, in, out);

    if (ret == 0) {
        G_warning(_("Vector map <%s> not found"), in);
        return -1;
    }
    else if (ret == -1) {
        G_warning(_("Unable to copy vector map <%s> to <%s>"), in, out);
        return -1;
    }

    /* Rename all tables if the format is native */
    Vect_set_open_level(1);
    Vect_open_update_head(&Map, out, G_mapset());

    if (Map.format != GV_FORMAT_NATIVE) {       /* Done */
        Vect_close(&Map);
        return 0;
    }

    /* Copy tables */
    n = Vect_get_num_dblinks(&Map);
    type = GV_1TABLE;
    if (n > 1)
        type = GV_MTABLE;

    /* Make the list of fields */
    fields = (int *)G_malloc(n * sizeof(int));

    for (i = 0; i < n; i++) {
        Fin = Vect_get_dblink(&Map, i);
        fields[i] = Fin->number;
    }

    for (i = 0; i < n; i++) {
        G_debug(3, "field[%d] = %d", i, fields[i]);

        Fin = Vect_get_field(&Map, fields[i]);
        if (Fin == NULL) {
            G_warning(_("Database connection not defined for layer %d"),
                      fields[i]);
            Vect_close(&Map);
            return -1;
        }

        Fout = Vect_default_field_info(&Map, Fin->number, Fin->name, type);
        G_debug(3, "Copy drv:db:table '%s:%s:%s' to '%s:%s:%s'",
                Fin->driver, Fin->database, Fin->table, Fout->driver,
                Fout->database, Fout->table);

        /* TODO: db_rename_table instead of db_copy_table */
        ret = db_copy_table(Fin->driver, Fin->database, Fin->table,
                            Fout->driver, Vect_subst_var(Fout->database,
                                                         &Map), Fout->table);

        if (ret == DB_FAILED) {
            G_warning(_("Unable to copy table <%s>"), Fin->table);
            Vect_close(&Map);
            return -1;
        }

        /* Change the link */
        Vect_map_del_dblink(&Map, Fin->number);

        Vect_map_add_dblink(&Map, Fout->number, Fout->name, Fout->table,
                            Fin->key, Fout->database, Fout->driver);

        /* Delete old table */
        ret = db_delete_table(Fin->driver, Fin->database, Fin->table);
        if (ret == DB_FAILED) {
            G_warning(_("Unable to delete table <%s>"), Fin->table);
            Vect_close(&Map);
            return -1;
        }

        driver =
            db_start_driver_open_database(Fout->driver,
                                          Vect_subst_var(Fout->database,
                                                         &Map));
        if (driver == NULL) {
            G_warning(_("Unable to open database <%s> by driver <%s>"),
                      Fout->database, Fout->driver);
        }
        else {
            if (db_create_index2(driver, Fout->table, Fin->key) != DB_OK)
                G_warning(_("Unable to create index for table <%s>, key <%s>"),
                          Fout->table, Fout->key);

            db_close_database_shutdown_driver(driver);
        }
    }

    Vect_close(&Map);
    free(fields);

    return 0;
}

/*!
   \brief Delete vector map including attribute tables

   Vector map must be located in current mapset.
   
   \param map name of vector map to be delete 

   \return -1 error
   \return 0 success
 */
int Vect_delete(const char *map)
{
    return Vect__delete(map, FALSE);
}

/*!
  \brief Delete vector map (internal use only)
  
  \param map name of vector map to be delete 
  \param is_tmp TRUE for temporary maps

  \return -1 error
  \return 0 success
*/
int Vect__delete(const char *map, int is_tmp)
{
    int ret;
    char *path, path_buf[GPATH_MAX];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    const char *tmp, *mapset;
    
    struct Map_info Map;
    
    DIR *dir;
    struct dirent *ent;

    G_debug(3, "Delete vector '%s' (is_tmp = %d)", map, is_tmp);

    mapset = G_mapset();
    
    /* remove mapset from fully qualified name */
    if (G_name_is_fully_qualified(map, xname, xmapset)) {
        if (strcmp(mapset, xmapset) != 0)
            G_warning(_("Ignoring invalid mapset: %s"), xmapset);
        map = xname;
    }

    if (map == NULL || strlen(map) == 0) {
        G_warning(_("Invalid vector map name <%s>"), map ? map : "null");
        return -1;
    }

    Vect_set_open_level(1); /* Topo not needed */
    ret = Vect__open_old(&Map, map, mapset, NULL, FALSE, TRUE, is_tmp);
    if (ret < 1) {
        if (is_tmp)
            return 0; /* temporary vector map doesn't exist */
        else {
            G_warning(_("Unable to open header file for vector map <%s>"),
                      map);
            return -1;
        }
    }
        
    path = Vect__get_element_path(&Map, GV_DBLN_ELEMENT);
    G_debug(1, "dbln file: %s", path);

    if (access(path, F_OK) == 0) {
        int i, n;
        struct field_info *Fi;
        
        /* Delete all tables, NOT external (OGR) */
        if (Map.format == GV_FORMAT_NATIVE) {
            n = Vect_get_num_dblinks(&Map);
            for (i = 0; i < n; i++) {
                Fi = Vect_get_dblink(&Map, i);
                if (Fi == NULL) {
                    G_warning(_("Database connection not defined for layer %d"),
                              Map.dblnk->field[i].number);
                    Vect_close(&Map);
                    return -1;
                }
                G_debug(3, "Delete drv:db:table '%s:%s:%s'", Fi->driver,
                        Fi->database, Fi->table);

                ret = db_table_exists(Fi->driver, Fi->database, Fi->table);
                if (ret == -1) {
                    G_warning(_("Unable to find table <%s> linked to vector map <%s>"),
                              Fi->table, map);
                    Vect_close(&Map);
                    return -1;
                }

                if (ret == 1) {
                    ret =
                        db_delete_table(Fi->driver, Fi->database, Fi->table);
                    if (ret == DB_FAILED) {
                        G_warning(_("Unable to delete table <%s>"),
                                  Fi->table);
                        Vect_close(&Map);
                        return -1;
                    }
                }
                else {
                    G_warning(_("Table <%s> linked to vector map <%s> does not exist"),
                              Fi->table, map);
                }
            }
        }
    }
    G_free(path);
    
    /* Delete all files from vector/name directory */
    path = Vect__get_element_path(&Map, NULL);
    Vect_close(&Map);
    G_debug(3, "opendir '%s'", path);
    dir = opendir(path);
    if (dir == NULL) {
        G_warning(_("Unable to open directory '%s'"), path);
        return -1;
    }

    while ((ent = readdir(dir))) {
        G_debug(3, "file = '%s'", ent->d_name);
        if ((strcmp(ent->d_name, ".") == 0) ||
            (strcmp(ent->d_name, "..") == 0))
            continue;
        
        sprintf(path_buf, "%s/%s", path, ent->d_name);
        G_debug(3, "delete file '%s'", path_buf);
        ret = unlink(path_buf);
        if (ret == -1) {
            G_warning(_("Unable to delete file '%s'"), path_buf);
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);
    
    /* NFS can create .nfsxxxxxxxx files for those deleted 
     *  -> we have to move the directory to ./tmp before it is deleted */
    tmp = G_tempfile();

    G_debug(3, "rename '%s' to '%s'", path, tmp);
    ret = rename(path, tmp);
    if (ret == -1) {
        G_warning(_("Unable to rename directory '%s' to '%s'"), path, tmp);
        return -1;
    }
    G_free(path);

    G_debug(3, "remove directory '%s'", tmp);
    /* Warning: remove() fails on Windows */
    ret = rmdir(tmp);
    if (ret == -1) {
        G_warning(_("Unable to remove directory '%s'"), tmp);
        return -1;
    }

    return 0;
}

/*!
   \brief Set spatial index to be realease when vector is closed.

   By default, the memory occupied by spatial index is not released.

   \param Map vector map
 */
void Vect_set_release_support(struct Map_info *Map)
{
    Map->plus.release_support = TRUE;
}

/*!
   \brief Set category index to be updated when vector is changed.

   By default, category index is not updated if vector is changed,
   this function sets category index update.

   WARNING: currently only category for elements is updated not for
   areas

   \param Map vector map
*/
void Vect_set_category_index_update(struct Map_info *Map)
{
    Map->plus.update_cidx = TRUE;
}
