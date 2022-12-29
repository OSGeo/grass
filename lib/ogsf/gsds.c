/*!
   \file lib/ogsf/gsds.c

   \brief OGSF library - dataset loading and management (lower level functions) 

   GRASS OpenGL gsurf OGSF Library 

   The idea here is to treat datasets as separate objects, which SHOULD:
   - allow easier reuse of data for different attributes.
   - allow a mechanism for changing data and have changes reflected
   in each attribute using that data.
   - allow a mechanism to automatically update data when the data source
   is changed.
   - allow easier weaning from GRASS.
   - allow easier use of shared memory between processes.

   These structures are defined in gstypes.h:

   <code>
   typedef struct{
   float *fb;
   int *ib;
   short *sb;
   char *cb;
   struct BM *bm;
   } typbuff;
   </code>

   How about adding a transform func here, so GET_MAPATT would do an
   on-the-fly transformation? Or even a transform func LIST!

   <code>
   typedef struct{
   int data_id;
   int dims[MAXDIMS];
   int ndims;
   int numbytes;
   char unique_name[80];
   typbuff databuff;
   int changed;
   int need_reload;
   } dataset;
   </code>

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown UI GMS Lab
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/ogsf.h>

#define LUCKY 33
#define BLOC 20
#define MAX_DS 100

static int init_gsds(void);
static int check_numsets(void);
static dataset *get_dataset(int);
static int get_type(dataset *);

static dataset *Data[MAX_DS];
static dataset Ds[MAX_DS];	/* trying to avoid allocation */

static int Numsets = 0;

static int Cur_id = LUCKY;
static int Cur_max;
static size_t Tot_mem = 0;

/*!
   \brief Initialize gsds
 */
static int init_gsds(void)
{
    int i;

    for (i = 0; i < MAX_DS; i++) {
	/* avoiding dynamic allocation */
	Data[i] = &(Ds[i]);
    }

    Cur_max = MAX_DS;

    return (1);
}

/*!
   \brief Check numsets

   \return 0 numset < cur_max
 */
static int check_numsets(void)
{
    if (Numsets < Cur_max) {
	return (0);
    }

    G_fatal_error(_("Maximum number of datasets exceeded"));

    /* This return statement keeps compilers happy, it is never executed */
    return (0);
}

/*!
   \brief Get dataset

   \param id data id

   \return pointer to dataset struct
   \return NULL dataset not found
 */
static dataset *get_dataset(int id)
{
    int i;

    for (i = 0; i < Numsets; i++) {
	if (Data[i]->data_id == id) {
	    return (Data[i]);
	}
    }

    return (NULL);
}

/*!
   \brief Get type

   \param ds pointer to dataset struct

   \return type code
   \return -1 unsupported type
 */
static int get_type(dataset * ds)
{
    if (ds) {
	if (ds->databuff.bm) {
	    return (ATTY_MASK);
	}

	if (ds->databuff.cb) {
	    return (ATTY_CHAR);
	}

	if (ds->databuff.sb) {
	    return (ATTY_SHORT);
	}

	if (ds->databuff.ib) {
	    return (ATTY_INT);
	}

	if (ds->databuff.fb) {
	    return (ATTY_FLOAT);
	}
    }

    return (-1);
}

/*!
   \brief Get handle to gsds.   

   Successive calls will continue search until "begin" is set   
   (problem here is, unique_name no longer uniquely identifies
   dataset, since changes may be made; but unique_name should still
   be useful for reloading dataset)
   changes & types are set to actual for dataset if found.

   \param name
   \param changes,types  acceptable changes & types, flags may be or'd
   not changed is assumed to always be acceptable
   \param begin flag to indicate search from beginning

   \return data id
   \return -1 not found
 */
int gsds_findh(const char *name, IFLAG * changes, IFLAG * types, int begin)
{
    static int i;
    int start;

    start = begin ? 0 : i + 1;

    for (i = start; i < Numsets; i++) {
	if (!strcmp(Data[i]->unique_name, name)) {
	    if ((Data[i]->changed & *changes) || !(Data[i]->changed)) {
		if (get_type(Data[i]) & *types) {
		    *changes = Data[i]->changed;
		    *types = get_type(Data[i]);

		    return (Data[i]->data_id);
		}
	    }
	}
    }

    return (-1);
}

/*!
   \brief Get handle to gsds

   \param name raster map name

   \return -1 on failure
   \return data id
 */
int gsds_newh(const char *name)
{
    dataset *new;
    static int first = 1;
    int i;

    if (first) {
	if (0 > init_gsds()) {
	    return (-1);
	}

	first = 0;
    }
    else if (0 > check_numsets()) {
	return (-1);
    }

    if (!name) {
	return (-1);
    }

    new = Data[Numsets];

    if (new) {
	Numsets++;
	new->data_id = Cur_id++;

	for (i = 0; i < MAXDIMS; i++) {
	    new->dims[i] = 0;
	}

	new->unique_name = G_store(name);
	new->databuff.fb = NULL;
	new->databuff.ib = NULL;
	new->databuff.sb = NULL;
	new->databuff.cb = NULL;
	new->databuff.bm = NULL;
	new->databuff.nm = NULL;
	new->databuff.k = 0.0;
	new->changed = 0;
	new->ndims = 0;
	new->need_reload = 1;

	return (new->data_id);
    }

    return (-1);
}

/*!
   \brief Get data buffer

   Doesn't prevent writing a buff thats's been gotten with change_flag
   == 0 (could return a copy, but willing to trust calling func for
   now)

   \param id dataset id
   \param change_flag set changed flag

   \return pointer to typbuff struct
   \return NULL on failure
 */
typbuff *gsds_get_typbuff(int id, IFLAG change_flag)
{
    dataset *ds;

    if ((ds = get_dataset(id))) {
	ds->changed = ds->changed | change_flag;
	ds->need_reload = 0;

	return (&(ds->databuff));
    }

    return (NULL);
}

/*!
   \brief Get name

   \param id

   \return name
   \return NULL on failure
 */
char *gsds_get_name(int id)
{
    int i;
    dataset *fds;
    static char retstr[GPATH_MAX];

    for (i = 0; i < Numsets; i++) {
	if (Data[i]->data_id == id) {
	    fds = Data[i];
	    strcpy(retstr, fds->unique_name);

	    return (retstr);
	}
    }

    return (NULL);
}

/*!
   \brief Free allocated dataset

   \param id

   \return 0 not found
   \return 1 found
 */
int gsds_free_datah(int id)
{
    int i, j, found = 0;
    dataset *fds;

    G_debug(3, "gsds_free_datah");

    for (i = 0; i < Numsets; i++) {
	if (Data[i]->data_id == id) {
	    found = 1;
	    fds = Data[i];
	    free_data_buffs(fds, ATTY_ANY);
	    G_free((void *)fds->unique_name);
	    fds->unique_name = NULL;
	    fds->data_id = 0;

	    for (j = i; j < (Numsets - 1); j++) {
		Data[j] = Data[j + 1];
	    }

	    Data[j] = fds;
	}
    }

    if (found) {
	--Numsets;
    }

    return (found);
}

/*!
   \brief Free allocated buffer

   \param id dataset id
   \param typ data type

   \return 0 not found
   \return 1 found
 */
int gsds_free_data_buff(int id, int typ)
{
    int i, found = 0;
    dataset *fds;

    for (i = 0; i < Numsets; i++) {
	if (Data[i]->data_id == id) {
	    found = 1;
	    fds = Data[i];
	    free_data_buffs(fds, typ);
	}
    }

    return (found);
}

/*!
   \brief Free data buffer

   \param ds pointer to dataset struct
   \param typ data type

   \return freed size
 */
size_t free_data_buffs(dataset * ds, int typ)
{
    int i;
    size_t siz, nsiz = 1, freed = 0;

    for (i = 0; i < ds->ndims; i++) {
	nsiz *= ds->dims[i];
    }

    if (typ & ATTY_NULL) {
	if (ds->databuff.nm) {
	    siz = BM_get_map_size(ds->databuff.nm);
	    BM_destroy(ds->databuff.nm);
	    ds->databuff.nm = NULL;
	    freed += siz;
	}
    }

    if (typ & ATTY_MASK) {
	if (ds->databuff.bm) {
	    siz = BM_get_map_size(ds->databuff.bm);
	    BM_destroy(ds->databuff.bm);
	    ds->databuff.bm = NULL;
	    freed += siz;
	}
    }

    if (typ & ATTY_CHAR) {
	if (ds->databuff.cb) {
	    siz = nsiz * sizeof(char);
	    free(ds->databuff.cb);
	    ds->databuff.cb = NULL;
	    freed += siz;
	}
    }

    if (typ & ATTY_SHORT) {
	if (ds->databuff.sb) {
	    siz = nsiz * sizeof(short);
	    free(ds->databuff.sb);
	    ds->databuff.sb = NULL;
	    freed += siz;
	}
    }

    if (typ & ATTY_INT) {
	if (ds->databuff.ib) {
	    siz = nsiz * sizeof(int);
	    free(ds->databuff.ib);
	    ds->databuff.ib = NULL;
	    freed += siz;
	}
    }

    if (typ & ATTY_FLOAT) {
	if (ds->databuff.fb) {
	    siz = nsiz * sizeof(float);
	    free(ds->databuff.fb);
	    ds->databuff.fb = NULL;
	    freed += siz;
	}
    }

    Tot_mem -= freed;
    ds->numbytes -= freed;

    if (freed) {
	G_debug(5, "free_data_buffs(): freed data from id no. %d",
		ds->data_id);
	G_debug(5,
		"free_data_buffs(): %.3f Kbytes freed, current total = %.3f",
		freed / 1000., Tot_mem / 1000.);
    }

    return (freed);
}

/*!
   \brief Allocates correct buffer according to type, keeps track of total mem

   \todo add ATTY_CONST

   \param id dataset id
   \param dims array of dimensions
   \param ndims number of dimensions
   \param type data type

   \return amount of allocated memory
 */
size_t gsds_alloc_typbuff(int id, int *dims, int ndims, int type)
{
    dataset *ds;
    int i;
    size_t siz = 1;

    if ((ds = get_dataset(id))) {
	/*
	   free_data_buffs(ds); 
	   careful here - allowing > 1 type to coexist (for float -> color conv.)
	   now also use this to allocate a null mask
	   (then if not used, use gsds_free_data_buff(id, ATTY_NULL))
	 */

	for (i = 0; i < ndims; i++) {
	    ds->dims[i] = dims[i];
	    siz *= dims[i];
	}

	switch (type) {
	case ATTY_NULL:
	    if (ndims != 2) {
		/* higher dimension bitmaps not supported */
		return 0;
	    }

	    if (NULL == (ds->databuff.nm = BM_create(dims[1], dims[0]))) {
		return 0;
	    }

	    siz = BM_get_map_size(ds->databuff.nm);

	    break;

	case ATTY_MASK:
	    if (ndims != 2) {
		/* higher dimension bitmaps not supported */
		return (-1);
	    }

	    if (NULL == (ds->databuff.bm = BM_create(dims[1], dims[0]))) {
		return 0;
	    }

	    siz = BM_get_map_size(ds->databuff.bm);

	    break;

	case ATTY_CHAR:
	    siz *= sizeof(char);

	    if (siz) {
		if (NULL ==
		    (ds->databuff.cb = (unsigned char *)G_malloc(siz))) {
		    return 0;
		}
	    }
	    else {
		return 0;
	    }

	    break;

	case ATTY_SHORT:
	    siz *= sizeof(short);

	    if (siz) {
		if (NULL == (ds->databuff.sb = (short *)G_malloc(siz))) {
		    return 0;
		}
	    }
	    else {
		return 0;
	    }

	    break;

	case ATTY_INT:
	    siz *= sizeof(int);

	    if (siz) {
		if (NULL == (ds->databuff.ib = (int *)G_malloc(siz))) {
		    return 0;
		}
	    }
	    else {
		return 0;
	    }

	    break;

	case ATTY_FLOAT:
	    siz *= sizeof(float);

	    if (siz) {
		if (NULL == (ds->databuff.fb = (float *)G_malloc(siz))) {
		    return 0;
		}
	    }
	    else {
		return 0;
	    }

	    break;

	default:
	    return 0;
	}

	ds->changed = 0;	/* starting with clean slate */
	ds->need_reload = 1;
	ds->numbytes += siz;
	ds->ndims = ndims;
	Tot_mem += siz;

	G_debug(5,
		"gsds_alloc_typbuff(): %f Kbytes allocated, current total = %f",
		siz / 1000., Tot_mem / 1000.);

	return (siz);
    }

    return 0;
}

/*!
   \brief ADD

   \param id

   \return -1 on error
   \return 
 */
int gsds_get_changed(int id)
{
    dataset *ds;

    if ((ds = get_dataset(id))) {
	return ((int)ds->changed);
    }

    return (-1);
}

/*!
   \brief ADD

   \param id
   \param reason

   \return -1 on error
   \return
 */
int gsds_set_changed(int id, IFLAG reason)
{
    dataset *ds;

    if ((ds = get_dataset(id))) {
	ds->changed = reason;
    }

    return (-1);
}

/*!                                             
   \brief ADD

   \param id

   \return
 */
int gsds_get_type(int id)
{
    dataset *ds;

    ds = get_dataset(id);

    return (get_type(ds));
}
