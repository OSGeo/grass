/*!
  \file db/dbmi_base/string.c
  
  \brief DBMI Library (base) - string management
  
  (C) 1999-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC)
  \author Upgraded to GRASS 5.7 by Radim Blazek
*/

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/dbmi.h>

/*!
  \brief Initialize dbString 
  
  \param[out] x pointer to dbString
 */
void db_init_string(dbString *x)
{
    G_zero(x, sizeof(dbString));
}

static int set_string(dbString * x, char *s, int copy);

/*!
  \brief Inserts string to dbString (enlarge string)
  
  \param[in,out] x pointer to dbString
  \param s string to be inserted

  \return DB_OK on success
  \return DB_MEMORY_ERR on error
 */
int db_set_string(dbString * x, const char *s)
{
    return set_string(x, (char *)s, 1);
}

/*!
  \brief Inserts string to dbString (overwrite current value)
  
  \param[in,out] x pointer to dbString
  \param s string to be inserted

  \return DB_OK on success
  \return DB_MEMORY_ERR on error
 */
int db_set_string_no_copy(dbString * x, char *s)
{
    return set_string(x, s, 0);
}

/*!
  \brief Get string size

  \param x pointer to dbString

  \return string size
 */
unsigned int db_sizeof_string(const dbString * x)
{
    if (x->nalloc < 0)
	return 0;
    return (unsigned int)x->nalloc;
}

/*!
  \brief Zero string

  \param x pointer to dbString
 */
void db_zero_string(dbString * x)
{
    if (db_get_string(x) && x->nalloc > 0)
	db_zero((void *)db_get_string(x), x->nalloc);
}

static int set_string(dbString * x, char *s, int copy)
{
    int len;
    int stat;

    if (s == NULL) {
	s = "";
	copy = 1;
    }

    len = strlen(s) + 1;

    if (copy) {
	stat = db_enlarge_string(x, len);
	if (stat != DB_OK)
	    return stat;
	strcpy(x->string, s);
    }
    else {
	db_free_string(x);
	x->string = s;
	x->nalloc = -1;
    }
    return DB_OK;
}

/*!
   \brief Enlarge dbString

   \param x pointer to dbString
   \param len requested string size

   \return DB_OK on success
   \return DB_MEMORY_ERR on error
 */
int db_enlarge_string(dbString * x, int len)
{
    if (x->nalloc < len) {
	if (x->nalloc < 0)
	    x->string = NULL;
	x->string = db_realloc((void *)x->string, len);
	if (x->string == NULL)
	    return DB_MEMORY_ERR;
	x->nalloc = len;
    }
    return DB_OK;
}

/*!
  \brief Get string

  \param x pointer to dbString

  \return pointer to buffer containing string
*/
char *db_get_string(const dbString * x)
{
    return x->string;
}

/*!
  \brief Free allocated space for dbString

  \param x pointer to dbString
*/
void db_free_string(dbString *x)
{
    if (x->nalloc > 0)
	db_free(x->string);
    db_init_string(x);
}

/*!
  \brief Free allocated dbString array

  \param a pointer to 1st dbString in the array
  \param n number of items in array
 */
void db_free_string_array(dbString *a, int n)
{
    int i;

    if (a) {
	for (i = 0; i < n; i++)
	    db_free_string(&a[i]);
	db_free(a);
    }
}

/*!
  \brief Allocate dbString array

  \param count number of items to be allocated

  \return pointer to 1st dbString in the array
*/
dbString *db_alloc_string_array(int count)
{
    int i;
    dbString *a;

    if (count < 0)
	count = 0;
    a = (dbString *) db_calloc(count, sizeof(dbString));
    if (a) {
	for (i = 0; i < count; i++)
	    db_init_string(&a[i]);
    }
    return a;
}

/*!
  \brief Append string to dbString

  \param x pointer to dbString
  \param s string to be appended

  \return DB_OK on success
  \return otherwise error code is returned
 */
int db_append_string(dbString * x, const char *s)
{
    int len;
    int stat;

    if (!db_get_string(x))
	return db_set_string(x, s);

    len = strlen(db_get_string(x)) + strlen(s) + 1;
    stat = db_enlarge_string(x, len);
    if (stat != DB_OK)
	return stat;
    strcat(db_get_string(x), s);
    return DB_OK;
}

/*!
  \brief Copy dbString

  \param dst destination dbString
  \param src source dbString

  \return DB_OK on success
  \return DB_ERR code on error
 */
int db_copy_string(dbString * dst, const dbString * src)
{
    return db_set_string(dst, db_get_string(src));
}

/*!
  \brief Replace each ' is replaced by ''

  \param src pointer to dbString
*/
void db_double_quote_string(dbString * src)
{
    char *ptra, *ptrb, buf[2];
    dbString tmp;

    db_init_string(&tmp);
    buf[1] = 0;

    ptrb = db_get_string(src);
    while ((ptra = strchr(ptrb, '\'')) != NULL) {
	for (; ptrb <= ptra; ptrb++) {
	    buf[0] = ptrb[0];
	    db_append_string(&tmp, buf);
	}
	db_append_string(&tmp, "'");
    }
    db_append_string(&tmp, ptrb);
    db_set_string(src, db_get_string(&tmp));
    db_free_string(&tmp);
}
