/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <sys/types.h>
#include <string.h>
#include <grass/vector.h>

int dig__write_head(struct Map_info *Map)
{
    unsigned char buf[10];
    long length = GV_COOR_HEAD_SIZE;

    G_debug(1, "dig__write_head()");

    dig_set_cur_port(&(Map->head.port));
    dig_fseek(&(Map->dig_fp), 0L, 0);

    /* bytes 1 - 5 */
    buf[0] = Map->head.coor_version.major;
    buf[1] = Map->head.coor_version.minor;
    buf[2] = Map->head.coor_version.back_major;
    buf[3] = Map->head.coor_version.back_minor;

    buf[4] = Map->head.port.byte_order;
    if (0 >= dig__fwrite_port_C((char *)buf, 5, &(Map->dig_fp)))
	return (0);

    /* increase header size for new vectors, already set in V1_open_new_nat() */
    length = Map->head.head_size;

    /* bytes 6 - 9 : header size */
    if (0 >= dig__fwrite_port_L(&length, 1, &(Map->dig_fp)))
	return (0);

    /* byte 10 : dimension 2D or 3D */
    buf[0] = Map->head.with_z;
    if (0 >= dig__fwrite_port_C((char *)buf, 1, &(Map->dig_fp)))
	return (0);

    /* bytes 11 - 18 : size of coordinate file */
    G_debug(1, "write coor size (%"PRI_OFF_T") to head", Map->head.size);
    if (Map->head.head_size >= GV_COOR_HEAD_SIZE + 4) {
	if (Map->head.size > PORT_LONG_MAX) {
	    /* can only happen when sizeof(off_t) == 8 */
	    if (0 >= dig__fwrite_port_O(&(Map->head.size), 1, &(Map->dig_fp), sizeof(off_t)))
		return (0);
	}
	else {
	    /* write twice to fill the space and set offset (account for sizeof(off_t) == 4) */
	    if (0 >= dig__fwrite_port_O(&(Map->head.size), 1, &(Map->dig_fp), 4))
		return (0);
	    if (0 >= dig__fwrite_port_O(&(Map->head.size), 1, &(Map->dig_fp), 4))
		return (0);
	}
    }
    else {
	/* old vector with shorter coor head size got modified */
	/* bytes 11 - 14 : size of coordinate file */
	if (0 >= dig__fwrite_port_O(&(Map->head.size), 1, &(Map->dig_fp), 4))
	    return (0);
    }

    G_debug(2, "coor body offset %"PRI_OFF_T, dig_ftell(&(Map->dig_fp)));
    return 1;
}


int dig__read_head(struct Map_info *Map)
{
    unsigned char buf[10];
    struct Port_info port;

    dig_fseek(&(Map->dig_fp), 0L, 0);

    /* bytes 1 - 5 */
    if (0 >= dig__fread_port_C((char *)buf, 5, &(Map->dig_fp)))
	return (0);
    Map->head.coor_version.major = buf[0];
    Map->head.coor_version.minor = buf[1];
    Map->head.coor_version.back_major = buf[2];
    Map->head.coor_version.back_minor = buf[3];
    Map->head.port.byte_order = buf[4];

    G_debug(2,
	    "Coor header: file version %d.%d , supported from GRASS version %d.%d",
	    Map->head.coor_version.major, Map->head.coor_version.minor,
	    Map->head.coor_version.back_major, Map->head.coor_version.back_minor);

    G_debug(2, "  byte order %d", Map->head.port.byte_order);

    /* check version numbers */
    if (Map->head.coor_version.major > GV_COOR_VER_MAJOR ||
	Map->head.coor_version.minor > GV_COOR_VER_MINOR) {
	/* The file was created by GRASS library with higher version than this one */

	if (Map->head.coor_version.back_major > GV_COOR_VER_MAJOR ||
	    Map->head.coor_version.back_minor > GV_COOR_VER_MINOR) {
	    /* This version of GRASS lib is lower than the oldest which can read this format */
	    G_fatal_error
		("Vector 'coor' format version %d.%d is not supported by this version of GRASS. "
		 "Update your GRASS.", Map->head.coor_version.major,
		 Map->head.coor_version.minor);
	    return (-1);
	}

	G_warning
	    ("Your GRASS version does not fully support vector format %d.%d."
	     " Consider to upgrade GRASS.", Map->head.coor_version.major,
	     Map->head.coor_version.minor);
    }

    dig_init_portable(&port, Map->head.port.byte_order);
    dig_set_cur_port(&port);

    /* bytes 6 - 9 : header size */
    if (0 >= dig__fread_port_L(&(Map->head.head_size), 1, &(Map->dig_fp)))
	return (0);
    G_debug(2, "  header size %ld", Map->head.head_size);

    /* byte 10 : dimension 2D or 3D */
    if (0 >= dig__fread_port_C((char *)buf, 1, &(Map->dig_fp)))
	return (0);
    Map->head.with_z = buf[0];
    G_debug(2, "  with_z %d", Map->head.with_z);

    /* Map->head.size holds stats value */
    if (Map->head.size > PORT_LONG_MAX && Map->head.head_size >= GV_COOR_HEAD_SIZE + 4) {
	/* bytes 11 - 18 : size of coordinate file */
	if (0 >= dig__fread_port_O(&(Map->head.size), 1, &(Map->dig_fp), sizeof(off_t)))
	    return (0);
    }
    else {
	/* bytes 11 - 14 : size of coordinate file */
	if (0 >= dig__fread_port_O(&(Map->head.size), 1, &(Map->dig_fp), 4))
	    return (0);
    }
    G_debug(2, "  coor size %"PRI_OFF_T, Map->head.size);

    /* Go to end of header, file may be written by new version of GRASS with longer header */

    dig_fseek(&(Map->dig_fp), Map->head.head_size, SEEK_SET);

    return (1);
}
