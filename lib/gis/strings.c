/*!
 * \file strings.c
 * 
 * \brief string/chring movement functions
 *
 * (C) 1999-2007 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Dave Gerdes (USACERL), Michael Shapiro (USACERL), Amit
 * Parghi (USACERL), Bernhard Reiter (Intevation GmbH, Germany) and
 * many others
 */

/* TODO: merge interesting functions from 
   ../datetime/scan.c
   here
*/

/*
 * string/chring movement functions
 *
** G_strcpy (T, F)
** G_strncpy (T, F, n)	copy F up to null or n, always copy null
** G_chrcpy (T, F, n)
** G_strmov (T, F)
** G_chrmov (T, F, n)
** G_strcat (T, F)
** G_chrcat (T, F, n)
**     char *T, *F;
**     int n;
 *
 * G_strcpy (T, F)    copy F up to null, copy null
 * G_chrcpy (T, F, n) copy F up to n,    copy null
 * 
 * G_strmov (T, F)    copy F up to null
 * G_chrmov (T, F, n) copy F up to n
 * 
 * G_strcat (T, F)    cat F up to null, copy null
 * G_chrcat (T, F, n) cat F up to n,    copy null
 *
 * the -cpy and -cat functions are for null-terminated destinations;
 * the -mov functions are for non-null-terminated ('chring') destinations.
 * all functions return 'T'.
 *
 * Author Dave Gerdes (USACERL)
 *
 *
 * G_strcasecmp(a, b) char *a, *b;
 *   string compare ignoring case (upper or lower)
 *   returns: -1 a<b; 0 a==b; 1 a>b
 *
 * Author Michael Shapiro (USACERL)
 *
 *
 * G_strstr(mainString, subString)
 *	Return a pointer to the first occurrence of subString
 *	in mainString, or NULL if no occurrences are found.
 * G_strdup(string)
 *	Return a pointer to a string that is a duplicate of the string
 *	given to G_strdup.  The duplicate is created using malloc.
 *	If unable to allocate the required space, NULL is returned.
 *
 * Author: Amit Parghi (USACERL), 1993 02 23
 *
 * G_strchg(char* bug, char character, char new) {
 *      replace all occurencies of character in string(inplace) with new
 *
 * Author: Bernhard Reiter (Intevation GmbH, Germany)
 *
 * char * G_str_replace(char* buffer, char* old_str, char* new_str)
 *  Replace all occurencies of old_str in buffer with new_str
 *  Author Beverly Wallace (LMCO)
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <grass/gis.h>

#ifndef NULL
#define NULL		0
#endif

static char *G_strend (const char *S)
{
    while (*S)
	S++;
    return (char *) S;
}

/*!
 * \fn char *G_strcpy (char *T, const char *F)
 *
 * \brief Copies characters from the string F into the string T.
 *
 * This function has undefined results if the strings overlap.
 *
 * \return T value
 *
 * \param[out] T target string 
 * \param[in] F source string
 */
char *G_strcpy (char *T, const char *F)
{
    char *d = T;

    while ((*d++ = *F++))
        ;
    return (T);
}

/*!
 * \fn char *G_chrcpy (char *T, const char *F, int n)
 *
 * \brief Copies characters from the string F into the string T.
 * 
 * Copies just the first n characters from the string F. At the end
 * the null terminator is written into the string T.
 *
 *
 * \return T value 
 *
 * \param[out] T target string
 * \param[in] F source string
 * \param[in] n number of characters to copy
 */
char *G_chrcpy (char *T, const char *F, int n)
{
    char *d = T;

    while (n--)
        *d++ = *F++;
    *d = '\0';
    return (T);
}

/*!
 * \fn char *G_strncpy (char *T, const char *F, int n)
 *
 * \brief This function is similar to G_chrcpy() but always copies at least
 * n characters into the string T.
 * 
 * If the length of F is more than n, then copies just the first n
 * characters. At the end the null terminator is written into the
 * string T.
 *
 * \return T value
 *
 * \param[out] T target string
 * \param[in] F source string
 * \param[in] n number of characters to copy
 */
char *G_strncpy (char *T, const char *F, int n)
{
    char *d = T;

    while (n-- && *F)
        *d++ = *F++;
    *d = '\0';
    return (T);
}

/*!
 * \fn char *G_strmov (char *T, const char *F)
 *
 * \brief Copies characters from the string F (not including the
 * terminating null character) into the string T.
 *
 * \return T value
 *
 * \param[out] T target string
 * \param[in] F source string
 */
char *G_strmov (char *T, const char *F)
{
    char *d = T;

    while (*F)
        *d++ = *F++;
    return (T);
}

/*!
 * \fn char *G_chrmov (char *T, const char *F, int n)
 *
 * \brief This copies characters from the string F (exactly n
 * characters) into the string T.
 *
 * The terminating null character is not explicitly written into the
 * string T.
 *
 * \return T value
 *
 * \param[out] T target string
 * \param[in] F source string
 * \param[in] n number of characters to copy
 */
char *G_chrmov (char *T, const char *F, int n)
{
    char *d = T;

    while (n--)
        *d++ = *F++;
    return (T);
}

/*!
 * \fn char *G_strcat (char *T, const char *F)
 *
 * \brief This copies characters from the string F into the string T.
 *
 * This function is similar to G_strcpy(), except that the
 * characters from F are concatenated or appended to the end of
 * T, instead of overwriting it.  That is, the first character from
 * F overwrites the null character marking the end of T.
 *
 * \return T value
 *
 * \param[out] T target string
 * \param[in] F source string
 */
char *G_strcat (char *T, const char *F)
{
    G_strcpy (G_strend (T), F);
    return (T);
}

/*!
 * \fn char *G_chrcat (char *T, const char *F, int n)
 *
 * \brief This function is like G_strcat() except that not more than n
 * characters from F are appended to the end of T.
 *
 * This function is similar to G_strcpy(), except that the
 * characters from F are concatenated or appended to the end of
 * T, instead of overwriting it.  That is, the first character from
 * F overwrites the null character marking the end of T.
 *
 * \return T value
 *
 * \param[out] T target string
 * \param[in] F source string
 * \param[in] n number of character to copy
 */
char *G_chrcat (char *T, const char *F, int n)
{
    G_chrcpy (G_strend (T), F, n);
    return (T);
}

/*!
 * \fn int G_strcasecmp(const char *x, const char *y)
 *
 * \brief String compare ignoring case (upper or lower)
 *
 * Returning a value that has the same sign as the difference between
 * the first differing pair of characters
 *
 * \return 0 the two strings are equal
 * \return -1, 1
 *
 * \param[in] x first string to compare
 * \param[in] y second string to compare
 */
int G_strcasecmp(const char *x, const char *y)
{
    int xx,yy;

    if (!x)
	return y ? -1 : 0;
    if (!y)
	return x ? 1 : 0;
    while (*x && *y)
    {
	xx = *x++;
	yy = *y++;
	if (xx >= 'A' && xx <= 'Z')
	    xx = xx + 'a' - 'A';
	if (yy >= 'A' && yy <= 'Z')
	    yy = yy + 'a' - 'A';
	if (xx < yy) return -1;
	if (xx > yy) return 1;
    }
    if (*x) return 1;
    if (*y) return -1;
    return 0;
}

/*!
 * \fn char *G_strstr(char *mainString, const char *subString)
 *
 * \brief Finds the first occurrence of the character C in the
 * null-terminated string beginning at mainString
 *
 * \return a pointer to the first occurrence of subString in
 * mainString
 * \return NULL if no occurrences are found
 *
 * \param[in] mainString string where to find sub-string
 * \param[in] subString sub-string
 */
char *G_strstr(const char *mainString, const char *subString)
{
    const char *p;
    const char *q;
    int length;

    p = subString;
    q = mainString;
    length = strlen(subString);

    do {
	while (*q != '\0' && *q != *p) {	/* match 1st subString char */
	    q++;
	}
    } while (*q != '\0' && strncmp(p, q, length) != 0 && q++);
				/* Short-circuit evaluation is your friend */

    if (*q == '\0') {				/* ran off end of mainString */
	return NULL;
    } else {
	return (char *) q;
    }
}

/*!
 * \fn char *G_strdup(const char *string)
 *
 * \brief Copies the null-terminated string into a newly
 * allocated string. The string is allocated using G_malloc().
 *
 * \return pointer to a string that is a duplicate of the string
 *  given to G_strdup().
 * \return NULL if unable to allocate the required space
 *
 * \param[in] string the string to duplicate
 */
char *G_strdup(const char *string)
{
    char *p;

    p = G_malloc (strlen(string) + 1);

    if (p != NULL) {
	strcpy(p, string);
    }

    return p;
}

/*!
 * \fn char *G_strchg(char* bug, char character, char new)
 *
 * \brief Replace all occurencies of character in string bug with new
 *
 * \return bug string
 *
 * \param[in,out] bug base string
 * \param[in] character character to replace
 * \param[in] new new character
 */
char *G_strchg(char* bug, char character, char new)
{
 char *help = bug;
 while(*help) {
	if (*help==character)
		*help=new;
	help++;
	}
 return bug;
}

/*!
 * \fn char *G_str_replace(char* buffer, const char* old_str, const char* new_str) 
 *
 * \brief Replace all occurencies of old_str in buffer with new_str
 *
 * Code example:
 * \verbatim
 * char *name;
 * name = G_str_replace ( inbuf, ".exe", "" );
 * ... 
 * G_free (name);
 * \endverbatim
 *
 * \return the newly allocated string, input buffer is unchanged 
 *
 * \param[in,out] buffer main string
 * \param[in] old_str string to replace
 * \param[in] new_str new string
 */
char *G_str_replace(char* buffer, const char* old_str, const char* new_str) 
{

	char *B, *R;
	const char *N;
	char *replace;
	int count, len;

	/* Make sure old_str and new_str are not NULL */
	if (old_str == NULL || new_str == NULL)
		return G_strdup (buffer);
	/* Make sure buffer is not NULL */
	if (buffer == NULL)
		return NULL;

	/* Make sure old_str occurs */
  	B = strstr (buffer, old_str);
	if (B == NULL)
		/* return NULL; */
		return G_strdup (buffer);

	if (strlen (new_str) > strlen (old_str)) {
		/* Count occurences of old_str */
		count = 0;
		len = strlen (old_str);
		B = buffer;
		while(B != NULL && *B != '\0') {
  			B = G_strstr (B, old_str);
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
	replace = G_malloc (len + 1);
	if (replace == NULL)
		return NULL;

	/* Replace old_str with new_str */
	B = buffer;
	R = replace;
	len = strlen (old_str);
	while(*B != '\0') {
		if (*B == old_str[0] && strncmp (B, old_str, len) == 0) {
			N = new_str;
			while (*N != '\0')
				*R++ = *N++;
			B += len;
		}
		else {
			*R++ = *B++;
		}
	}
	*R='\0';

	return replace;
}

/*!
 * \fn int G_strip(register char *buf)
 *
 * \brief Removes all leading and trailing white space from string.
 *
 * \return 0
 *
 * \param[in,out] buf buffer to be worked on
 *
 */
int G_strip (char *buf)
{
    register char *a, *b;

/* remove leading white space */
    for (a = b = buf; *a == ' ' || *a == '\t'; a++)
	    ;
    if (a != b)
	while ((*b++ = *a++))
	    ;
/* remove trailing white space */
    for (a = buf; *a; a++)
	    ;
    if (a != buf)
    {
	for (a--; *a == ' ' || *a == '\t'; a--)
		;
	a++;
	*a = 0;
    }

    return 0;
}

/*!
 * \fn char *G_chop (char *line)
 *
 * \brief Chop leading and trailing white spaces:
 * \verbatim space, \f, \n, \r, \t, \v \endverbatim
 *
 * \return pointer to string
 *
 * \param line buffer to be worked on
 */
/*
 * G_chop - chop leading and trailing white spaces: 
 *          space, \f, \n, \r, \t, \v
 *        - returns pointer to string
 *    
 * char *G_chop (char *s)
 *
 * modified copy of G_squeeze();    RB March 2000
 *                          <Radim.Blazek@dhv.cz>
 *
 */
char *G_chop (char *line)
{
    register char *f = line, *t = line;

    while (isspace (*f))  	/* go to first non white-space char */
        f++;

    if (! *f)			/* no more chars in string */
    {
        *t = '\0';
	return (line);
    }

    for (t = line; *t; t++)  	/* go to end */
        ;
    while ( isspace (*--t) )	
	;
    *++t = '\0';  		/* remove trailing white-spaces */

    t = line;
    while (*f)			/* copy */   
            *t++ = *f++;
    *t = '\0';

    return (line);
}

/*!
 * \fn void G_str_to_upper (char *str)
 *
 * \brief Convert string to upper case
 *
 * \param[in,out] str pointer to string
 */
void G_str_to_upper (char *str)
{
    int i = 0;

    if (!str) return;

    while ( str[i] )
    {
        str[i] = toupper(str[i]);
        i++;
    }
}

/*!
 * \fn void G_str_to_lower (char *str)
 *
 * \brief Convert string to lower case
 *
 * \param[in,out] str pointer to string
 */
void G_str_to_lower (char *str)
{
    int i = 0;

    if (!str) return;

    while ( str[i] )
    {
        str[i] = tolower(str[i]);
        i++;
    }
}

/*!
 * \fn int G_str_to_sql (char *str)
 *
 * \brief Make string SQL compliant
 *
 * \return number of changed characters
 *
 * \param[in,out] str pointer to string
 */
int G_str_to_sql (char *str)
{
    int count;
    char *c;
    
    count = 0;

    if (!str) return 0;

    c = str;
    while ( *c )
    {
	*c = toascii(*c);
	
	if ( !( *c>='A' && *c<='Z' ) && !( *c>='a' && *c<='z' ) && !( *c>='0' && *c<='9' ) ) {
	    *c = '_';
	    count++;
	}
	c++;
    }
    
    c = str;
    if ( !( *c>='A' && *c<='Z' ) && !( *c>='a' && *c<='z' ) ) {
	*c = 'x';
	count++;
    }

    return count;
}
