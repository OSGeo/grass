/*!
   \file key_value3.c

   \brief Key_Value management.

   (C) 2001-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author CERL
 */

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
   \brief Write key/value pairs to file

   \param[in]  file filename for writing
   \param[in]  kv   Key_Value structure

   \return 0 success
   \return 1 error writing
 */

void G_write_key_value_file(const char *file,
			    const struct Key_Value *kv)
{
    FILE *fp = fopen(file, "w");
    if (!fp)
	G_fatal_error(_("Unable to open output file <%s>"), file);

    if (G_fwrite_key_value(fp, kv) != 0)
	G_fatal_error(_("Error writing file <%s>"), file);

    if (fclose(fp) != 0)
	G_fatal_error(_("Error closing output file <%s>"), file);
}

/*!
   \brief Read key/values pairs from file

   Allocated memory must be freed G_free_key_value().

   \param[in]  file filename for reading
   \param[out] stat status (0 ok, -1 cannot open file, -2 error writing key/value)

   \return poiter to allocated Key_Value structure
   \return NULL on error
 */
struct Key_Value *G_read_key_value_file(const char *file)
{
    FILE *fp;
    struct Key_Value *kv;

    fp = fopen(file, "r");
    if (!fp)
	G_fatal_error(_("Unable to open input file <%s>"), file);

    kv = G_fread_key_value(fp);
    if (!kv)
	G_fatal_error(_("Error reading file <%s>"), file);

    if (fclose(fp) != 0)
	G_fatal_error(_("Error closing input file <%s>"), file);

    return kv;
}
