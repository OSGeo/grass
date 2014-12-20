/*!
   \file lib/gis/key_value4.c

   \brief Key_Value management.

   (C) 2001-2014 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author CERL
 */

#include <grass/gis.h>
#include <string.h>

/*!
   \brief Update file, set up value for given key

   \param[in] file  filename to be updated
   \param[in] key   key value
   \param[in] value value to be updated
 */
void G_update_key_value_file(const char *file,
			     const char *key, const char *value)
{
    struct Key_Value *kv;

    kv = G_read_key_value_file(file);
    G_set_key_value(key, value, kv);
    G_write_key_value_file(file, kv);
    G_free_key_value(kv);
}

/*!
   \brief Look up for key in file

   \param[in]  file  filename
   \param[in]  key   key to be found in file
   \param[out] value value for key
   \param[in]  n     number of characters to be copied

   \return 0 not found
   \return 1 ok
 */
int G_lookup_key_value_from_file(const char *file,
				 const char *key, char value[], int n)
{
    struct Key_Value *kv;
    const char *v;

    *value = '\0';
    kv = G_read_key_value_file(file);

    v = G_find_key_value(key, kv);

    if (v) {
	strncpy(value, v, n);
	value[n - 1] = '\0';
    }

    G_free_key_value(kv);

    return v ? 1 : 0;
}
