#include <string.h>
#include <stdlib.h>
#include <grass/dbmi.h>

/*!
   \fn char *db_store (char *s)
   \brief 
   \return 
   \param 
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
   \fn void *db_malloc (int n)
   \brief 
   \return 
   \param 
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
   \fn void *db_calloc (int n, int m)
   \brief 
   \return 
   \param 
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
   \fn void *db_realloc (void *s, int n)
   \brief 
   \return 
   \param 
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
   \fn void *db_free (void *s)
   \brief 
   \return 
   \param 
 */
void *db_free(void *s)
{
    free(s);
}
