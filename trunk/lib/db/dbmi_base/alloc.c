/*!
  \file lib/db/dbmi_base/alloc.c
  
  \brief DBMI Library (base) - allocate memory
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <string.h>
#include <stdlib.h>
#include <grass/dbmi.h>

/*!
  \brief Make a copy of string buffer
  
  Allocated string buffer should be freed by db_free().

  \param s source string buffer

  \return allocated string buffer
*/
char *db_store(const char *s)
{
    char *a;

    a = db_malloc(strlen(s) + 1);
    if (a)
	strcpy(a, s);
    return a;
}

/*!
  \brief Allocate memory

  On failure is called db_memory_error().
  
  \param n number of bytes to be allocated

  \return pointer to allocated memory
 */
void *db_malloc(int n)
{
    void *s;

    if (n <= 0)
	n = 1;
    s = malloc((unsigned int)n);
    if (s == NULL)
	db_memory_error();
    return s;
}

/*!
  \brief Allocate memory

  On failure is called db_memory_error().
  
  \param n number of entities
  \param m entity size

  \return pointer to allocated memmory
 */
void *db_calloc(int n, int m)
{
    void *s;

    if (n <= 0)
	n = 1;
    if (m <= 0)
	m = 1;
    s = calloc((unsigned int)n, (unsigned int)m);
    if (s == NULL)
	db_memory_error();
    return s;
}

/*!
  \brief Reallocate memory

  On failure is called db_memory_error().
  
  \param s pointer to memory
  \param n number of newly allocated bytes

  \return pointer to allocated memmory
 */
void *db_realloc(void *s, int n)
{
    if (n <= 0)
	n = 1;
    if (s == NULL)
	s = malloc((unsigned int)n);
    else
	s = realloc(s, (unsigned int)n);
    if (s == NULL)
	db_memory_error();
    return s;
}

/*!
  \brief Free allocated memory
  
  \param s pointer to memory to be freed
*/
void db_free(void *s)
{
    free(s);
}
