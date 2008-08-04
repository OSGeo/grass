#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_zero(void *s, int n)
{
    char *c = (char *)s;

    while (n-- > 0)
	*c++ = 0;
}
