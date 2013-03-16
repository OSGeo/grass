/*!
  \file lib/vector/Vlib/open_nat.c
  
  \brief Vector library - open vector map (native format) - level 1
  
  Higher level functions for reading/writing/manipulating vectors.
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int check_coor(struct Map_info *Map);

/*!
  \brief Open existing vector map (level 1)
  
  Map->name and Map->mapset must be set before.
  
  \param Map poiter to Map_info structure
  \param update non-zero for write mode, otherwise read-only

  \return 0 success
  \return -1 error
*/
int V1_open_old_nat(struct Map_info *Map, int update)
{
    char buf[1000];
    struct Coor_info CInfo;

    G_debug(1, "V1_open_old_nat(): name = %s mapset = %s", Map->name,
	    Map->mapset);

    sprintf(buf, "%s/%s", GV_DIRECTORY, Map->name);
    dig_file_init(&(Map->dig_fp));
    if (update)
	Map->dig_fp.file = G_fopen_modify(buf, GV_COOR_ELEMENT);
    else
	Map->dig_fp.file =
	    G_fopen_old(buf, GV_COOR_ELEMENT, Map->mapset);

    if (Map->dig_fp.file == NULL) {
        G_warning(_("Unable to open coor file for vector map <%s>"),
		  Vect_get_full_name(Map));
        return -1;
    }

    /* needed to determine file size, Map->head.size will be updated by dig__read_head(Map) */
    Vect_coor_info(Map, &CInfo); 
    Map->head.size = CInfo.size;
    
    if (!(dig__read_head(Map)))
	return (-1);

    /* compare coor size stored in head with real size */
    /* check should catch if LFS is required but not available */
    check_coor(Map);

    /* set conversion matrices */
    dig_init_portable(&(Map->head.port), Map->head.port.byte_order);

    /* load to memory */
    if (!update)
	dig_file_load(&(Map->dig_fp)); /* has currently no effect, file never loaded */

    return 0;
}

/*!
   \brief Create new vector map (level 1)

   \param[out] Map pointer to Map_info structure
   \param name vector map name to be created
   \param with_z 2D or 3D (unused?)

   \return 0 success
   \return -1 error 
*/
int V1_open_new_nat(struct Map_info *Map, const char *name, int with_z)
{
    char *path, file_path[GPATH_MAX];

    G_debug(1, "V1_open_new_nat(): name = %s with_z = %d is_tmp = %d",
            name, with_z, Map->temporary);

    path = Vect__get_path(Map);

    /* Set the 'coor' file version */
    Map->head.Version_Major = GV_COOR_VER_MAJOR;
    Map->head.Version_Minor = GV_COOR_VER_MINOR;
    Map->head.Back_Major    = GV_COOR_EARLIEST_MAJOR;
    Map->head.Back_Minor    = GV_COOR_EARLIEST_MINOR;

    /* TODO: open better */
    dig_file_init(&(Map->dig_fp));
    Map->dig_fp.file = G_fopen_new(path, GV_COOR_ELEMENT);
    if (Map->dig_fp.file == NULL)
	return -1;
    fclose(Map->dig_fp.file);

    dig_file_init(&(Map->dig_fp));
    Map->dig_fp.file = G_fopen_modify(path, GV_COOR_ELEMENT);
    if (Map->dig_fp.file == NULL)
	return -1;

    /* if overwrite OK, any existing files have already been deleted by
     * Vect_open_new(): remove this check ? */
    /* check to see if dig_plus file exists and if so, remove it */
    G_file_name(file_path, path, GV_TOPO_ELEMENT, G_mapset());
    G_free(path);
    if (access(file_path, F_OK) == 0)
        unlink(file_path); /* remove topo file if exists */
    
    /* set conversion matrices */
    dig_init_portable(&(Map->head.port), dig__byte_order_out());

    /* write coor header */
    if (!(dig__write_head(Map)))
	return -1;

    return 0;
}

/* check file size */
int check_coor(struct Map_info *Map)
{
    struct Coor_info CInfo;
    off_t dif;

    /* NOTE: coor file is open */
    Vect_coor_info(Map, &CInfo);
    dif = CInfo.size - Map->head.size;
    G_debug(1, "coor size in head = %lu, real coor file size= %lu",
	    (unsigned long) Map->head.size, (unsigned long) CInfo.size);

    if (dif > 0) {
	G_warning(_("Coor file of vector map <%s@%s> is larger than it should be "
		   "(%ld bytes excess)"), Map->name, Map->mapset, dif);
    }
    else if (dif < 0) {
	G_warning(_("Coor file of vector <%s@%s> is shorter than it should be "
		   "(%ld bytes missing)."), Map->name, Map->mapset, -dif);
    }
    return 1;
}
