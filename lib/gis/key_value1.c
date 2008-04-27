/*!
   \file key_value1.c

   \brief Key_Value management.

   (C) 2001-2008 by the GRASS Development Team
   
   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.
   
   \author CERL
*/

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>

/*!
  \brief Allocate and initialize Key_Value structure

  \return poiter to allocated Key_Value structure
*/
struct Key_Value *
G_create_key_value(void)
{
    struct Key_Value *kv;

    kv = (struct Key_Value *) G_malloc (sizeof(struct Key_Value));
    if (kv == NULL)
	return NULL;

    kv->nitems = 0;
    kv->nalloc = 0;
    kv->key = (char **) NULL;
    kv->value = (char **) NULL;

    return kv;
}

/*!
  \brief Set value for given key

  If key has spaces in it, this will break the logic 
  so rule is: NO SPACES IN key.

  \param[in] key key to be set up
  \param[in] value value for given key
  \param[in,out] kv Key_value structure to be modified

  \return 0 no memory
  \return 1 ok, but key was NULL or "" so ignored
  \return 2 ok
*/
int G_set_key_value (
    const char *key, const char *value,
    struct Key_Value *kv)
{
    int n;
    int size;

    if (key == NULL || key == 0)
	return 1;

    for (n = 0; n < kv->nitems; n++)
	if (strcmp(key, kv->key[n]) == 0)
	    break;
    
    if (n == kv->nitems)
    {
	if (n >= kv->nalloc)
	{
	    if (kv->nalloc <= 0)
	    {
		kv->nalloc = 8;
		size = kv->nalloc * sizeof (char *);
		kv->key = (char **) G_malloc (size);
		kv->value = (char **) G_malloc (size);
	    }
	    else
	    {
		kv->nalloc *= 2;
		size = kv->nalloc * sizeof (char *);
		kv->key = (char **) G_realloc (kv->key, size);
		kv->value = (char **) G_realloc (kv->value, size);
	    }

	    if (kv->key == NULL || kv->value == NULL)
	    {
		if (kv->key)
		{
		    G_free (kv->key);
		    kv->key = NULL;
		}
		if (kv->value)
		{
		    G_free (kv->value);
		    kv->value = NULL;
		}
		kv->nitems = kv->nalloc = 0;
		return 0;
	    }
	}
	kv->value[n] = NULL;
	kv->key[n] = G_malloc (strlen(key)+1);
	if (kv->key[n] == NULL)
	    return 0;
	strcpy (kv->key[n], key);
	kv->nitems++;
    }
    if (value == NULL)
	size = 0;
    else
	size = strlen(value);
    if (kv->value[n] != NULL)
	G_free (kv->value[n]);
    if (size > 0)
    {
	kv->value[n] = G_malloc (size+1);
	if (kv->value[n] == NULL)
	    return 0;
	strcpy (kv->value[n], value);
    }
    else
	kv->value[n] = NULL;
    return 2;
}

/*!
  \brief Find given key

  \param[in] key key to be found
  \param[in] kv Key_value structure

  \return poiter to value of key
  \return NULL if no key found
*/
char *G_find_key_value (const char *key, const struct Key_Value *kv)
{
    int n;

    for (n = 0; n < kv->nitems; n++)
	if (strcmp (key, kv->key[n]) == 0)
	    return kv->value[n][0] ? kv->value[n] : NULL;
    return NULL;
}

/*!
  \brief Free allocated Key_Value structure

  \param[in] kv Key_Value structure

  \return 0
*/
int G_free_key_value(struct Key_Value *kv)
{
    int n;

    for (n = 0; n < kv->nitems; n++)
    {
	G_free (kv->key[n]);
	G_free (kv->value[n]);
    }
    G_free (kv->key);
    G_free (kv->value);
    kv->nitems = 0; /* just for safe measure */
    kv->nalloc = 0;
    G_free (kv);

    return 0;
}
