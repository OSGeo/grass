/*!
  \file lib/db/dbmi_base/strip.c
  
  \brief DBMI Library (base) - strip strings
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>

/*!
  \brief Strip given string

  'buf' is rewritten in place with leading and trailing white
  space removed.

  \param buf string buffer
*/
void db_strip(char *buf)
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
