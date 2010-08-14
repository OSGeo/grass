#include <grass/config.h>
#include <grass/dbmi.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_isdir(const char *path)
{
    STRUCT_STAT x;

    if (stat(path, &x) != 0)
	return DB_FAILED;
    return (S_ISDIR(x.st_mode) ? DB_OK : DB_FAILED);
}
