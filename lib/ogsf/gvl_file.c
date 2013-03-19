/*!
   \file gvl_file.c

   \brief OGSF library - loading and manipulating volumes (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Tomas Paudits (February 2004)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/ogsf.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

#define LUCKY                 33

#define MODE_DIRECT            0
#define MODE_SLICE             1
#define MODE_FULL              2
#define MODE_PRELOAD           3

#define MODE_DEFAULT		   0

#define STATUS_READY           0
#define STATUS_BUSY            1

/*!
   \brief structure for slice mode reading from volume file
 */
typedef struct
{
    int num, skip;
    int crnt, base;

    void *slice[MAX_VOL_SLICES];
} slice_data;

static geovol_file *Data[MAX_VOL_FILES];
static geovol_file Df[MAX_VOL_FILES];	/* trying to avoid allocation */

static int Numfiles = 0;

static int Cur_id = LUCKY;
static int Cur_max;

static int Rows, Cols, Depths;

/* local functions prototypes */
void *open_g3d_file(const char *, IFLAG *, double *, double *);
int close_g3d_file(void *);

/*!
   \brief Initialize volume files

   \return 1
 */
static int init_volfiles(void)
{
    int i;
    RASTER3D_Region *w3;

    for (i = 0; i < MAX_VOL_FILES; i++) {
	/* avoiding dynamic allocation */
	Data[i] = &(Df[i]);
    }

    Cur_max = MAX_VOL_FILES;

    /* get window */
    w3 = GVL_get_window();

    /* set cols, rows, depths from window */
    Cols = w3->cols;
    Rows = w3->rows;
    Depths = w3->depths;

    return (1);
}

/*!
   \brief Check number of files

   \return 0
 */
static int check_num_volfiles(void)
{
    if (Numfiles < Cur_max) {
	return (0);
    }

    G_fatal_error(_("Maximum number of datafiles exceeded"));

    /* This return statement keeps compilers happy, it is never executed */
    return (0);
}

/*!
   \brief Get geovol_file structure for given handle

   \param id

   \return pointer to geovol_file struct
   \return NULL on failure
 */
geovol_file *gvl_file_get_volfile(int id)
{
    int i;

    for (i = 0; i < Numfiles; i++) {
	if (Data[i]->data_id == id) {
	    return (Data[i]);
	}
    }

    return (NULL);
}

/*!
   \brief Find file with name and type in geovol_file array an return handle

   \param name file name
   \param begin

   \param data id
   \param -1 not found
 */
int find_datah(const char *name, IFLAG type, int begin)
{
    static int i;
    int start;

    start = begin ? 0 : i + 1;

    for (i = start; i < Numfiles; i++) {
	if (!strcmp(Data[i]->file_name, name)) {
	    if (Data[i]->file_type == type) {
		return (Data[i]->data_id);
	    }
	}
    }

    return (-1);
}

/*!
   \brief Get file name for given handle

   \param id handle id

   \return file name
   \return NULL on failure
 */
char *gvl_file_get_name(int id)
{
    int i;
    geovol_file *fvf;
    static char retstr[GPATH_MAX];

    for (i = 0; i < Numfiles; i++) {
	if (Data[i]->data_id == id) {
	    fvf = Data[i];
	    strcpy(retstr, fvf->file_name);

	    return (retstr);
	}
    }

    return (NULL);
}

/*!
   \brief Get file type for given handle

   \param vf pointer to geovol_file struct

   \return file type 
 */
int gvl_file_get_file_type(geovol_file * vf)
{
    return (vf->file_type);
}

/*!
   \brief Get data type for given handle

   \param vf pointer to geovol_file struct

   \return data type
 */
int gvl_file_get_data_type(geovol_file * vf)
{
    return (vf->data_type);
}

/*!
   \brief Get minimum and maximum value in volume file

   \param vf pointer to geovol_file struct
   \param[out] min min value
   \param[out] max max value
 */
void gvl_file_get_min_max(geovol_file * vf, double *min, double *max)
{
    *min = vf->min;
    *max = vf->max;
}

/*!
   \brief Open 3d raster file

   \param name file name
   \param file_type file type
   \param data_type data type
   \param[out] min min value
   \param[out] max max value

   \return pointer to file
   \return NULL on failure
 */
void *open_volfile(const char *name, IFLAG file_type, IFLAG * data_type,
		   double *min, double *max)
{
    if (file_type == VOL_FTYPE_RASTER3D) {
	return open_g3d_file(name, data_type, min, max);
    }

    return (NULL);
}

/*!
   \brief Close volume file

   \param map volume filename
   \param type file type

   \return
   \return -1 on failure
 */
int close_volfile(void *map, IFLAG type)
{
    if (type == VOL_FTYPE_RASTER3D) {
	return close_g3d_file(map);
    }

    return (-1);
}

/*!
   \brief Get handle for given file name and type

   \param name volume filename
   \param file_type file type

   \return data id
   \return -1 on failure
 */
int gvl_file_newh(const char *name, IFLAG file_type)
{
    geovol_file *new;
    static int first = 1;
    int i, id;
    void *m;
    IFLAG data_type;
    double min, max;

    if (first) {
	if (0 > init_volfiles()) {
	    return (-1);
	}

	first = 0;
    }

    if (0 <= (id = find_datah(name, file_type, 1))) {
	for (i = 0; i < Numfiles; i++) {
	    if (Data[i]->data_id == id) {
		new = Data[i];
		new->count++;

		return (id);
	    }
	}
    }

    if (0 > check_num_volfiles()) {
	return (-1);
    }

    if (!name) {
	return (-1);
    }

    if ((m = open_volfile(name, file_type, &data_type, &min, &max)) == NULL) {
	return (-1);
    }

    new = Data[Numfiles];

    if (new) {
	Numfiles++;
	new->data_id = Cur_id++;

	new->file_name = G_store(name);
	new->file_type = file_type;
	new->count = 1;
	new->map = m;
	new->min = min;
	new->max = max;
	new->data_type = data_type;

	new->status = STATUS_READY;
	new->buff = NULL;

	new->mode = 255;
	gvl_file_set_mode(new, MODE_DEFAULT);

	return (new->data_id);
    }

    return (-1);
}

/*!
   \brief Free allocated buffers

   \param vf pointer to geovol_file struct

   \return 1
 */
int free_volfile_buffs(geovol_file * vf)
{
    if (vf->mode == MODE_SLICE) {
	G_free(vf->buff);
	vf->buff = NULL;
    }

    if (vf->mode == MODE_PRELOAD) {
	G_free(vf->buff);
	vf->buff = NULL;
    }

    return (1);
}

/*!
   \brief Free geovol_file structure for given handle

   \param id 

   \return
 */
int gvl_file_free_datah(int id)
{
    int i, j, found = -1;
    geovol_file *fvf;

    G_debug(5, "gvl_file_free_datah(): id=%d", id);

    for (i = 0; i < Numfiles; i++) {
	if (Data[i]->data_id == id) {
	    found = 1;
	    fvf = Data[i];

	    if (fvf->count > 1) {
		fvf->count--;
	    }
	    else {
		close_volfile(fvf->map, fvf->file_type);
		free_volfile_buffs(fvf);

		G_free(fvf->file_name);
		fvf->file_name = NULL;

		fvf->data_id = 0;

		for (j = i; j < (Numfiles - 1); j++) {
		    Data[j] = Data[j + 1];
		}

		Data[j] = fvf;

		--Numfiles;
	    }
	}
    }

    return (found);
}

/******************************************************************/
/* reading from RASTER3D raster volume files */

/******************************************************************/

/*!
   \brief Open 3d raster file

   \param filename file name
   \param type data type
   \param[out] min min value
   \param[out] max max value

   \returns pointer to data
 */
void *open_g3d_file(const char *filename, IFLAG * type, double *min,
		    double *max)
{
    const char *mapset;
    int itype;
    void *map;

    /* search for g3d file a return his mapset */
    mapset = G_find_raster3d(filename, "");
    if (!mapset) {
	G_warning(_("3D raster map <%s> not found"), filename);
	return (NULL);
    }

    /* open g3d file */
    map =
	Rast3d_open_cell_old(filename, mapset, RASTER3D_DEFAULT_WINDOW,
			RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);
    if (!map) {
	G_warning(_("Unable to open 3D raster map <%s>"), filename);
	return (NULL);
    }

    /* load range into range structure of map */
    if (!Rast3d_range_load(map)) {
	G_warning(_("Unable to read range of 3D raster map <%s>"), filename);
	return (NULL);
    }

    Rast3d_range_min_max(map, min, max);

    /* get file data type */
    itype = Rast3d_file_type_map(map);
    if (itype == FCELL_TYPE)
	*type = VOL_DTYPE_FLOAT;
    if (itype == DCELL_TYPE)
	*type = VOL_DTYPE_DOUBLE;

    return (map);
}

/*!
   \brief Close g3d file

   \param map 3d raster map

   \return -1 on failure
   \return 1 on success
 */
int close_g3d_file(void *map)
{
    /* close opened g3d file */
    if (Rast3d_close((RASTER3D_Map *) map) != 1) {
	G_warning(_("Unable to close 3D raster map <%s>"),
		  ((RASTER3D_Map *) map)->fileName);
	return (-1);
    }

    return (1);
}

/*!
   \brief Eead value from g3d file

   \param type data type
   \param map 3D raster map
   \param x,y,z real coordinates
   \param[out] value data value

   \return -1 on failure
   \return 1 on success
 */
int read_g3d_value(IFLAG type, void *map, int x, int y, int z, void *value)
{
    switch (type) {
	/* float data type */
    case (VOL_DTYPE_FLOAT):
	*((float *)value) = Rast3d_get_float(map, x, y, z);
	break;

	/* double data type */
    case (VOL_DTYPE_DOUBLE):
	*((double *)value) = Rast3d_get_double(map, x, y, z);
	break;

	/* unsupported data type */
    default:
	return (-1);
    }

    return (1);
}

/*!
   \brief Read slice of values at level from g3d file

   \param type data type
   \param map 3D raster map
   \param level
   \param[out] data

   \return -1 on failure
   \return 0 on success
 */
int read_g3d_slice(IFLAG type, void *map, int level, void *data)
{
    int x, y;

    switch (type) {
	/* float data type */
    case (VOL_DTYPE_FLOAT):
	for (x = 0; x < Cols; x++) {
	    for (y = 0; y < Rows; y++) {
		((float *)data)[x + y * Cols] =
		    Rast3d_get_float(map, x, y, level);
	    }
	}

	break;

	/* double data type */
    case (VOL_DTYPE_DOUBLE):
	for (x = 0; x < Cols; x++) {
	    for (y = 0; y < Rows; y++) {
		((double *)data)[x + y * Cols] =
		    Rast3d_get_double(map, x, y, level);
	    }
	}

	break;

	/* unsupported data type */
    default:
	return (-1);
    }

    return (1);
}

/*!
   \brief Read all values from g3d file

   \param type data type
   \param map 3D raster map
   \param[out] data data buffer

   \return -1 on failure
   \return 1 on success
 */
int read_g3d_vol(IFLAG type, void *map, void *data)
{
    int x, y, z;

    switch (type) {
	/* float data type */
    case (VOL_DTYPE_FLOAT):
	for (x = 0; x < Cols; x++) {
	    for (y = 0; y < Rows; y++) {
		for (z = 0; z < Depths; z++) {
		    ((float *)data)[x + y * Cols + z * Rows * Cols] =
			Rast3d_get_float(map, x, y, z);
		}
	    }
	}

	break;

	/* double data type */
    case (VOL_DTYPE_DOUBLE):
	for (x = 0; x < Cols; x++) {
	    for (y = 0; y < Rows; y++) {
		for (z = 0; z < Depths; z++) {
		    ((double *)data)[x + y * Cols + z * Rows * Cols] =
			Rast3d_get_double(map, x, y, z);
		}
	    }
	}

	break;

	/* unsupported data type */
    default:
	return (-1);
    }

    return (1);
}

/*!
   \brief Check for null value

   \param type data type
   \param value

   \return 1 if value is null
   \return 0 if value is not null
   \return -1 on failure (unsupported data type
 */
int is_null_g3d_value(IFLAG type, void *value)
{
    switch (type) {
	/* float data type */
    case (VOL_DTYPE_FLOAT):
	return Rast3d_is_null_value_num(value, FCELL_TYPE);
	break;

	/* double data type */
    case (VOL_DTYPE_DOUBLE):
	return Rast3d_is_null_value_num(value, DCELL_TYPE);
	break;

	/* unsupported data type */
    default:
	return (-1);
    }

    return (-1);
}

/******************************************************************/
/* reading from buffer */

/******************************************************************/

/*!
   \brief Get value from buffer

   \param type data type
   \param data data buffer
   \param offset
   \param value

   \return -1 on failure (unsupported data type)
   \return 1 on success
 */
int get_buff_value(IFLAG type, void *data, int offset, void *value)
{
    switch (type) {
	/* float data type */
    case (VOL_DTYPE_FLOAT):
	*((float *)value) = ((float *)data)[offset];
	break;

	/* double data type */
    case (VOL_DTYPE_DOUBLE):
	*((double *)value) = ((double *)data)[offset];
	break;

	/* unsupported data type */
    default:
	return (-1);
    }

    return (1);
}

/******************************************************************/
/* direct mode reading from volume file */

/******************************************************************/

/*!
   \brief Read value direct from volume file

   \param vf pointer to geovol_file struct
   \param x,y,z real point
   \param[out] value data value

   \return -1 on failure
   \return 1 on success
 */
int get_direct_value(geovol_file * vf, int x, int y, int z, void *value)
{
    switch (vf->file_type) {
	/* RASTER3D file type */
    case (VOL_FTYPE_RASTER3D):
	if (0 > read_g3d_value(vf->data_type, vf->map, x, y, z, value))
	    return (-1);
	break;

    default:
	return (-1);
    }

    return (1);
}

/******************************************************************/
/* full mode reading from volume file */

/******************************************************************/

/*!
   \brief Allocate buffer memory for full mode reading

   \param vf pointer to geovol_file

   \return -1 on failure
   \return 1 on success
 */
int alloc_vol_buff(geovol_file * vf)
{
    switch (vf->data_type) {
	/* float data type */
    case (VOL_DTYPE_FLOAT):
	if ((vf->buff =
	     (float *)G_malloc(sizeof(float) * Cols * Rows * Depths)) == NULL)
	    return (-1);
	break;

	/* double data type */
    case (VOL_DTYPE_DOUBLE):
	if ((vf->buff =
	     (double *)G_malloc(sizeof(double) * Cols * Rows * Depths)) ==
	    NULL)
	    return (-1);
	break;

	/* unsupported data type */
    default:
	return (-1);
    }

    return (1);
}

/*!
   \brief Free memory buffer memory

   \param vf pointer to geovol_file struct

   \return 1
 */
int free_vol_buff(geovol_file * vf)
{
    G_free(vf->buff);

    return (1);
}

/*!
   \brief Read all values from volume file

   \param vf pointer to geovol_file struct

   \return -1 on failure
   \return 1 on success
 */
int read_vol(geovol_file * vf)
{
    switch (vf->file_type) {
	/* RASTER3D file format */
    case (VOL_FTYPE_RASTER3D):
	if (0 > read_g3d_vol(vf->data_type, vf->map, vf->buff))
	    return (-1);
	break;
	/* unsupported file format */
    default:
	return (-1);
    }

    return (1);
}

/*!
   \brief Get value from volume buffer

   \param vf pointer to geovol_file struct
   \param x,y,z real point
   \param[out] value data value

   \return 1
 */
int get_vol_value(geovol_file * vf, int x, int y, int z, void *value)
{
    get_buff_value(vf->data_type, vf->buff, z * Rows * Cols + y * Cols + x,
		   value);

    return (1);
}

/******************************************************************/
/* slice mode reading from volume file */

/******************************************************************/

/*!
   \brief Allocate buffer for slice mode reading

   \param vf pointer to geovol_file struct

   \return -1 on failure
   \return 1 on success
 */
int alloc_slice_buff(geovol_file * vf)
{
    int i;
    slice_data *sd = (slice_data *) vf->buff;

    switch (vf->data_type) {
	/* float data type */
    case (VOL_DTYPE_FLOAT):
	for (i = 0; i < sd->num; i++) {
	    if ((sd->slice[i] =
		 (float *)G_malloc(sizeof(float) * Cols * Rows)) == NULL)
		return (-1);
	}
	break;

	/* double data type */
    case (VOL_DTYPE_DOUBLE):
	for (i = 0; i < sd->num; i++) {
	    if ((sd->slice[i] =
		 (double *)G_malloc(sizeof(double) * Cols * Rows)) == NULL)
		return (-1);
	}
	break;

	/* unsupported data type */
    default:
	return (-1);
    }

    return (1);
}

/*!
   \brief Free buffer for slice mode reading

   \param vf pointer to geovol_file struct

   \return 1
 */
int free_slice_buff(geovol_file * vf)
{
    int i;
    slice_data *sd = (slice_data *) vf->buff;

    for (i = 0; i < sd->num; i++) {
	G_free(sd->slice[i]);
    }

    return (1);
}

/*!
   \brief Read slice of values at level from volume file

   \param vf pointer to geovol_file struct
   \param s
   \param l

   \return -1 on failure
   \return 1 on success
 */
int read_slice(geovol_file * vf, int s, int l)
{
    /* get slice structure */
    slice_data *sd = (slice_data *) vf->buff;

    switch (vf->file_type) {
	/* RASTER3D file format */
    case (VOL_FTYPE_RASTER3D):
	if (0 > read_g3d_slice(vf->data_type, vf->map, l, sd->slice[s]))
	    return (-1);
	break;
	/* unsupported file format */
    default:
	return (-1);
    }

    return (1);
}

/*!
   \brief Read new slice into buffer

   \param vf pointer to geovol_file struct
 */
void shift_slices(geovol_file * vf)
{
    void *tmp;
    int i;

    slice_data *sd = (slice_data *) vf->buff;

    /* change pointers to slices */
    tmp = sd->slice[0];
    for (i = 0; i < sd->num - 1; i++) {
	sd->slice[i] = sd->slice[i + 1];
    }
    sd->slice[sd->num - 1] = tmp;

    /* read new slice data */
    read_slice(vf, sd->num, sd->crnt + 1 + (sd->num - sd->base));

    /* increase current slice value */
    sd->crnt++;
}

/*!
   \brief Get value from slice buffer

   \param vf pointer to geovol_file struct
   \param x,y,z real point
   \param[out] value data value

   \return -1 on failure
   \return 1 on success
 */
int get_slice_value(geovol_file * vf, int x, int y, int z, void *value)
{
    slice_data *sd = (slice_data *) vf->buff;

    /* value is in loaded slices */
    if ((z >= sd->crnt - (sd->base - 1)) &&
	(z <= sd->crnt + sd->num - sd->base)) {

	get_buff_value(vf->data_type, sd->slice[(z - sd->crnt)], y * Cols + x,
		       value);
    }

    /* if value isn't in loaded slices, need read new data slice */
    else if (z == sd->crnt - (sd->base - 1) + 1) {
	shift_slices(vf);
	get_buff_value(vf->data_type, sd->slice[(z - sd->crnt)], y * Cols + x,
		       value);
    }

    /* value out of range */
    else {
	return (-1);
    }

    return (1);
}

/******************************************************************/
/* reading from volume file */

/******************************************************************/

/*!
   \brief Start read - allocate memory buffer a read first data into buffer

   \param vf pointer to geovol_file struct

   \return -1 on failure
   \return 1 on success
 */
int gvl_file_start_read(geovol_file * vf)
{
    int i;
    slice_data *sd;

    /* check status */
    if (vf->status == STATUS_BUSY)
	return (-1);

    switch (vf->mode) {
	/* read whole volume into memory */
    case (MODE_FULL):
	/* allocate memory */
	if (0 > alloc_vol_buff(vf))
	    return (-1);

	/* read volume */
	read_vol(vf);
	break;

	/* read first data slices into memory */
    case (MODE_SLICE):
	/* allocate slices buffer memory */
	if (0 > alloc_slice_buff(vf))
	    return (-1);

	/* read volume */
	sd = (slice_data *) vf->buff;
	/* set current slice to 0 */
	sd->crnt = 0;

	/* read first slices into buffer */
	for (i = 0; i < (sd->num - sd->base + 1); i++)
	    read_slice(vf, (sd->base - 1) + i, i);
	break;
    }

    /* set status */
    vf->status = STATUS_BUSY;

    return (1);
}

/*!
   \brief End read - free buffer memory

   \param vf pointer to geovol_file struct

   \return -1 on failure
   \return 1 on success
 */
int gvl_file_end_read(geovol_file * vf)
{
    /* check status */
    if (vf->status == STATUS_READY)
	return (-1);

    switch (vf->mode) {
    case (MODE_FULL):
	if (0 > free_vol_buff(vf))
	    return (-1);
	break;

    case (MODE_SLICE):
	/* allocate slices buffer memory */
	if (0 > free_slice_buff(vf))
	    return (-1);
    }

    /* set status */
    vf->status = STATUS_READY;

    return (1);
}

/*!
   \brief Get value for volume file at x, y, z

   \param vf pointer to geovol_file struct

   \return -1 on failure
   \return 1 on success
 */
int gvl_file_get_value(geovol_file * vf, int x, int y, int z, void *value)
{
    /*  check status */
    if (vf->status != STATUS_BUSY) {
	return (-1);
    }

    switch (vf->mode) {
	/* read value direct from g3d file */
    case (MODE_DIRECT):
	if (0 > get_direct_value(vf, x, y, z, value))
	    return (-1);
	break;

    case (MODE_SLICE):
	if (0 > get_slice_value(vf, x, y, z, value))
	    return (-1);
	break;

    case (MODE_FULL):
    case (MODE_PRELOAD):
	if (0 > get_vol_value(vf, x, y, z, value))
	    return (-1);
	break;
    }
    return (1);
}

/*!
   \brief Check for null value

   \param vf pointer to geovol_file struct
   \param value data value

   \return -1 on failure
   \return 1 on success
 */
int gvl_file_is_null_value(geovol_file * vf, void *value)
{
    switch (vf->file_type) {
	/* RASTER3D file format */
    case (VOL_FTYPE_RASTER3D):
	return is_null_g3d_value(vf->file_type, value);
	break;
	/* unsupported file format */
    default:
	return (-1);
    }

    return (-1);
}

/******************************************************************/
/* set parameters for reading volumes */

/******************************************************************/

/*!
   \brief Set read mode

   \param vf pointer to geovol_file struct
   \param mode read mode

   \return -1 on failure
   \return 1 on success
 */
int gvl_file_set_mode(geovol_file * vf, IFLAG mode)
{
    slice_data *sd;

    if (vf->status == STATUS_BUSY)
	return (-1);

    if (vf->mode == mode)
	return (1);

    if (vf->mode == MODE_SLICE)
	G_free(vf->buff);

    if (vf->mode == MODE_PRELOAD)
	G_free(vf->buff);

    if (mode == MODE_SLICE) {
	if ((vf->buff = (slice_data *) G_malloc(sizeof(slice_data))) == NULL)
	    return (-1);

	sd = (slice_data *) vf->buff;
	sd->num = 1;
	sd->crnt = 0;
	sd->base = 1;
    }

    if (mode == MODE_PRELOAD) {
	/* allocate memory */
	if (0 > alloc_vol_buff(vf))
	    return (-1);

	/* read volume */
	read_vol(vf);
    }

    vf->mode = mode;

    return (1);
}

/*!
   \brief Set parameters for slice reading

   \param vf pointer to geovol_file struct
   \param n
   \param b

   \return -1 on failure
   \return 1 on success
 */
int gvl_file_set_slices_param(geovol_file * vf, int n, int b)
{
    slice_data *sd;

    if (vf->status == STATUS_BUSY)
	return (-1);

    if (!(vf->mode == MODE_SLICE))
	return (-1);

    sd = (slice_data *) vf->buff;
    sd->num = n;
    sd->base = b;

    return (1);
}
