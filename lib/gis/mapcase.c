#include <grass/gis.h>
/*
 * Map uppercase A-Z to lower case a-z
 *
 */


/*!
 * \brief convert string to lower case
 *
 * Upper case
 * letters in the string <b>s</b> are converted to their lower case equivalent.
 * Returns <b>s.</b>
 *
 *  \param string
 *  \return char
 */

char *G_tolcase(char *string)
{
    char *p;

    for (p = string; *p; p++) {
	/* convert to lower case */
	if (*p >= 'A' && *p <= 'Z')
	    *p -= 'A' - 'a';
    }

    return (string);
}


/*
 * Map lowercase a-z to uppercase A-Z
 *
 */


/*!
 * \brief convert string to upper case
 *
 * Lower case letters in the string <b>s</b> are converted to their upper case equivalent.
 * Returns <b>s.</b>
 *
 *  \param string
 *  \return char
 */

char *G_toucase(char *string)
{
    char *p;

    for (p = string; *p; p++) {
	/* convert to upper case */
	if (*p >= 'A' && *p <= 'z')
	    *p += 'A' - 'a';
    }

    return (string);
}
