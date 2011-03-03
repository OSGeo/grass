/*!
 * \file strings.c
 * 
 * \brief GIS Library - string/chring movement functions
 *
 * \todo merge interesting functions from ../datetime/scan.c here
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Dave Gerdes (USACERL), Michael Shapiro (USACERL), Amit
 * Parghi (USACERL), Bernhard Reiter (Intevation GmbH, Germany) and
 * many others
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <grass/gis.h>

#ifndef NULL
#define NULL		0
#endif

/*!
 * \brief String compare ignoring case (upper or lower)
 *
 * Returning a value that has the same sign as the difference between
 * the first differing pair of characters
 *
 * \param[in] x first string to compare
 * \param[in] y second string to compare
 *
 * \return 0 the two strings are equal
 * \return -1, 1
 */
int G_strcasecmp(const char *x, const char *y)
{
    int xx, yy;

    if (!x)
	return y ? -1 : 0;
    if (!y)
	return x ? 1 : 0;
    while (*x && *y) {
	xx = *x++;
	yy = *y++;
	if (xx >= 'A' && xx <= 'Z')
	    xx = xx + 'a' - 'A';
	if (yy >= 'A' && yy <= 'Z')
	    yy = yy + 'a' - 'A';
	if (xx < yy)
	    return -1;
	if (xx > yy)
	    return 1;
    }
    if (*x)
	return 1;
    if (*y)
	return -1;
    return 0;
}

/*!
 * \brief Copy string to allocated memory.
 *
 * This routine allocates enough memory to hold the string <b>s</b>,
 * copies <b>s</b> to the allocated memory, and returns a pointer
 * to the allocated memory.
 *
 * If <em>s</em> is NULL then empty string is returned.
 *
 *  \param s string
 *
 *  \return pointer to newly allocated string
 */

char *G_store(const char *s)
{
    char *buf;

    if (s == NULL) {
	buf = G_malloc(sizeof(char));
	buf[0] = '\0';
    }
    else {
	buf = G_malloc(strlen(s) + 1);
	strcpy(buf, s);
    }
    
    return buf;
}

/*!
 * \brief Replace all occurencies of character in string bug with new
 *
 * \param[in,out] bug base string
 * \param[in] character character to replace
 * \param[in] new new character
 *
 * \return bug string
 */
char *G_strchg(char *bug, char character, char new)
{
    char *help = bug;

    while (*help) {
	if (*help == character)
	    *help = new;
	help++;
    }
    return bug;
}

/*!
 * \brief Replace all occurencies of old_str in buffer with new_str
 *
 * Code example:
 * \code
 * char *name;
 * name = G_str_replace ( inbuf, ".exe", "" );
 * ... 
 * G_free (name);
 * \endcode
 *
 * \param buffer input string buffer
 * \param old_str string to be replaced
 * \param new_str new string
 *
 * \return the newly allocated string, input buffer is unchanged 
 */
char *G_str_replace(const char *buffer, const char *old_str, const char *new_str)
{
    char *B, *R;
    const char *N;
    char *replace;
    int count, len;

    /* Make sure old_str and new_str are not NULL */
    if (old_str == NULL || new_str == NULL)
	return G_store(buffer);
    /* Make sure buffer is not NULL */
    if (buffer == NULL)
	return NULL;

    /* Make sure old_str occurs */
    B = strstr(buffer, old_str);
    if (B == NULL)
	/* return NULL; */
	return G_store(buffer);

    if (strlen(new_str) > strlen(old_str)) {
	/* Count occurences of old_str */
	count = 0;
	len = strlen(old_str);
	B = buffer;
	while (B != NULL && *B != '\0') {
	    B = strstr(B, old_str);
	    if (B != NULL) {
		B += len;
		count++;
	    }
	}

	len = count * (strlen(new_str) - strlen(old_str))
	    + strlen(buffer);

    }
    else
	len = strlen(buffer);

    /* Allocate new replacement */
    replace = G_malloc(len + 1);
    if (replace == NULL)
	return NULL;

    /* Replace old_str with new_str */
    B = buffer;
    R = replace;
    len = strlen(old_str);
    while (*B != '\0') {
	if (*B == old_str[0] && strncmp(B, old_str, len) == 0) {
	    N = new_str;
	    while (*N != '\0')
		*R++ = *N++;
	    B += len;
	}
	else {
	    *R++ = *B++;
	}
    }
    *R = '\0';

    return replace;
}

/*!
 * \brief Removes all leading and trailing white space from string.
 *
 * \param[in,out] buf buffer to be worked on
 *
 * \return
 */
void G_strip(char *buf)
{
    char *a, *b;

    /* remove leading white space */
    for (a = b = buf; *a == ' ' || *a == '\t'; a++) ;
    if (a != b)
	while ((*b++ = *a++)) ;
    /* remove trailing white space */
    for (a = buf; *a; a++) ;
    if (a != buf) {
	for (a--; *a == ' ' || *a == '\t'; a--) ;
	a++;
	*a = 0;
    }
}

/*!
 * \brief Chop leading and trailing white spaces:
 * \verbatim space, \f, \n, \r, \t, \v \endverbatim
 *
 * modified copy of G_squeeze();    RB March 2000 <Radim.Blazek@dhv.cz>
 *
 * \param line buffer to be worked on
 *
 * \return pointer to string
 */
char *G_chop(char *line)
{
    char *f = line, *t = line;

    while (isspace(*f))		/* go to first non white-space char */
	f++;

    if (!*f) {			/* no more chars in string */
	*t = '\0';
	return (line);
    }

    for (t = line; *t; t++)	/* go to end */
	;
    while (isspace(*--t)) ;
    *++t = '\0';		/* remove trailing white-spaces */

    t = line;
    while (*f)			/* copy */
	*t++ = *f++;
    *t = '\0';

    return (line);
}

/*!
 * \brief Convert string to upper case
 *
 * \param[in,out] str pointer to string
 */
void G_str_to_upper(char *str)
{
    int i = 0;

    if (!str)
	return;

    while (str[i]) {
	str[i] = toupper(str[i]);
	i++;
    }
}

/*!
 * \brief Convert string to lower case
 *
 * \param[in,out] str pointer to string
 */
void G_str_to_lower(char *str)
{
    int i = 0;

    if (!str)
	return;

    while (str[i]) {
	str[i] = tolower(str[i]);
	i++;
    }
}

/*!
 * \brief Make string SQL compliant
 *
 * \param[in,out] str pointer to string
 *
 * \return number of changed characters
 */
int G_str_to_sql(char *str)
{
    int count;
    char *c;

    count = 0;

    if (!str || !*str)
	return 0;

    c = str;
    while (*c) {
	*c = toascii(*c);

	if (!(*c >= 'A' && *c <= 'Z') && !(*c >= 'a' && *c <= 'z') &&
	    !(*c >= '0' && *c <= '9')) {
	    *c = '_';
	    count++;
	}
	c++;
    }

    c = str;
    if (!(*c >= 'A' && *c <= 'Z') && !(*c >= 'a' && *c <= 'z')) {
	*c = 'x';
	count++;
    }

    return count;
}

/*!
 * \brief Remove superfluous white space.
 *
 * Leading and trailing white space is removed from the string 
 * <b>line</b> and internal white space which is more than one character 
 * is reduced to a single space character. White space here means 
 * spaces, tabs, linefeeds, newlines, and formfeeds.
 *
 * \param[in,out] line
 * \return Pointer to <b>line</b>
 */

char *G_squeeze(char *line)
{
    char *f = line, *t = line;
    int l;

    /* skip over space at the beginning of the line. */
    while (isspace(*f))
	f++;

    while (*f)
	if (!isspace(*f))
	    *t++ = *f++;
	else if (*++f)
	    if (!isspace(*f))
		*t++ = ' ';
    *t = '\0';
    l = strlen(line) - 1;
    if (*(line + l) == '\n')
	*(line + l) = '\0';

    return line;
}
