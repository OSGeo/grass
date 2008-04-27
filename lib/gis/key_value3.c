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

/*!
  \brief Write key/value pairs to file

  \param[in]  file filename for writing
  \param[in]  kv   Key_Value structure
  \param[out] stat status (0 ok, -3 cannot open file, -4 error writing key/value)

  \return 0 success
  \return 1 error writing
*/

int G_write_key_value_file (
    const char *file,
    const struct Key_Value *kv,
    int *stat)
{
    FILE *fd;

    *stat = 0;
    fd = fopen(file, "w");
    if (fd == NULL)
	*stat = -3;
    else if(G_fwrite_key_value(fd, kv) != 0 || fclose(fd) == EOF)
	*stat = -4;
    return (*stat != 0);
}

/*!
  \brief Read key/values pairs from file

  Allocated memory must be freed G_free_key_value().

  \param[in]  file filename for reading
  \param[out] stat status (0 ok, -1 cannot open file, -2 error writing key/value)

  \return poiter to allocated Key_Value structure
  \return NULL on error
*/
struct Key_Value *G_read_key_value_file(const char *file, int *stat)
{
    FILE *fd;
    struct Key_Value *kv;

    *stat = 0;
    fd = fopen (file, "r");
    if (fd == NULL)
    {
	*stat = -1;
	return NULL;
    }
    kv = G_fread_key_value (fd);
    fclose (fd);
    if (kv == NULL)
	*stat = -2;
    return kv;
}
