/*!
   \file lib/gis/key_value1.c

   \brief Subroutines for Key/Value management.

   (C) 2001-2008, 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author CERL
 */

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>

/*!
   \brief Allocate and initialize Key_Value structure

   \return pointer to allocated Key_Value structure
 */
struct Key_Value *G_create_key_value(void)
{
    struct Key_Value *kv = G_malloc(sizeof(struct Key_Value));
    G_zero(kv, sizeof(struct Key_Value));
    
    return kv;
}

/*!
   \brief Set value for given key

   \param key key to be set up
   \param value value for given key
   \param[in,out] kv Key_value structure to be modified
 */
void G_set_key_value(const char *key, const char *value, struct Key_Value *kv)
{
    int n;

    if (!key)
	return;

    for (n = 0; n < kv->nitems; n++)
	if (strcmp(key, kv->key[n]) == 0)
	    break;

    if (n == kv->nitems) {
	if (n >= kv->nalloc) {
	    size_t size;

	    if (kv->nalloc <= 0)
		kv->nalloc = 8;
	    else
		kv->nalloc *= 2;

	    size = kv->nalloc * sizeof(char *);
	    kv->key = G_realloc(kv->key, size);
	    kv->value = G_realloc(kv->value, size);
	}

	kv->key[n] = G_store(key);
	kv->value[n] = G_store(value);
	kv->nitems++;
	return;
    }

    if (kv->value[n])
	G_free(kv->value[n]);

    kv->value[n] = value ? G_store(value) : NULL;
}

/*!
   \brief Find given key (case sensitive)

   \param key key to be found
   \param kv pointer to Key_value structure

   \return pointer to value of key
   \return NULL if no key found
 */
const char *G_find_key_value(const char *key, const struct Key_Value *kv)
{
    int n;

    if (!kv)
	return NULL;
    
    for (n = 0; n < kv->nitems; n++)
	if (strcmp(key, kv->key[n]) == 0)
	    return kv->value[n][0] ? kv->value[n] : NULL;
    
    return NULL;
}

/*!
   \brief Free allocated Key_Value structure

   \param[in] kv Key_Value structure to be freed
 */
void G_free_key_value(struct Key_Value *kv)
{
    int n;

    if (!kv)
	return;

    for (n = 0; n < kv->nitems; n++) {
	G_free(kv->key[n]);
	G_free(kv->value[n]);
    }
    G_free(kv->key);
    G_free(kv->value);
    kv->nitems = 0;		/* just for safe measure */
    kv->nalloc = 0;
    G_free(kv);
}
