
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:     	IO array managment functions 
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/
#include <math.h>

#include <grass/N_pde.h>
#include <grass/raster.h>
#include <grass/glocale.h>


/* ******************** 2D ARRAY FUNCTIONS *********************** */

/*!
 * \brief Read a raster map into a N_array_2d structure
 *
 * The raster map will be opened in the current region settings.
 * If no N_array_2d structure is provided (NULL pointer), a new structure will be
 * allocated with the same data type as the raster map and the size of the current region. 
 * The array offset will be set to 0.
 * <br><br>
 * If a N_array_2d structure is provided, the values from the raster map are 
 * casted to the N_array_2d type. The array must have the same size 
 * as the current region. 
 * <br><br>
 * The new created or the provided array are returned.
 * If the reading of the raster map fails, G_fatal_error() will
 * be invoked.
 *
 * \param name * char - the name of an existing raster map
 * \param array * N_array_2d - an existing array or NULL
 * \return N_array_2d * - the existing or new allocated array
 * */
N_array_2d *N_read_rast_to_array_2d(char *name, N_array_2d * array)
{
    int map;			/*The rastermap */
    int x, y, cols, rows, type;
    void *rast;
    void *ptr;
    struct Cell_head region;
    N_array_2d *data = array;

    /* Get the active region */
    G_get_set_window(&region);

    /*set the rows and cols */
    rows = region.rows;
    cols = region.cols;

    /*open the raster map */
    map = Rast_open_old(name, "");

    type = Rast_get_map_type(map);

    /*if the array is NULL create a new one with the data type of the raster map */
    /*the offset is 0 by default */
    if (data == NULL) {
	if (type == DCELL_TYPE) {
	    data = N_alloc_array_2d(cols, rows, 0, DCELL_TYPE);
	}
	if (type == FCELL_TYPE) {
	    data = N_alloc_array_2d(cols, rows, 0, FCELL_TYPE);
	}
	if (type == CELL_TYPE) {
	    data = N_alloc_array_2d(cols, rows, 0, CELL_TYPE);
	}
    }
    else {
	/*Check the array sizes */
	if (data->cols != cols)
	    G_fatal_error
		("N_read_rast_to_array_2d: the data array size is different from the current region settings");
	if (data->rows != rows)
	    G_fatal_error
		("N_read_rast_to_array_2d: the data array size is different from the current region settings");
    }

    rast = Rast_allocate_buf(type);

    G_message(_("Reading raster map <%s> into memory"), name);

    for (y = 0; y < rows; y++) {
	G_percent(y, rows - 1, 10);

	Rast_get_row(map, rast, y, type);

	for (x = 0, ptr = rast; x < cols;
	     x++, ptr = G_incr_void_ptr(ptr, Rast_cell_size(type))) {
	    if (type == CELL_TYPE) {
		if (Rast_is_c_null_value(ptr)) {
		    N_put_array_2d_value_null(data, x, y);
		}
		else {
		    if (data->type == CELL_TYPE)
			N_put_array_2d_c_value(data, x, y,
					       (CELL) * (CELL *) ptr);
		    if (data->type == FCELL_TYPE)
			N_put_array_2d_f_value(data, x, y,
					       (FCELL) * (CELL *) ptr);
		    if (data->type == DCELL_TYPE)
			N_put_array_2d_d_value(data, x, y,
					       (DCELL) * (CELL *) ptr);
		}
	    }
	    if (type == FCELL_TYPE) {
		if (Rast_is_f_null_value(ptr)) {
		    N_put_array_2d_value_null(data, x, y);
		}
		else {
		    if (data->type == CELL_TYPE)
			N_put_array_2d_c_value(data, x, y,
					       (CELL) * (FCELL *) ptr);
		    if (data->type == FCELL_TYPE)
			N_put_array_2d_f_value(data, x, y,
					       (FCELL) * (FCELL *) ptr);
		    if (data->type == DCELL_TYPE)
			N_put_array_2d_d_value(data, x, y,
					       (DCELL) * (FCELL *) ptr);
		}
	    }
	    if (type == DCELL_TYPE) {
		if (Rast_is_d_null_value(ptr)) {
		    N_put_array_2d_value_null(data, x, y);
		}
		else {
		    if (data->type == CELL_TYPE)
			N_put_array_2d_c_value(data, x, y,
					       (CELL) * (DCELL *) ptr);
		    if (data->type == FCELL_TYPE)
			N_put_array_2d_f_value(data, x, y,
					       (FCELL) * (DCELL *) ptr);
		    if (data->type == DCELL_TYPE)
			N_put_array_2d_d_value(data, x, y,
					       (DCELL) * (DCELL *) ptr);
		}
	    }
	}
    }

    /* Close file */
    Rast_close(map);

    return data;
}

/*!
 * \brief Write a N_array_2d struct to a raster map
 *
 * A new raster map is created with the same type as the N_array_2d.
 * The current region is used to open the raster map.
 * The N_array_2d must have the same size as the current region.
 If the writing of the raster map fails, G_fatal_error() will
 * be invoked.

 * \param array N_array_2d * 
 * \param name char * - the name of the raster map
 * \return void
 *
 * */
void N_write_array_2d_to_rast(N_array_2d * array, char *name)
{
    int map;			/*The rastermap */
    int x, y, cols, rows, count, type;
    CELL *rast = NULL;
    FCELL *frast = NULL;
    DCELL *drast = NULL;
    struct Cell_head region;

    if (!array)
	G_fatal_error(_("N_array_2d * array is empty"));

    /* Get the current region */
    G_get_set_window(&region);

    rows = region.rows;
    cols = region.cols;
    type = array->type;

    /*Open the new map */
    map = Rast_open_new(name, type);

    if (type == CELL_TYPE)
	rast = Rast_allocate_buf(type);
    if (type == FCELL_TYPE)
	frast = Rast_allocate_buf(type);
    if (type == DCELL_TYPE)
	drast = Rast_allocate_buf(type);

    G_message(_("Write 2d array to raster map <%s>"), name);

    count = 0;
    for (y = 0; y < rows; y++) {
	G_percent(y, rows - 1, 10);
	for (x = 0; x < cols; x++) {
	    if (type == CELL_TYPE)
		rast[x] = N_get_array_2d_c_value(array, x, y);
	    if (type == FCELL_TYPE)
		frast[x] = N_get_array_2d_f_value(array, x, y);
	    if (type == DCELL_TYPE)
		drast[x] = N_get_array_2d_d_value(array, x, y);
	}
	if (type == CELL_TYPE)
	    Rast_put_c_row(map, rast);
	if (type == FCELL_TYPE)
	    Rast_put_f_row(map, frast);
	if (type == DCELL_TYPE)
	    Rast_put_d_row(map, drast);
    }

    /* Close file */
    Rast_close(map);
}


/* ******************** 3D ARRAY FUNCTIONS *********************** */

/*!
 * \brief Read a volume map into a N_array_3d structure
 *
 * The volume map is opened in the current region settings.
 * If no N_array_3d structure is provided (NULL pointer), a new structure will be
 * allocated with the same data type as the volume map and the size of the current region. 
 * The array offset will be set to 0.
 * <br><br>
 *
 * If a N_array_3d structure is provided, the values from the volume map are 
 * casted to the N_array_3d type. The array must have the same size 
 * as the current region. 
 * <br><br>
 *
 * The new created or the provided array is returned.
 * If the reading of the volume map fails, G3d_fatalError() will
 * be invoked.
 *
 * \param name * char - the name of an existing volume map
 * \param array * N_array_3d - an existing array or NULL
 * \param mask int - 0 = false, 1 = ture : if a mask is presenent, use it with the input volume map
 * \return N_array_3d * - the existing or new allocated array
 * */
N_array_3d *N_read_rast3d_to_array_3d(char *name, N_array_3d * array,
				      int mask)
{
    void *map = NULL;		/*The 3D Rastermap */
    int changemask = 0;
    int x, y, z, cols, rows, depths, type;
    double d1 = 0, f1 = 0;
    N_array_3d *data = array;
    RASTER3D_Region region;


    /*get the current region */
    G3d_getWindow(&region);

    cols = region.cols;
    rows = region.rows;
    depths = region.depths;


    if (NULL == G_find_grid3(name, ""))
	G3d_fatalError(_("Requested g3d map <%s> not found"), name);

    /*Open all maps with default region */
    map =
	G3d_openCellOld(name, G_find_grid3(name, ""), RASTER3D_DEFAULT_WINDOW,
			RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);

    if (map == NULL)
	G3d_fatalError(_("Error opening g3d map <%s>"), name);

    type = G3d_tileTypeMap(map);

    /*if the array is NULL create a new one with the data type of the volume map */
    /*the offset is 0 by default */
    if (data == NULL) {
	if (type == FCELL_TYPE) {
	    data = N_alloc_array_3d(cols, rows, depths, 0, FCELL_TYPE);
	}
	if (type == DCELL_TYPE) {
	    data = N_alloc_array_3d(cols, rows, depths, 0, DCELL_TYPE);
	}
    }
    else {
	/*Check the array sizes */
	if (data->cols != cols)
	    G_fatal_error
		("N_read_rast_to_array_3d: the data array size is different from the current region settings");
	if (data->rows != rows)
	    G_fatal_error
		("N_read_rast_to_array_3d: the data array size is different from the current region settings");
	if (data->depths != depths)
	    G_fatal_error
		("N_read_rast_to_array_3d: the data array size is different from the current region settings");
    }


    G_message(_("Read g3d map <%s> into the memory"), name);

    /*if requested set the Mask on */
    if (mask) {
	if (G3d_maskFileExists()) {
	    changemask = 0;
	    if (G3d_maskIsOff(map)) {
		G3d_maskOn(map);
		changemask = 1;
	    }
	}
    }

    for (z = 0; z < depths; z++) {	/*From the bottom to the top */
	G_percent(z, depths - 1, 10);
	for (y = 0; y < rows; y++) {
	    for (x = 0; x < cols; x++) {
		if (type == FCELL_TYPE) {
		    G3d_getValue(map, x, y, z, &f1, type);
		    if (Rast_is_f_null_value((void *)&f1)) {
			N_put_array_3d_value_null(data, x, y, z);
		    }
		    else {
			if (data->type == FCELL_TYPE)
			    N_put_array_3d_f_value(data, x, y, z, f1);
			if (data->type == DCELL_TYPE)
			    N_put_array_3d_d_value(data, x, y, z, (double)f1);
		    }
		}
		else {
		    G3d_getValue(map, x, y, z, &d1, type);
		    if (Rast_is_d_null_value((void *)&d1)) {
			N_put_array_3d_value_null(data, x, y, z);
		    }
		    else {
			if (data->type == FCELL_TYPE)
			    N_put_array_3d_f_value(data, x, y, z, (float)d1);
			if (data->type == DCELL_TYPE)
			    N_put_array_3d_d_value(data, x, y, z, d1);
		    }

		}
	    }
	}
    }

    /*We set the Mask off, if it was off before */
    if (mask) {
	if (G3d_maskFileExists())
	    if (G3d_maskIsOn(map) && changemask)
		G3d_maskOff(map);
    }

    /* Close files and exit */
    if (!G3d_closeCell(map))
	G3d_fatalError(map, NULL, 0, _("Error closing g3d file"));

    return data;
}

/*!
 * \brief Write a N_array_3d struct to a volume map
 *
 * A new volume map is created with the same type as the N_array_3d.
 * The current region is used to open the volume map.
 * The N_array_3d must have the same size as the current region.
 * If the writing of the volume map fails, G3d_fatalError() will
 * be invoked.
 *
 *
 * \param array N_array_3d * 
 * \param name char * - the name of the volume map
 * \param mask int - 1 = use a 3d mask, 0 do not use a 3d mask
 * \return void
 *
 * */
void N_write_array_3d_to_rast3d(N_array_3d * array, char *name, int mask)
{
    void *map = NULL;		/*The 3D Rastermap */
    int changemask = 0;
    int x, y, z, cols, rows, depths, count, type;
    double d1 = 0.0, f1 = 0.0;
    N_array_3d *data = array;
    RASTER3D_Region region;

    /*get the current region */
    G3d_getWindow(&region);

    cols = region.cols;
    rows = region.rows;
    depths = region.depths;
    type = data->type;

    /*Check the array sizes */
    if (data->cols != cols)
	G_fatal_error
	    ("N_write_array_3d_to_rast3d: the data array size is different from the current region settings");
    if (data->rows != rows)
	G_fatal_error
	    ("N_write_array_3d_to_rast3d: the data array size is different from the current region settings");
    if (data->depths != depths)
	G_fatal_error
	    ("N_write_array_3d_to_rast3d: the data array size is different from the current region settings");

    /*Open the new map */
    if (type == DCELL_TYPE)
        map = G3d_openNewOptTileSize(name, RASTER3D_USE_CACHE_XY, &region, DCELL_TYPE, 32);
    else if (type == FCELL_TYPE)
        map = G3d_openNewOptTileSize(name, RASTER3D_USE_CACHE_XY, &region, FCELL_TYPE, 32);

    if (map == NULL)
	G3d_fatalError(_("Error opening g3d map <%s>"), name);

    G_message(_("Write 3d array to g3d map <%s>"), name);

    /*if requested set the Mask on */
    if (mask) {
	if (G3d_maskFileExists()) {
	    changemask = 0;
	    if (G3d_maskIsOff(map)) {
		G3d_maskOn(map);
		changemask = 1;
	    }
	}
    }

    count = 0;
    for (z = 0; z < depths; z++) {	/*From the bottom to the top */
	G_percent(z, depths - 1, 10);
	for (y = 0; y < rows; y++) {
	    for (x = 0; x < cols; x++) {
		if (type == FCELL_TYPE) {
		    f1 = N_get_array_3d_f_value(data, x, y, z);
		    G3d_putFloat(map, x, y, z, f1);
		}
		else if (type == DCELL_TYPE) {
		    d1 = N_get_array_3d_d_value(data, x, y, z);
		    G3d_putDouble(map, x, y, z, d1);
		}
	    }
	}
    }

    /*We set the Mask off, if it was off before */
    if (mask) {
	if (G3d_maskFileExists())
	    if (G3d_maskIsOn(map) && changemask)
		G3d_maskOff(map);
    }

    /* Flush all tile */
    if (!G3d_flushAllTiles(map))
	G3d_fatalError("Error flushing tiles with G3d_flushAllTiles");
    /* Close files and exit */
    if (!G3d_closeCell(map))
	G3d_fatalError(map, NULL, 0, _("Error closing g3d file"));

    return;
}
