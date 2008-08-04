
/**
 * \file squeeze.c
 *
 * \brief GIS Library - String white space removal functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1998-2008
 */

#include <ctype.h>
#include <string.h>
#include <grass/gis.h>


/*
 * last modification: 12 aug 81, j w hamilton
 *
 * 1998-04-04  WBH
 *     Also squeezes out newlines -- easier to use with fgets()
 *
 * 1999-19-12 Werner Droege 
 *     changed line 37, line 48ff. -- return (strip_NL(line))
 */


/**
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
    register char *f = line, *t = line;
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
