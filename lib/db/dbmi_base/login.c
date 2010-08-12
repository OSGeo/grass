/*!
  \file db/dbmi_base/login.c
  
  \brief DBMI Library (base) - login settings
  
  (C) 1999-2009 by the GRASS Development Team
  
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
	file = (char *)malloc(1000);
	sprintf(file, "%s/%s/dblogin", G_home(), CONFIG_DIR);
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
		      const char *pwd)
{
    if (login->n == login->a) {
	login->a += 10;
	login->data =
	    (DATA *) realloc((void *)login->data, login->a * sizeof(DATA));
    }
    login->data[login->n].driver = G_store(dr);
    login->data[login->n].database = G_store(db);
    login->data[login->n].user = G_store(usr ? usr : "");
    login->data[login->n].password = G_store(pwd ? pwd : "");

    login->n++;
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
    char buf[2001], dr[500], db[500], usr[500], pwd[500];

    login->n = 0;
    file = login_filename();

    G_debug(3, "DB login file = <%s>", file);

    if (access(file, F_OK) != 0) {
	G_debug(3, "login file does not exist");
	return 0;
    }

    fd = fopen(file, "r");
    if (fd == NULL)
	return -1;

    while (G_getl2(buf, 2000, fd)) {
	G_chop(buf);

	usr[0] = pwd[0] = '\0';
	ret = sscanf(buf, "%[^|]|%[^|]|%[^|]|%[^\n]", dr, db, usr, pwd);

	G_debug(3, "ret = %d : drv=[%s] db=[%s] usr=[%s] pwd=[%s]",
		ret, dr, db, usr, pwd);

	if (ret < 2) {
	    G_warning(_("Login file corrupted"));
	    continue;
	}

	add_login(login, dr, db, usr, pwd);
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

    G_debug(3, "DB login file = <%s>", file);

    fd = fopen(file, "w");
    if (fd == NULL)
	return -1;

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
	fprintf(fd, "\n");
    }

    fclose(fd);

    return 0;
}

/*!
   \brief Set user/password for driver/database
   
   \param driver driver name
   \param database database name
   \param user user name
   \param password password string
   
   \return DB_OK
   \return DB_FAILED
 */
int db_set_login(const char *driver, const char *database, const char *user,
		 const char *password)
{
    int i, found;
    LOGIN login;

    G_debug(3, "db_set_login(): drv=[%s] db=[%s] usr=[%s] pwd=[%s]",
	    driver, database, user, password);

    init_login(&login);

    if (read_file(&login) == -1)
	return DB_FAILED;

    found = 0;
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

	    found = 1;
	    break;
	}
    }

    if (!found)
	add_login(&login, driver, database, user, password);

    if (write_file(&login) == -1)
	return DB_FAILED;

    return DB_OK;
}

/*!
   \brief Get user/password for driver/database if driver/database is not found, user/password are set to NULL

   \param driver driver name
   \param database database name
   \param[out] user name
   \param[out] password string
   
   \return DB_OK
   \return DB_FAILED
 */
int db_get_login(const char *driver, const char *database, const char **user,
		 const char **password)
{
    int i;
    LOGIN login;

    G_debug(3, "db_get_login(): drv=[%s] db=[%s]", driver, database);

    user[0] = '\0';
    password[0] = '\0';

    init_login(&login);

    if (read_file(&login) == -1)
	return DB_FAILED;

    for (i = 0; i < login.n; i++) {
	if (strcmp(login.data[i].driver, driver) == 0 &&
	    strcmp(login.data[i].database, database) == 0) {
	    if (login.data[i].user && strlen(login.data[i].user) > 0)
		*user = G_store(login.data[i].user);
	    else
		*user = NULL;

	    if (login.data[i].password && strlen(login.data[i].password) > 0)
		*password = G_store(login.data[i].password);
	    else
		*password = NULL;

	    break;
	}
    }

    return DB_OK;
}
