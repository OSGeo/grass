/*!
   \file lib/gis/key_value2.c

   \brief Read/write Key_Value from/to file.

   (C) 2001-2008, 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author CERL
 */

#include <grass/gis.h>

/*!
   \brief Write key/value pairs to file

   \param[in,out] fd file to write to
   \param         kv pointer Key_Value structure

   \return 0 success
   \return -1 error writing
 */
int G_fwrite_key_value(FILE * fd, const struct Key_Value *kv)
{
    int n;
    int err;

    err = 0;
    for (n = 0; n < kv->nitems; n++)
	if (kv->value[n][0]) {
	    if (EOF == fprintf(fd, "%s: %s\n", kv->key[n], kv->value[n]))
		err = -1;
	}
    return err;
}

/*!
   \brief Read key/values pairs from file

   Allocated memory must be freed G_free_key_value().

   \param fd file to read key/values from

   \return pointer to allocated Key_Value structure
   \return NULL on error
 */
struct Key_Value *G_fread_key_value(FILE * fd)
{
    struct Key_Value *kv;
    char *key, *value;
    char buf[1024];

    kv = G_create_key_value();
    if (kv == NULL)
	return NULL;
    while (G_getl2(buf, sizeof(buf) - 1, fd) != 0) {
	key = value = buf;
	while (*value && *value != ':')
	    value++;
	if (*value != ':')
	    continue;
	*value++ = 0;
	G_strip(key);
	G_strip(value);
	G_set_key_value(key, value, kv);
    }
    return kv;
}
