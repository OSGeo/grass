/* TODO: should this go into strings.c ? */

#include <grass/gis.h>


/*!
 * \brief delimiter
 *
 * position of delimiter
 *
 *  \param str
 *  \param delim
 *  \return char * 
 */

char *G_index(const char *str, int delim)
{
    while (*str && *str != delim)
	str++;
    if (delim == 0)
	return (char *)str;
    return *str ? (char *)str : NULL;
}


/*!
 * \brief ???
 *
 * ???
 *
 *  \param str
 *  \param delim
 *  \return char * 
 */

char *G_rindex(const char *str, int delim)
{
    const char *p;

    p = NULL;
    while (*str) {
	if (*str == delim)
	    p = str;
	str++;
    }
    if (delim == 0)
	return (char *)str;
    return (char *)p;
}
