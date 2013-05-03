
/****************************************************************************
*
* MODULE:       Vector library 
*   	    	
* AUTHOR(S):    Radim Blazek.
*
* PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/version.h>

int dig_write_cidx_head(struct gvfile * fp, struct Plus_head *plus)
{
    int i;
    unsigned char buf[5];
    long length = 9;

    G_debug(3, "dig_write_cidx_head()");

    dig_rewind(fp);
    dig_set_cur_port(&(plus->cidx_port));

    /* Head of header */
    /* bytes 1 - 5 */
    buf[0] = GV_CIDX_VER_MAJOR;
    buf[1] = GV_CIDX_VER_MINOR;
    buf[2] = GV_CIDX_EARLIEST_MAJOR;
    buf[3] = GV_CIDX_EARLIEST_MINOR;
    buf[4] = plus->cidx_port.byte_order;
    if (0 >= dig__fwrite_port_C((const char *)buf, 5, fp))
	return (-1);

    /* get required offset size */
    if (plus->off_t_size == 0) {
	/* should not happen, topo is written first */
	if (plus->coor_size > (off_t)PORT_LONG_MAX)
	    plus->off_t_size = 8;
	else
	    plus->off_t_size = 4;
    }

    /* bytes 6 - 9 : header size */
    if (0 >= dig__fwrite_port_L(&length, 1, fp))
	return (0);

    /* Body of header - info about all fields */
    /* Number of fields */
    if (0 >= dig__fwrite_port_I(&(plus->n_cidx), 1, fp))
	return (-1);

    for (i = 0; i < plus->n_cidx; i++) {
	int t;
	struct Cat_index *ci;

	ci = &(plus->cidx[i]);

	G_debug(3, "cidx %d head offset: %"PRI_OFF_T, i, dig_ftell(fp));

	/* Field number */
	if (0 >= dig__fwrite_port_I(&(ci->field), 1, fp))
	    return (-1);

	/* Number of categories */
	if (0 >= dig__fwrite_port_I(&(ci->n_cats), 1, fp))
	    return (-1);

	/* Number of unique categories */
	if (0 >= dig__fwrite_port_I(&(ci->n_ucats), 1, fp))
	    return (-1);

	/* Number of types */
	if (0 >= dig__fwrite_port_I(&(ci->n_types), 1, fp))
	    return (-1);

	/* Types */
	for (t = 0; t < ci->n_types; t++) {
	    int wtype;

	    /* type */
	    wtype = dig_type_to_store(ci->type[t][0]);
	    if (0 >= dig__fwrite_port_I(&wtype, 1, fp))
		return (-1);

	    /* number of items */
	    if (0 >= dig__fwrite_port_I(&(ci->type[t][1]), 1, fp))
		return (-1);

	}

	/* Offset */
	if (0 >= dig__fwrite_port_O(&(ci->offset), 1, fp, plus->off_t_size))
	    return (0);
	G_debug(3, "cidx %d offset: %"PRI_OFF_T, i, ci->offset);
    }

    G_debug(3, "cidx body offset %"PRI_OFF_T, dig_ftell(fp));

    return (0);
}

/*!
  \brief Read header of cidx file
  
  \param fp pointer to gvfile structure
  \param plus pointer to Plus_head strcuture

  \return 0 OK
  \return -1 error
*/
int dig_read_cidx_head(struct gvfile * fp, struct Plus_head *plus)
{
    unsigned char buf[5];
    int i, byte_order;

    dig_rewind(fp);

    /* bytes 1 - 5 */
    if (0 >= dig__fread_port_C((char *)buf, 5, fp))
	return (-1);
    plus->version.cidx.major = buf[0];
    plus->version.cidx.minor = buf[1];
    plus->version.cidx.back_major = buf[2];
    plus->version.cidx.back_minor = buf[3];
    byte_order = buf[4];

    G_debug(3,
	    "Cidx header: file version %d.%d , supported from GRASS version %d.%d",
	    plus->version.cidx.major, plus->version.cidx.minor,
	    plus->version.cidx.back_major, plus->version.cidx.back_minor);

    G_debug(3, "  byte order %d", byte_order);

    /* check version numbers */
    if (plus->version.cidx.major > GV_CIDX_VER_MAJOR ||
	plus->version.cidx.minor > GV_CIDX_VER_MINOR) {
	/* The file was created by GRASS library with higher version than this one */

	if (plus->version.cidx.back_major > GV_CIDX_VER_MAJOR ||
	    plus->version.cidx.back_minor > GV_CIDX_VER_MINOR) {
	    /* This version of GRASS lib is lower than the oldest which can read this format */
	    G_debug(1, "Category index format version %d.%d",
		    plus->version.cidx.major, plus->version.cidx.minor);
	    G_fatal_error
		(_("This version of GRASS (%d.%d) is too old to read this category index format."
		 " Try to rebuild topology or upgrade GRASS to at least version %d."),
		 GRASS_VERSION_MAJOR, GRASS_VERSION_MINOR, GRASS_VERSION_MAJOR + 1);
	    return (-1);
	}

	G_warning
	    ("Your GRASS version does not fully support category index format %d.%d of the vector."
	     " Consider to rebuild topology or upgrade GRASS.",
	     plus->version.cidx.major, plus->version.cidx.minor);
    }

    dig_init_portable(&(plus->cidx_port), byte_order);
    dig_set_cur_port(&(plus->cidx_port));

    /* bytes 6 - 9 : header size */
    if (0 >= dig__fread_port_L(&(plus->cidx_head_size), 1, fp))
	return (-1);
    G_debug(3, "  header size %ld", plus->cidx_head_size);

    /* get required offset size */
    if (plus->off_t_size == 0) {
	/* should not happen, topo is opened first */
	if (plus->coor_size > (off_t)PORT_LONG_MAX)
	    plus->off_t_size = 8;
	else
	    plus->off_t_size = 4;
    }

    /* Body of header - info about all fields */
    /* Number of fields */
    if (0 >= dig__fread_port_I(&(plus->n_cidx), 1, fp))
	return (-1);

    /* alloc space */
    if (plus->a_cidx < plus->n_cidx) {
	plus->a_cidx = plus->n_cidx;
	plus->cidx =
	    (struct Cat_index *)G_realloc(plus->cidx, plus->a_cidx * sizeof(struct Cat_index));
    }

    for (i = 0; i < plus->n_cidx; i++) {
	int t;
	struct Cat_index *ci;

	ci = &(plus->cidx[i]);
	ci->cat = NULL;
	ci->a_cats = 0;

	/* Field number */
	if (0 >= dig__fread_port_I(&(ci->field), 1, fp))
	    return (-1);

	/* Number of categories */
	if (0 >= dig__fread_port_I(&(ci->n_cats), 1, fp))
	    return (-1);

	/* Number of unique categories */
	if (0 >= dig__fread_port_I(&(ci->n_ucats), 1, fp))
	    return (-1);

	/* Number of types */
	if (0 >= dig__fread_port_I(&(ci->n_types), 1, fp))
	    return (-1);

	/* Types */
	for (t = 0; t < ci->n_types; t++) {
	    int rtype;

	    /* type */
	    if (0 >= dig__fread_port_I(&rtype, 1, fp))
		return (-1);
	    ci->type[t][0] = dig_type_from_store(rtype);

	    /* number of items */
	    if (0 >= dig__fread_port_I(&(ci->type[t][1]), 1, fp))
		return (-1);
	}

	/* Offset */
	if (0 >= dig__fread_port_O(&(ci->offset), 1, fp, plus->off_t_size))
	    return (0);
    }

    if (dig_fseek(fp, plus->cidx_head_size, SEEK_SET) == -1)
	return (-1);

    return (0);
}

/* Write spatial index */
int dig_write_cidx(struct gvfile * fp, struct Plus_head *plus)
{
    int i;

    dig_set_cur_port(&(plus->cidx_port));
    dig_rewind(fp);

    dig_write_cidx_head(fp, plus);

    /* Write category-type-id for each field */
    for (i = 0; i < plus->n_cidx; i++) {
	int j;
	struct Cat_index *ci;

	ci = &(plus->cidx[i]);
	ci->offset = dig_ftell(fp);

	/* convert type  */
	for (j = 0; j < ci->n_cats; j++)
	    ci->cat[j][1] = dig_type_to_store(ci->cat[j][1]);

	if (0 >= dig__fwrite_port_I((int *)ci->cat, 3 * ci->n_cats, fp))
	    return (-1);

	/* Return back */
	for (j = 0; j < ci->n_cats; j++)
	    ci->cat[j][1] = dig_type_from_store(ci->cat[j][1]);
    }

    dig_write_cidx_head(fp, plus);	/* rewrite with offsets */

    return 0;
}

/*!
  \brief Read spatial index file 

  \param fp pointer to gvfile structure
  \param[in,out] plus pointer to Plus_head structure
  \param head_only non-zero to read only head

  \return 0 OK
  \return 1 error
*/
int dig_read_cidx(struct gvfile * fp, struct Plus_head *plus, int head_only)
{
    int i;

    G_debug(3, "dig_read_cidx()");

    dig_cidx_free(plus);
    dig_cidx_init(plus);

    dig_rewind(fp);
    if (dig_read_cidx_head(fp, plus) == -1) {
	G_debug(3, "Cannot read cidx head");
	return 1;
    }

    if (head_only) {
	plus->cidx_up_to_date = 1;	/* OK ? */
	return 0;
    }

    dig_set_cur_port(&(plus->cidx_port));

    /* Read category-type-id for each field */
    for (i = 0; i < plus->n_cidx; i++) {
	int j;
	struct Cat_index *ci;

	ci = &(plus->cidx[i]);
	ci->a_cats = ci->n_cats;
	ci->cat = G_malloc(ci->a_cats * 3 * sizeof(int));

	if (dig_fseek(fp, ci->offset, 0) == -1)
	    return 1;

	if (0 >= dig__fread_port_I((int *)ci->cat, 3 * ci->n_cats, fp))
	    return 1;

	/* convert type  */
	for (j = 0; j < ci->n_cats; j++)
	    ci->cat[j][1] = dig_type_from_store(ci->cat[j][1]);
    }


    plus->cidx_up_to_date = 1;

    return 0;
}
