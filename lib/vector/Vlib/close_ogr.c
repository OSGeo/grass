/*!
   \file close_ogr.c

   \brief Vector library - Close map (OGR)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and Piero Cavalieri.
*/

#include <grass/config.h>
#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>

/*!
  \brief Close vector map (OGR dsn & layer)

  \param Map pointer to Map_info

  \return 0 on success
  \return non-zero on error
*/
int V1_close_ogr(struct Map_info *Map)
{
    int i;

    if (!VECT_OPEN(Map))
	return -1;

    if (Map->format != GV_FORMAT_OGR_DIRECT &&
	(Map->mode == GV_MODE_WRITE || Map->mode == GV_MODE_RW))
	Vect__write_head(Map);

    if (Map->fInfo.ogr.feature_cache)
	OGR_F_Destroy(Map->fInfo.ogr.feature_cache);

    if (Map->fInfo.ogr.driver)
	OGR_DS_Destroy(Map->fInfo.ogr.driver);
    OGR_DS_Destroy(Map->fInfo.ogr.ds);
    
    for (i = 0; i < Map->fInfo.ogr.lines_alloc; i++) {
	Vect_destroy_line_struct(Map->fInfo.ogr.lines[i]);
    }

    G_free(Map->fInfo.ogr.lines);
    G_free(Map->fInfo.ogr.lines_types);

    G_free(Map->fInfo.ogr.driver_name);
    G_free(Map->fInfo.ogr.dsn);
    G_free(Map->fInfo.ogr.layer_name);

    return 0;
}

/*!
  \brief Write OGR specific files (fidx)

  \param Map vector map
  
  \return 0 on success
  \return non-zero on error
*/
int V2_close_ogr(struct Map_info *Map)
{
    char fname[1000], elem[1000];
    char buf[5];
    long length = 9;
    struct gvfile fp;
    struct Port_info port;

    G_debug(3, "V2_close_ogr()");

    if (!VECT_OPEN(Map))
	return -1;

    if (strcmp(Map->mapset, G_mapset()) == 0 && Map->support_updated &&
	Map->plus.built == GV_BUILD_ALL) {
	sprintf(elem, "%s/%s", GV_DIRECTORY, Map->name);
	G__file_name(fname, elem, "fidx", Map->mapset);
	G_debug(4, "Open fidx: %s", fname);
	dig_file_init(&fp);
	fp.file = fopen(fname, "w");
	if (fp.file == NULL) {
	    G_warning(_("Unable to open fidx file for write <%s>"), fname);
	    return 1;
	}

	dig_init_portable(&port, dig__byte_order_out());
	dig_set_cur_port(&port);

	/* Header */
	/* bytes 1 - 5 */
	buf[0] = 5;
	buf[1] = 0;
	buf[2] = 5;
	buf[3] = 0;
	buf[4] = (char)dig__byte_order_out();
	if (0 >= dig__fwrite_port_C(buf, 5, &fp))
	    return (1);

	/* bytes 6 - 9 : header size */
	if (0 >= dig__fwrite_port_L(&length, 1, &fp))
	    return (1);

	/* Body */
	/* number of records  */
	if (0 >= dig__fwrite_port_I(&(Map->fInfo.ogr.offset_num), 1, &fp))
	    return (1);

	/* offsets */
	if (0 >= dig__fwrite_port_I(Map->fInfo.ogr.offset,
				    Map->fInfo.ogr.offset_num, &fp))
	    return (1);

	fclose(fp.file);

    }

    G_free(Map->fInfo.ogr.offset);

    return 0;
}
#endif
