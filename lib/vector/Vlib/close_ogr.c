
/*****************************************************************************
*
* MODULE:       Vector library 
*   	    	
* AUTHOR(S):    Radim Blazek, Piero Cavalieri 
*
* PURPOSE:      Higher level functions for reading/writing/manipulating vectors.
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <grass/Vect.h>
#include <stdlib.h>

#ifdef HAVE_OGR
#include <ogr_api.h>

/* 
 ** return 0 on success
 **         non-zero on error
 */
int V1_close_ogr(struct Map_info *Map)
{
    int i;

    if (!VECT_OPEN(Map))
	return -1;

    if (Map->mode == GV_MODE_WRITE || Map->mode == GV_MODE_RW)
	Vect__write_head(Map);

    if (Map->fInfo.ogr.feature_cache)
	OGR_F_Destroy(Map->fInfo.ogr.feature_cache);

    OGR_DS_Destroy(Map->fInfo.ogr.ds);

    for (i = 0; i < Map->fInfo.ogr.lines_alloc; i++) {
	Vect_destroy_line_struct(Map->fInfo.ogr.lines[i]);
    }

    free(Map->fInfo.ogr.lines);
    free(Map->fInfo.ogr.lines_types);

    free(Map->fInfo.ogr.dsn);
    free(Map->fInfo.ogr.layer_name);

    return 0;
}

/* 
 *  Write OGR specific files (fidx)
 * 
 *  return 0 on success
 *         non-zero on error
 */
int V2_close_ogr(struct Map_info *Map)
{
    char fname[1000], elem[1000];
    char buf[5];
    long length = 9;
    GVFILE fp;
    struct Port_info port;

    G_debug(3, "V2_close_ogr()");

    if (!VECT_OPEN(Map))
	return -1;

    if (strcmp(Map->mapset, G_mapset()) == 0 && Map->support_updated &&
	Map->plus.built == GV_BUILD_ALL) {
	sprintf(elem, "%s/%s", GRASS_VECT_DIRECTORY, Map->name);
	G__file_name(fname, elem, "fidx", Map->mapset);
	G_debug(4, "Open fidx: %s", fname);
	dig_file_init(&fp);
	fp.file = fopen(fname, "w");
	if (fp.file == NULL) {
	    G_warning("Can't open fidx file for write: %s\n", fname);
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

    free(Map->fInfo.ogr.offset);

    return 0;
}
#endif
