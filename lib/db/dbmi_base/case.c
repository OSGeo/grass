/*!
  \file lib/db/dbmi_base/case.c
  
  \brief DBMI Library (base) - case string conversion
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>

/*!
  \brief Convert character to lowercase

  \param s character to be modified
*/
void db_char_to_lowercase(char *s)
{
    if (*s >= 'A' && *s <= 'Z')
	*s -= 'A' - 'a';
}

/*!
  \brief Convert character to uppercase

  \param s character to be modified
 */
void db_char_to_uppercase(char *s)
{
    if (*s >= 'a' && *s <= 'z')
	*s += 'A' - 'a';
}

/*!
  \brief Convert string to lowercase

  \param s string buffer to be modified
*/
void db_Cstring_to_lowercase(char *s)
{
    while (*s)
	db_char_to_lowercase(s++);
}

/*!
  \brief Convert string to lowercase

  \param s string buffer to be modified
*/
void db_Cstring_to_uppercase(char *s)
{
    while (*s)
	db_char_to_uppercase(s++);
}

/*!
  \brief Compare strings case-insensitive

  \param a,b string buffers to be compared

  \return 0 strings equal
  \return 1 otherwise
 */
int db_nocase_compare(const char *a, const char *b)
{
    char s, t;

    while (*a && *b) {
	s = *a++;
	t = *b++;
	db_char_to_uppercase(&s);
	db_char_to_uppercase(&t);
	if (s != t)
	    return 0;
    }
    return (*a == 0 && *b == 0);
}
