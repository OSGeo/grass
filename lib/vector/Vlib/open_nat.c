/*!
  \file open_nat.c
  
  \brief Vector library - open vector map (native format)
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  Update to GRASS 5.7 Radim Blazek and David D. Gray.
  
  \date 2001
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <grass/Vect.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static char name_buf[GPATH_MAX];
static int check_coor ( struct Map_info *Map );

/**
   \brief Open existing vector map

   Map->name and Map->mapset must be set before.

   \param Map poiter to vector map
   \param update non-zero for write mode, otherwise read-only

   \return 0 success
   \return -1 error
*/
int 
V1_open_old_nat ( struct Map_info *Map, int update)
{
  char buf[1000];

  G_debug (1, "V1_open_old_nat(): name = %s mapset = %s", Map->name, Map->mapset);
  
  sprintf (buf, "%s/%s", GRASS_VECT_DIRECTORY, Map->name);
  dig_file_init ( &(Map->dig_fp) );
  if ( update )
      Map->dig_fp.file = G_fopen_modify (buf, GRASS_VECT_COOR_ELEMENT);
  else
      Map->dig_fp.file = G_fopen_old (buf, GRASS_VECT_COOR_ELEMENT, Map->mapset);

  if ( Map->dig_fp.file == NULL ) return -1;

  if ( !(dig__read_head (Map)) ) return (-1);
  check_coor ( Map );

  /* set conversion matrices */
  dig_init_portable ( &(Map->head.port), Map->head.port.byte_order );

  /* load to memory */
  if ( !update )
      dig_file_load ( &(Map->dig_fp) );

  return (0);
}

/**
   \brief Open/Create new vector map.

   \param Map pointer to vector map
   \param name map name
   \param with_z 2D or 3D (unused?)


   \return 0 success
   \return -1 error 
*/
int 
V1_open_new_nat (
	      struct Map_info *Map,
	      char *name,
	      int with_z)
{
  char buf[1000];
  struct stat info;

  G_debug (1, "V1_open_new_nat(): name = %s", name);

  sprintf (buf, "%s/%s", GRASS_VECT_DIRECTORY, name);

  /* Set the 'coor' file version */
  Map->head.Version_Major = GV_COOR_VER_MAJOR;
  Map->head.Version_Minor = GV_COOR_VER_MINOR;
  Map->head.Back_Major = GV_COOR_EARLIEST_MAJOR;
  Map->head.Back_Minor = GV_COOR_EARLIEST_MINOR;

  /* TODO open better */
  dig_file_init ( &(Map->dig_fp) );
  Map->dig_fp.file = G_fopen_new (buf, GRASS_VECT_COOR_ELEMENT);
  if ( Map->dig_fp.file == NULL ) return (-1);
  fclose ( Map->dig_fp.file );

  dig_file_init ( &(Map->dig_fp) );
  Map->dig_fp.file = G_fopen_modify (buf, GRASS_VECT_COOR_ELEMENT);
  if ( Map->dig_fp.file == NULL ) return (-1);

  /* check to see if dig_plus file exists and if so, remove it */
  G__file_name (name_buf, buf, GV_TOPO_ELEMENT, G_mapset ());
  if (stat (name_buf, &info) == 0)	/* file exists? */
       unlink (name_buf);

  G__file_name (name_buf, buf, GRASS_VECT_COOR_ELEMENT, G_mapset ());

  Map->head.size = 0;
  Map->head.head_size = GV_COOR_HEAD_SIZE;
  Vect__write_head (Map);

  /* set conversion matrices */
  dig_init_portable ( &(Map->head.port), dig__byte_order_out ());

  if ( !(dig__write_head (Map)) ) return (-1);
  
  return 0;
}

/* check file size */
int check_coor ( struct Map_info *Map )
{
    struct Coor_info CInfo;
    long  dif;
  
    Vect_coor_info ( Map, &CInfo);
    dif = CInfo.size - Map->head.size;
    G_debug ( 1, "coor size in head = %ld, real coor file size= %ld", 
	                     Map->head.size, CInfo.size);

    if ( dif > 0 ) {
	G_warning (_("Coor files of vector map <%s@%s> is larger than it should be "
		     "(%ld bytes excess)"), Map->name, Map->mapset, dif);
    } else if ( dif < 0 ) {
        G_warning (_("Coor files of vector <%s@%s> is shorter than it should be "
		     "(%ld bytes missing)."), Map->name, Map->mapset, -dif);
    }
    return 1;
}
