#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grass/dbmi.h>
#include "dbstubs.h"


static char *rfind(char *string, char c);
static int make_parent_dir(char *path, int mode);
static int make_dir(const char *path, int mode);


/*!
 \fn 
 \brief 
 \return 
 \param 
*/
int
db_driver_mkdir (const char *path, int mode, int parentdirs)
{
    if (parentdirs)
    {
	char path2[GPATH_MAX];
	strcpy(path2, path);
	if (make_parent_dir (path2, mode) != DB_OK)
	    return DB_FAILED;
    }

    return make_dir (path, mode);
}


/* make a directory if it doesn't exist */
/* this routine could be made more intelligent as to why it failed */
static int
make_dir (const char *path, int mode)
{
    if (db_isdir(path) == DB_OK)
	return DB_OK;

    if (G_mkdir (path) == 0)
	return DB_OK;

    db_syserror(path);

    return DB_FAILED;
}


static int
make_parent_dir(char *path, int mode)
{
    char *slash;
    int stat;

    slash = rfind(path,'/');
    if (slash == NULL || slash == path)
	return DB_OK; /* no parent dir to make. return ok */

    *slash = 0; /* add NULL to terminate parentdir string */
    if (access(path,0) == 0) /* path exists, good enough */
    {
	stat = DB_OK;
    }
    else if (make_parent_dir (path, mode) != DB_OK)
    {
	stat = DB_FAILED;
    }
    else if(make_dir (path, mode) == DB_OK)
    {
	stat = DB_OK;
    }
    else
    {
	stat = DB_FAILED;
    }
    *slash = '/';  /* put the slash back into the path */

    return stat;
}


static
char *rfind(char *string, char c)
{
    char *found;

    found = NULL;
    while (*string)
    {
	if (*string == c)
	    found = string;
	string++;
    }

    return found;
}
