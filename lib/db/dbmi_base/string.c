#include <string.h>
#include <stdlib.h>
#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_init_string(dbString * x)
{
    x->string = "";
    x->nalloc = 0;
}



/*!
   \fn 
   \brief 
   \return 
   \param 
 */
/* db_set_string(dbString *x, char *s, int copy)
 *  inserts 's' into 'x'
 *   if 'copy' is true, then memory is allocated to copy into
 *   else 'x' is made to point to 's'
 * returns DB_OK or DB_MEMORY_ERR
 */
static int set_string(dbString * x, char *s, int copy);

int db_set_string(dbString * x, const char *s)
{
    return set_string(x, (char *)s, 1);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_string_no_copy(dbString * x, char *s)
{
    return set_string(x, s, 0);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
unsigned int db_sizeof_string(dbString * x)
{
    if (x->nalloc < 0)
	return 0;
    return (unsigned int)x->nalloc;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_zero_string(dbString * x)
{
    db_zero((void *)db_get_string(x), db_sizeof_string(x));
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
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
   \fn 
   \brief 
   \return 
   \param 
 */
int db_enlarge_string(dbString * x, int len)
{
    if (x->nalloc < len) {
	if (x->nalloc <= 0)
	    x->string = db_store("");
	x->string = db_realloc((void *)x->string, len);
	if (x->string == NULL)
	    return DB_MEMORY_ERR;
	x->nalloc = len;
    }
    return DB_OK;
}

/*!
   \fn
   \brief
   \return
   \param
 */

char *db_get_string(dbString * x)
{
    return x->string;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_free_string(dbString * x)
{
    if (x->nalloc > 0)
	free(x->string);
    db_init_string(x);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_free_string_array(dbString * a, int n)
{
    int i;

    if (a) {
	for (i = 0; i < n; i++)
	    db_free_string(&a[i]);
	free(a);
    }
}

/*!
   \fn 
   \brief 
   \return 
   \param 
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
   \fn 
   \brief 
   \return 
   \param 
 */
int db_append_string(dbString * x, const char *s)
{
    int len;
    int stat;

    len = strlen(db_get_string(x)) + strlen(s) + 1;
    stat = db_enlarge_string(x, len);
    if (stat != DB_OK)
	return stat;
    strcat(db_get_string(x), s);
    return DB_OK;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_copy_string(dbString * dst, dbString * src)
{
    return db_set_string(dst, db_get_string(src));
}

/*!
   \fn 
   \brief each ' is replaced by ''
   \return 
   \param 
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
