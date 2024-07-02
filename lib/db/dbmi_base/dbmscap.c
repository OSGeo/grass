/*!
   \file lib/db/dbmi_base/dbmscap.c

   \brief DBMI Library (base) - DBmscap management

   (C) 1999-2009 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <grass/dbmi.h>
#include <grass/gis.h>

static char *dbmscap_files[] = {"/etc/dbmscap",
                                "/lib/dbmscap",
                                "/usr/lib/dbmscap",
                                "/usr/local/lib/dbmscap",
                                "/usr/local/dbmi/lib/dbmscap",
                                NULL};

static void add_entry(dbDbmscap **list, char *name, char *startup,
                      char *comment);

static char *dbmscap_filename(int err_flag)
{
    char *file;
    int i;

    file = getenv("DBMSCAP");
    if (file)
        return file;

    for (i = 0; (file = dbmscap_files[i]); i++) {
        if (access(file, 0) == 0)
            return file;
    }
    if (err_flag)
        db_error("DBMSCAP not set");

    return ((char *)NULL);
}

/*!
   \brief Get dbmscap file name

   \return pointer to string with file name
 */
const char *db_dbmscap_filename(void)
{
    return dbmscap_filename(1);
}

/*!
   \brief Check dbms

   \return 1 if true
   \return 0 if false
 */
int db_has_dbms(void)
{
    return (dbmscap_filename(0) != NULL);
}

/*!
   \brief Copy dbmscap entry

   \param dst destination
   \param src source
 */
void db_copy_dbmscap_entry(dbDbmscap *dst, dbDbmscap *src)
{
    strcpy(dst->driverName, src->driverName);
    strcpy(dst->comment, src->comment);
    strcpy(dst->startup, src->startup);
}

/*!
   \brief Read dbmscap

   dbmscap file was used in grass5.0 but it is not used in
   grass5.7 until we find it necessary. All code for dbmscap
   file is commented here.

   Instead of in dbmscap file db_read_dbmscap() searches
   for available dbmi drivers in $(GISBASE)/driver/db/

   \return pointer to dbDbmscap
 */
dbDbmscap *db_read_dbmscap(void)
{
    /*
       FILE *fd;
       char *file;
       char name[1024];
       char startup[1024];
       char comment[1024];
       int  line;
     */
    char *dirpath;
    DIR *dir;
    struct dirent *ent;

    dbDbmscap *list = NULL;

    /* START OF OLD CODE FOR dbmscap FILE - NOT USED, BUT KEEP IT FOR FUTURE */
#if 0
    /* get the full name of the dbmscap file */

    file = db_dbmscap_filename();
    if (file == NULL)
        return (dbDbmscap *) NULL;


    /* open the dbmscap file */

    fd = fopen(file, "r");
    if (fd == NULL) {
        db_syserror(file);
        return (dbDbmscap *) NULL;
    }


    /* find all valid entries
     * blank lines and lines with # as first non blank char are ignored
     * format is:
     *   driver name:startup command:comment
     */

    for (line = 1; fgets(buf, sizeof buf, fd); line++) {
        if (sscanf(buf, "%1s", comment) != 1 || *comment == '#')
            continue;
        if (sscanf(buf, "%[^:]:%[^:]:%[^:\n]", name, startup, comment) == 3)
            add_entry(&list, name, startup, comment);
        else if (sscanf(buf, "%[^:]:%[^:\n]", name, startup) == 2)
            add_entry(&list, name, startup, "");
        else {
            fprintf(stderr, "%s: line %d: invalid entry\n", file, line);
            fprintf(stderr, "%d:%s\n", line, buf);
        }
        if (list == NULL)
            break;
    }
    fclose(fd);
#endif
    /* END OF OLD CODE FOR dbmscap FILE */

    /* START OF NEW CODE FOR SEARCH IN $(GISBASE)/driver/db/ */

    /* opend db drivers directory */
#ifdef _WIN32
    dirpath = G_malloc(strlen("\\driver\\db\\") + strlen(G_gisbase()) + 1);
    sprintf(dirpath, "%s\\driver\\db\\", G_gisbase());
    G_convert_dirseps_to_host(dirpath);
#else
    G_asprintf(&dirpath, "%s/driver/db/", G_gisbase());
#endif

    G_debug(2, "dbDbmscap(): opendir [%s]", dirpath);
    dir = opendir(dirpath);
    if (dir == NULL) {
        db_syserror("Cannot open drivers directory");
        return (dbDbmscap *)NULL;
    }
    G_free(dirpath);

    /* read all drivers */
    while ((ent = readdir(dir))) {
        char *name;

        if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0))
            continue;

#ifdef _WIN32
        /* skip manifest files on Windows */
        if (strstr(ent->d_name, ".manifest"))
            continue;
#endif

        /* Remove '.exe' from name (windows extension) */
        name = G_str_replace(ent->d_name, ".exe", "");

#ifdef _WIN32
        dirpath = G_malloc(strlen("\\driver\\db\\") + strlen(G_gisbase()) +
                           strlen(ent->d_name) + 1);
        sprintf(dirpath, "%s\\driver\\db\\%s", G_gisbase(), ent->d_name);
        G_convert_dirseps_to_host(dirpath);
#else
        G_asprintf(&dirpath, "%s/driver/db/%s", G_gisbase(), ent->d_name);
#endif
        add_entry(&list, name, dirpath, "");
        G_free(name);
        G_free(dirpath);
    }

    closedir(dir);

    return list;
}

static int cmp_entry(dbDbmscap *a, dbDbmscap *b)
{
    return (*a->driverName && *b->driverName
                ? strcmp(a->driverName, b->driverName)
                : 0);
}

static void add_entry(dbDbmscap **list, char *name, char *startup,
                      char *comment)
{
    /* add an entry to the list, so that the list remains ordered (by
     * driverName) */

    dbDbmscap *head, *cur, *tail;

    cur = (dbDbmscap *)db_malloc(sizeof(dbDbmscap));
    if (cur == NULL) {
        *list = NULL;
        return;
        /* out of memory */
    }
    cur->next = NULL;

    /* copy each item to the dbmscap structure */
    strcpy(cur->driverName, name);
    strcpy(cur->startup, startup);
    strcpy(cur->comment, comment);

    /* find the last entry that is less than cur */
    tail = head = *list;
    while (tail && tail->next && cmp_entry(tail->next, cur) < 0)
        tail = tail->next;

    /* handle the first call (head == NULL) */
    if (tail && cmp_entry(tail, cur) < 0) {
        /* insert right after tail */
        cur->next = tail->next;
        tail->next = cur;
    }
    else {
        /* insert at first position */
        cur->next = head;
        head = cur;
    }

    *list = head;
}

/*!
   \brief Free dbmscap

   \param list pointer to dbDbmscap
 */
void db_free_dbmscap(dbDbmscap *list)
{
    dbDbmscap *next, *cur;

    for (cur = list; cur; cur = next) {
        next = cur->next;
        db_free(cur);
    }
}
