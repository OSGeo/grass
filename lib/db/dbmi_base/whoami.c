#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
char *db_whoami()
{
    char *userid = G_store(getenv("LOGNAME"));

    return userid;
}
