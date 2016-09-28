/*!
  \file lib/db/dbmi_base/login.c
  
  \brief DBMI Library (base) - login settings
  
  (C) 1999-2015 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

typedef struct
{
    char *driver;
    char *database;
    char *user;
    char *password;
    char *host;
    char *port;
} DATA;

typedef struct
{
    int n, a;
    DATA *data;
} LOGIN;

static const char *login_filename(void)
{
    static char *file;

    if (!file) {
	file = (char *)db_malloc(GPATH_MAX);
	sprintf(file, "%s%cdblogin", G_config_path(), HOST_DIRSEP);
    }
    return file;
}

static void init_login(LOGIN * login)
{
    login->n = 0;
    login->a = 10;

    login->data = (DATA *) malloc(login->a * sizeof(DATA));
}

static void add_login(LOGIN * login, const char *dr, const char *db, const char *usr,
		      const char *pwd, const char *host, const char *port, int idx)
{
    int login_idx;
    
    G_debug(3, "add_login(): drv='%s' db='%s' usr='%s' pwd='%s' host='%s', port='%s'",
            dr, db, usr ? usr : "null", pwd ? pwd : "null", host ? host : "null",
            port ? port : "null");

    if (login->n == login->a) {
	login->a += 10;
	login->data =
	    (DATA *) realloc((void *)login->data, login->a * sizeof(DATA));
    }
    if (idx > -1 && idx < login->n) {
        login_idx = idx;
    }
    else {
        login_idx = login->n;
        login->n++;
    }
    login->data[login_idx].driver = G_store(dr);
    login->data[login_idx].database = G_store(db);
    login->data[login_idx].user = G_store(usr ? usr : "");
    login->data[login_idx].password = G_store(pwd ? pwd : "");
    login->data[login_idx].host = G_store(host ? host : "");
    login->data[login_idx].port = G_store(port ? port : "");
}

/*
   Read the DB login file if it exists
   return: -1 error (cannot read file)
   number of items (0 also if file does not exist)
 */
static int read_file(LOGIN * login)
{
    int ret;
    const char *file;
    FILE *fd;
    char buf[DB_SQL_MAX];
    char **tokens;

    login->n = 0;
    file = login_filename();

    G_debug(3, "read_file(): DB login file = <%s>", file);

    if (access(file, F_OK) != 0) {
	G_debug(3, "login file does not exist");
	return 0;
    }

    fd = fopen(file, "r");
    if (fd == NULL) {
        G_warning(_("Unable to read file '%s'"), file);
	return -1;
    }

    while (G_getl2(buf, 2000, fd)) {
	G_chop(buf);

        tokens = G_tokenize(buf, "|");
        ret = G_number_of_tokens(tokens);

	if (ret < 2) {
	    G_warning(_("Login file (%s) corrupted (line: %s)"), file, buf);
            G_free_tokens(tokens);
	    continue;
	}

	add_login(login,
                  tokens[0],                  /* driver */
                  tokens[1],                  /* database */
                  ret > 2 ? tokens[2] : NULL, /* user*/
                  ret > 3 ? tokens[3] : NULL, /* password */
                  ret > 4 ? tokens[4] : NULL, /* host */
                  ret > 5 ? tokens[5] : NULL, /* port */
                  -1);
        G_free_tokens(tokens);
    }

    fclose(fd);

    return (login->n);
}

/*
   Write the DB login file
   return: -1 error (cannot read file)
   0 OK
 */
static int write_file(LOGIN * login)
{
    int i;
    const char *file;
    FILE *fd;

    file = login_filename();

    G_debug(3, "write_file(): DB login file = <%s>", file);

    fd = fopen(file, "w");
    if (fd == NULL) {
        G_warning(_("Unable to write file '%s'"), file);
	return -1;
    }

    /* fchmod is not available on Windows */
    /* fchmod ( fileno(fd), S_IRUSR | S_IWUSR ); */
    chmod(file, S_IRUSR | S_IWUSR);

    for (i = 0; i < login->n; i++) {
	fprintf(fd, "%s|%s", login->data[i].driver, login->data[i].database);
	if (login->data[i].user) {
	    fprintf(fd, "|%s", login->data[i].user);

	    if (login->data[i].password)
		fprintf(fd, "|%s", login->data[i].password);
	}
        if (login->data[i].host)
            fprintf(fd, "|%s", login->data[i].host);
        if (login->data[i].port)
            fprintf(fd, "|%s", login->data[i].port);

        fprintf(fd, "\n");
    }

    fclose(fd);

    return 0;
}

static int set_login(const char *driver, const char *database, const char *user,
                     const char *password, const char *host, const char *port,
                     int overwrite)
{
    int i, found;
    LOGIN login;

    G_debug(3, "db_set_login(): drv=[%s] db=[%s] usr=[%s] pwd=[%s] host=[%s] port=[%s]",
	    driver, database, user, password, host, port);

    init_login(&login);

    if (read_file(&login) == -1)
	return DB_FAILED;

    found = FALSE;
    for (i = 0; i < login.n; i++) {
	if (strcmp(login.data[i].driver, driver) == 0 &&
	    strcmp(login.data[i].database, database) == 0) {
	    if (user)
		login.data[i].user = G_store(user);
	    else
		login.data[i].user = G_store("");

	    if (password)
		login.data[i].password = G_store(password);
	    else
		login.data[i].password = G_store("");

	    found = TRUE;
	    break;
	}
    }

    if (found) {
        if (overwrite)
            G_warning(_("DB connection <%s/%s> already exists and will be overwritten"),
                      driver, database ? database : "");
        else
            G_fatal_error(_("DB connection <%s/%s> already exists. "
                            "Re-run '%s' with '--%s' flag to overwrite existing settings."),
                          driver, database ? database : "", G_program_name(), "overwrite");
    }
    
    if (!found)
	add_login(&login, driver, database, user, password, host, port, -1);
    else
        add_login(&login, driver, database, user, password, host, port, i);

    if (write_file(&login) == -1)
	return DB_FAILED;

    return DB_OK;
}

/*!
  \brief Set login parameters for driver/database
  
  \deprecated Use db_set_login2() instead.
  
  \todo: GRASS 8: to be replaced by db_set_login2().

  \param driver driver name
  \param database database name
  \param user user name
  \param password password string
  
  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_set_login(const char *driver, const char *database, const char *user,
                 const char *password)
{
    return set_login(driver, database, user,
                     password, NULL, NULL, FALSE);
}

/*!
  \brief Set login parameters for driver/database
  
  \param driver driver name
  \param database database name
  \param user user name
  \param password password string
  \param host host name
  \param port
  \param overwrite TRUE to overwrite existing connections
  
  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_set_login2(const char *driver, const char *database, const char *user,
		 const char *password, const char *host, const char *port,
                 int overwrite)
{
    return set_login(driver, database, user,
                     password, host, port, overwrite);
}

static int get_login(const char *driver, const char *database, const char **user,
                     const char **password, const char **host, const char **port)
{
    int i;
    LOGIN login;

    G_debug(3, "db_get_login(): drv=[%s] db=[%s]", driver, database);

    user[0] = '\0';
    password[0] = '\0';
    host[0] = '\0';
    port[0] = '\0';
    
    init_login(&login);

    if (read_file(&login) == -1)
	return DB_FAILED;

    for (i = 0; i < login.n; i++) {
	if (strcmp(login.data[i].driver, driver) == 0 &&
	    (!database || strcmp(login.data[i].database, database) == 0)) {
	    if (login.data[i].user && strlen(login.data[i].user) > 0)
		*user = G_store(login.data[i].user);
	    else
		*user = NULL;

	    if (login.data[i].password && strlen(login.data[i].password) > 0)
		*password = G_store(login.data[i].password);
	    else
		*password = NULL;

            if (login.data[i].host && strlen(login.data[i].host) > 0 && host)
		*host = G_store(login.data[i].host);
	    else
		*host = NULL;

            if (login.data[i].port && strlen(login.data[i].port) > 0 && port)
		*port = G_store(login.data[i].port);
	    else
		*port = NULL;

	    break;
	}
    }

    return DB_OK;
}

/*!  
  \brief Get login parameters for driver/database

  If driver/database is not found, output arguments are set to NULL.

  \deprecated Use db_set_login2() instead.
  
  \todo: GRASS 8: to be replaced by db_set_login2().

  \param driver driver name
  \param database database name (can be NULL)
  \param[out] user name
  \param[out] password string

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_get_login(const char *driver, const char *database, const char **user,
		 const char **password)
{
    return get_login(driver, database, user,
                     password, NULL, NULL);
}

/*!  
  \brief Get login parameters for driver/database

  If driver/database is not found, output arguments are set to NULL.
  
  \param driver driver name
  \param database database name (can be NULL)
  \param[out] user name
  \param[out] password string
  \param[out] host name
  \param[out] port

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_get_login2(const char *driver, const char *database, const char **user,
                  const char **password, const char **host, const char **port)
{
    return get_login(driver, database, user,
                     password, host, port);
}

/*!  
  \brief Print all connection settings to file
  
  \param fd file where to print settings
  
  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_get_login_dump(FILE *fd)
{
    int i;
    LOGIN login;
    
    G_debug(3, "db_get_login_dump()");
    
    init_login(&login);
    if (read_file(&login) == -1)
	return DB_FAILED;
    
    for (i = 0; i < login.n; i++) {
        fprintf(fd, "%s|%s|%s|%s|%s|%s\n",
                login.data[i].driver,
                login.data[i].database,
                login.data[i].user,
                login.data[i].password,
                login.data[i].host,
                login.data[i].port);
    }
    
    return DB_OK;
}
