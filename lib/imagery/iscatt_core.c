/*!
   \file lib/imagery/iscatt_core.c

   \brief Imagery library - wx.iscatt (wx Interactive Scatter Plot Tool) backend.

   Copyright (C) 2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Stepan Turek <stepan.turek@seznam.cz> (GSoC 2013, Mentor: Martin Landa)
 */

#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "iclass_local_proto.h"

struct rast_row
{
    CELL *row;
    char *null_row;
    struct Range rast_range;	/*Range of whole raster. */
};

/*!
   \brief Create pgm header.

   Scatter plot internally generates pgm files.
   These pgms have header in format created by this function.

   \param region region to be pgm header generated for 
   \param [out] header header of pgm file
 */
static int get_cat_rast_header(struct Cell_head *region, char *header)
{
    return sprintf(header, "P5\n%d\n%d\n1\n", region->cols, region->rows);
}

/*!
   \brief Create category raster conditions file.
   The file is used for holding selected areas from mapwindow.
   The reason why the standard GRASS raster is not used is that for every 
   modification (new area is selected) we would need to create whole new raster.
   Instead of this scatter plot only modifies affected region in 
   the internal pgm file. 

   \param cat_rast_region region to be file generated for
   \param[out] cat_rast path where the pgm raster file will be generated
 */
int I_create_cat_rast(struct Cell_head *cat_rast_region, const char *cat_rast)
{
    FILE *f_cat_rast;
    char cat_rast_header[1024];	//TODO magic number 
    int i_row, i_col;
    int head_nchars;

    unsigned char *row_data;

    f_cat_rast = fopen(cat_rast, "wb");
    if (!f_cat_rast) {
	G_warning("Unable to create category raster condition file <%s>.",
		  cat_rast);
	return -1;
    }

    head_nchars = get_cat_rast_header(cat_rast_region, cat_rast_header);

    fwrite(cat_rast_header, sizeof(char), head_nchars / sizeof(char),
	   f_cat_rast);
    if (ferror(f_cat_rast)) {
	fclose(f_cat_rast);
	G_warning(_
		  ("Unable to write header into category raster condition file <%s>."),
		  cat_rast);
	return -1;
    }

    row_data =
	(unsigned char *)G_malloc(cat_rast_region->cols *
				  sizeof(unsigned char));
    for (i_col = 0; i_col < cat_rast_region->cols; i_col++)
	row_data[i_col] = 0 & 255;

    for (i_row = 0; i_row < cat_rast_region->rows; i_row++) {
	fwrite(row_data, sizeof(unsigned char),
	       (cat_rast_region->cols) / sizeof(unsigned char), f_cat_rast);
	if (ferror(f_cat_rast)) {
	    fclose(f_cat_rast);
	    G_warning(_
		      ("Unable to write into category raster condition file <%s>."),
		      cat_rast);
	    return -1;
	}
    }

    fclose(f_cat_rast);
    return 0;
}

/*!
   \brief Find intersection region of two regions.

   \param A pointer to intersected region
   \param B pointer to intersected region
   \param [out] intersec pointer to intersection region of regions A B 
   			   (relevant params of the region are: south, north, east, west)

   \return  0 if interection exists
   \return -1 if regions does not intersect
 */
static int regions_intersecion(struct Cell_head *A, struct Cell_head *B,
			       struct Cell_head *intersec)
{

    if (B->north < A->south)
	return -1;
    else if (B->north > A->north)
	intersec->north = A->north;
    else
	intersec->north = B->north;

    if (B->south > A->north)
	return -1;
    else if (B->south < A->south)
	intersec->south = A->south;
    else
	intersec->south = B->south;

    if (B->east < A->west)
	return -1;
    else if (B->east > A->east)
	intersec->east = A->east;
    else
	intersec->east = B->east;

    if (B->west > A->east)
	return -1;
    else if (B->west < A->west)
	intersec->west = A->west;
    else
	intersec->west = B->west;

    if (intersec->north == intersec->south)
	return -1;

    if (intersec->east == intersec->west)
	return -1;

    return 0;

}

/*!
   \brief Get rows and cols numbers, which defines intersection of the regions.

   \param A pointer to intersected region
   \param B pointer to intersected region (A and B must have same resolution)
   \param [out] A_bounds rows and cols numbers of A stored in 
                south, north, east, west, which defines intersection of A and B
   \param [out] B_bounds rows and cols numbers of B stored in 
                south, north, east, west, which defines intersection of A and B

   \return  0 if interection exists
   \return -1 if regions do not intersect
   \return -2 resolution of regions is not same 
 */
static int get_rows_and_cols_bounds(struct Cell_head *A, struct Cell_head *B,
				    struct Cell_head *A_bounds,
				    struct Cell_head *B_bounds)
{
    float ns_res, ew_res;

    struct Cell_head intersec;

    /* TODO is it right check? */
    if (abs(A->ns_res - B->ns_res) > GRASS_EPSILON) {
	G_warning(
		"'get_rows_and_cols_bounds' ns_res does not fit, A->ns_res: %f B->ns_res: %f",
		A->ns_res, B->ns_res);
	return -2;
    }

    if (abs(A->ew_res - B->ew_res) > GRASS_EPSILON) {
	G_warning(
		"'get_rows_and_cols_bounds' ew_res does not fit, A->ew_res: %f B->ew_res: %f",
		A->ew_res, B->ew_res);
	return -2;
    }

    ns_res = A->ns_res;
    ew_res = A->ew_res;

    if (regions_intersecion(A, B, &intersec) == -1)
	return -1;

    A_bounds->north =
	ceil((A->north - intersec.north - ns_res * 0.5) / ns_res);
    A_bounds->south =
	ceil((A->north - intersec.south - ns_res * 0.5) / ns_res);

    A_bounds->east = ceil((intersec.east - A->west - ew_res * 0.5) / ew_res);
    A_bounds->west = ceil((intersec.west - A->west - ew_res * 0.5) / ew_res);

    B_bounds->north =
	ceil((B->north - intersec.north - ns_res * 0.5) / ns_res);
    B_bounds->south =
	ceil((B->north - intersec.south - ns_res * 0.5) / ns_res);

    B_bounds->east = ceil((intersec.east - B->west - ew_res * 0.5) / ew_res);
    B_bounds->west = ceil((intersec.west - B->west - ew_res * 0.5) / ew_res);

    return 0;
}

/*!
   \brief Insert raster map patch into pgm file.
   \see I_create_cat_rast

   Warning: calls Rast_set_window

   \param patch_rast name of raster map
   \param cat_rast_region region of category raster file
   \param cat_rast path to category raster file

   \return  0 on success
   \return -1 on failure
 */
int I_insert_patch_to_cat_rast(const char *patch_rast,
			       struct Cell_head *cat_rast_region,
			       const char *cat_rast)
{

    FILE *f_cat_rast;
    struct Cell_head patch_region, patch_bounds, cat_rast_bounds;
    char cat_rast_header[1024];
    int i_row, i_col, ncols, nrows, patch_col;
    int head_nchars, ret;
    int fd_patch_rast, init_shift, step_shift;
    unsigned char *patch_data;

    char *null_chunk_row;

    const char *mapset;

    f_cat_rast = fopen(cat_rast, "rb+");
    if (!f_cat_rast) {
	G_warning(_("Unable to open category raster condtions file <%s>."),
		  cat_rast);
	return -1;
    }

    head_nchars = get_cat_rast_header(cat_rast_region, cat_rast_header);
    if ((mapset = G_find_raster((char *)patch_rast, "")) == NULL) {
	fclose(f_cat_rast);
	G_warning(_("Unable to find patch raster <%s>."), patch_rast);
	return -1;
    }

    Rast_get_cellhd(patch_rast, mapset, &patch_region);
    Rast_set_window(&patch_region);

    if ((fd_patch_rast = Rast_open_old(patch_rast, mapset)) < 0) {
	fclose(f_cat_rast);
	return -1;
    }

    ret =
	get_rows_and_cols_bounds(cat_rast_region, &patch_region,
				 &cat_rast_bounds, &patch_bounds);
    if (ret == -2) {
	G_warning(_
		  ("Resolutions of patch <%s> and patched file <%s> are not same."),
		  patch_rast, cat_rast);

	Rast_close(fd_patch_rast);
	fclose(f_cat_rast);

	return -1;
    }
    else if (ret == -1) {

	Rast_close(fd_patch_rast);
	fclose(f_cat_rast);

	return 0;
    }

    ncols = cat_rast_bounds.east - cat_rast_bounds.west;
    nrows = cat_rast_bounds.south - cat_rast_bounds.north;

    patch_data = (unsigned char *)G_malloc(ncols * sizeof(unsigned char));

    init_shift =
	head_nchars + cat_rast_region->cols * cat_rast_bounds.north +
	cat_rast_bounds.west;

    if (fseek(f_cat_rast, init_shift, SEEK_SET) != 0) {
	G_warning(_
		  ("Corrupted  category raster conditions file <%s> (fseek failed)"),
		  cat_rast);

	Rast_close(fd_patch_rast);
	fclose(f_cat_rast);

	return -1;
    }

    step_shift = cat_rast_region->cols - ncols;

    null_chunk_row = Rast_allocate_null_buf();

    for (i_row = 0; i_row < nrows; i_row++) {
	Rast_get_null_value_row(fd_patch_rast, null_chunk_row,
				i_row + patch_bounds.north);

	for (i_col = 0; i_col < ncols; i_col++) {
	    patch_col = patch_bounds.west + i_col;

	    if (null_chunk_row[patch_col] != 1)
		patch_data[i_col] = 1 & 255;
	    else {
		patch_data[i_col] = 0 & 255;
	    }
	}

	fwrite(patch_data, sizeof(unsigned char),
	       (ncols) / sizeof(unsigned char), f_cat_rast);
	if (ferror(f_cat_rast)) {
	    G_warning(_
		      ("Unable to write into category raster conditions file <%s>"),
		      cat_rast);

	    Rast_close(fd_patch_rast);
	    G_free(null_chunk_row);
	    fclose(f_cat_rast);

	    return -1;
	}
	if (fseek(f_cat_rast, step_shift, SEEK_CUR) != 0) {
	    G_warning(_
		      ("Corrupted  category raster conditions file <%s> (fseek failed)"),
		      cat_rast);

	    Rast_close(fd_patch_rast);
	    G_free(null_chunk_row);
	    fclose(f_cat_rast);

	    return -1;
	}
    }

    Rast_close(fd_patch_rast);
    G_free(null_chunk_row);
    fclose(f_cat_rast);
    return 0;
}

/*!
   \brief Updates scatter plots data in category by pixels which meets category conditions.

   \param bands_rows data represents data describig one row from raster band
   \param belongs_pix array which defines which pixels belongs to category 
          (1 value) and which not (0 value)
   \param [out] scatts pointer to scScatts struct of type SC_SCATT_DATA, 
   		which are modified according to values in belongs_pix 
   		(represents scatter plot category)
 */
static inline void update_cat_scatt_plts(struct rast_row *bands_rows,
					 unsigned short *belongs_pix,
					 struct scScatts *scatts)
{
    int i_scatt, array_idx, i_chunk_rows_pix, max_arr_idx;

    CELL *b_1_row;
    CELL *b_2_row;
    char *b_1_null_row, *b_2_null_row;
    struct rast_row b_1_rast_row, b_2_rast_row;

    struct Range b_1_range, b_2_range;
    int b_1_range_size;

    int row_size = Rast_window_cols();

    int *scatts_bands = scatts->scatts_bands;

    for (i_scatt = 0; i_scatt < scatts->n_a_scatts; i_scatt++) {
	b_1_rast_row = bands_rows[scatts_bands[i_scatt * 2]];
	b_2_rast_row = bands_rows[scatts_bands[i_scatt * 2 + 1]];

	b_1_row = b_1_rast_row.row;
	b_2_row = b_2_rast_row.row;

	b_1_null_row = b_1_rast_row.null_row;
	b_2_null_row = b_2_rast_row.null_row;

	b_1_range = b_1_rast_row.rast_range;
	b_2_range = b_2_rast_row.rast_range;

	b_1_range_size = b_1_range.max - b_1_range.min + 1;
	max_arr_idx =
	    (b_1_range.max - b_1_range.min + 1) * (b_2_range.max -
						   b_2_range.min + 1);

	for (i_chunk_rows_pix = 0; i_chunk_rows_pix < row_size;
	     i_chunk_rows_pix++) {
	    /* pixel does not belongs to scatter plot or has null value in one of the bands */
	    if (!belongs_pix[i_chunk_rows_pix] ||
		b_1_null_row[i_chunk_rows_pix] == 1 ||
		b_2_null_row[i_chunk_rows_pix] == 1)
		continue;

	    /* index in scatter plot array */
	    array_idx =
		b_1_row[i_chunk_rows_pix] - b_1_range.min +
		(b_2_row[i_chunk_rows_pix] - b_2_range.min) * b_1_range_size;

	    if (array_idx < 0 || array_idx >= max_arr_idx) {
		G_warning
		    ("Inconsistent data. Value computed for scatter plot is out of initialized range.");
		continue;
	    }

	    /* increment scatter plot value */
	    ++scatts->scatts_arr[i_scatt]->scatt_vals_arr[array_idx];
	}
    }
}

/*!
   \brief Computes scatter plots data from bands_rows.

   \param scatt_conds pointer to scScatts struct of type SC_SCATT_CONDITIONS, 
   			       where are selected areas (condtitions) stored
   \param f_cats_rasts_conds file which stores selected areas (conditions) from
                            mapwindow see I_create_cat_rast and I_insert_patch_to_cat_rast
   \param bands_rows data arrays of raster rows from analyzed raster bands 
         (all data in bands_rows and belongs_pix arrays represents same region (row))
   \param[out] scatts pointer to scScatts struct of type SC_SCATT_DATA,
               where are computed scatter plots stored
   \param[out] fd_cats_rasts array of opened raster maps,
                which every represents all selected pixels for category

   \return  0 on success
   \return -1 on failure
 */
static inline int compute_scatts_from_chunk_row(struct scCats *scatt_conds,
						FILE ** f_cats_rasts_conds,
						struct rast_row *bands_rows,
						struct scCats *scatts,
						int *fd_cats_rasts)
{

    int i_rows_pix, i_cat, i_scatt, n_pixs;
    int cat_id, scatt_plts_cat_idx, array_idx, max_arr_idx;
    char *b_1_null_row, *b_2_null_row;
    struct rast_row b_1_rast_row, b_2_rast_row;
    CELL *cat_rast_row;

    struct scScatts *scatts_conds;
    struct scScatts *scatts_scatt_plts;

    struct Range b_1_range, b_2_range;
    int b_1_range_size;

    int *scatts_bands;

    CELL *b_1_row;
    CELL *b_2_row;
    unsigned char *i_scatt_conds;

    int row_size = Rast_window_cols();

    unsigned short *belongs_pix =
	(unsigned short *)G_malloc(row_size * sizeof(unsigned short));
    unsigned char *rast_pixs =
	(unsigned char *)G_malloc(row_size * sizeof(unsigned char));
    cat_rast_row = Rast_allocate_c_buf();


    for (i_cat = 0; i_cat < scatt_conds->n_a_cats; i_cat++) {
	scatts_conds = scatt_conds->cats_arr[i_cat];

	cat_id = scatt_conds->cats_ids[i_cat];

	scatt_plts_cat_idx = scatts->cats_idxs[cat_id];
	if (scatt_plts_cat_idx < 0)
	    continue;
	scatts_scatt_plts = scatts->cats_arr[scatt_plts_cat_idx];

	G_zero(belongs_pix, row_size * sizeof(unsigned short));

	/* if category has no conditions defined, scatter plots without 
	   any constraint are computed (default scatter plots) */
	if (!scatts_conds->n_a_scatts && !f_cats_rasts_conds[i_cat]) {
	    for (i_scatt = 0; i_scatt < scatts_scatt_plts->n_a_scatts;
		 i_scatt++) {
		/* all pixels belongs */
		for (i_rows_pix = 0; i_rows_pix < row_size; i_rows_pix++)
		    belongs_pix[i_rows_pix] = 1;
	    }
	}
	/* compute belonging pixels for defined conditions */
	else {
	    scatts_bands = scatts_conds->scatts_bands;

        /* check conditions from category raster condtitions file
           (see I_create_cat_rast) */
	    if (f_cats_rasts_conds[i_cat]) {
		n_pixs =
		    fread(rast_pixs, sizeof(unsigned char),
			  (row_size) / sizeof(unsigned char),
			  f_cats_rasts_conds[i_cat]);

		if (ferror(f_cats_rasts_conds[i_cat])) {
		    G_free(rast_pixs);
		    G_free(belongs_pix);
		    G_warning(_
			      ("Unable to read from category raster condtition file."));
		    return -1;
		}
		if (n_pixs != n_pixs) {
		    G_free(rast_pixs);
		    G_free(belongs_pix);
		    G_warning(_
			      ("Invalid size of category raster conditions file."));
		    return -1;

		}

		for (i_rows_pix = 0; i_rows_pix < row_size; i_rows_pix++) {
            if (rast_pixs[i_rows_pix] != (0 & 255))
			belongs_pix[i_rows_pix] = 1;
		}
	    }

	    /* check condtions defined in scatter plots */
	    for (i_scatt = 0; i_scatt < scatts_conds->n_a_scatts; i_scatt++) {
		b_1_rast_row = bands_rows[scatts_bands[i_scatt * 2]];
		b_2_rast_row = bands_rows[scatts_bands[i_scatt * 2 + 1]];

		b_1_row = b_1_rast_row.row;
		b_2_row = b_2_rast_row.row;

		b_1_null_row = b_1_rast_row.null_row;
		b_2_null_row = b_2_rast_row.null_row;

		b_1_range = b_1_rast_row.rast_range;
		b_2_range = b_2_rast_row.rast_range;

		b_1_range_size = b_1_range.max - b_1_range.min + 1;
		max_arr_idx =
		    (b_1_range.max - b_1_range.min + 1) * (b_2_range.max -
							   b_2_range.min + 1);

		i_scatt_conds =
		    scatts_conds->scatts_arr[i_scatt]->b_conds_arr;

		for (i_rows_pix = 0; i_rows_pix < row_size; i_rows_pix++) {
		    /* pixels already belongs to category from category raster conditions 
		       file or contains null value in one of the bands */
		    if (belongs_pix[i_rows_pix] ||
			b_1_null_row[i_rows_pix] == 1 ||
			b_2_null_row[i_rows_pix] == 1)
			continue;

		    array_idx =
			b_1_row[i_rows_pix] - b_1_range.min +
			(b_2_row[i_rows_pix] -
			 b_2_range.min) * b_1_range_size;
		    if (array_idx < 0 || array_idx >= max_arr_idx) {
			G_warning(_("Data inconsistent. "
				  "Value computed for scatter plot is out of initialized range."));
			continue;
		    }
            /* pixels meets condtion defined in scatter plot ->
               belongs to scatter plot category */
		    if (i_scatt_conds[array_idx])
			belongs_pix[i_rows_pix] = 1;
		}
	    }
	}

	/* update category raster with belonging pixels */
	if (fd_cats_rasts[i_cat] >= 0) {
	    Rast_set_null_value(cat_rast_row, Rast_window_cols(), CELL_TYPE);

	    for (i_rows_pix = 0; i_rows_pix < row_size; i_rows_pix++)
		if (belongs_pix[i_rows_pix])
		    cat_rast_row[i_rows_pix] = belongs_pix[i_rows_pix];

	    Rast_put_c_row(fd_cats_rasts[i_cat], cat_rast_row);
	}

	/* update scatter plots with belonging pixels */
	update_cat_scatt_plts(bands_rows, belongs_pix, scatts_scatt_plts);
    }

    G_free(cat_rast_row);
    G_free(rast_pixs);
    G_free(belongs_pix);

    return 0;
}

/*!
   \brief Get list of bands needed to be opened for analysis from scCats struct.
 */
static void get_needed_bands(struct scCats *cats, int *b_needed_bands)
{
    /* results in b_needed_bands - array of bools - if item has value 1,
       band (defined by item index) is needed to be opened */
    int i_cat, i_scatt;

    for (i_cat = 0; i_cat < cats->n_a_cats; i_cat++) {
	for (i_scatt = 0; i_scatt < cats->cats_arr[i_cat]->n_a_scatts;
	     i_scatt++) {
	    G_debug(3, "Active scatt %d in catt %d", i_scatt, i_cat);

	    b_needed_bands[cats->cats_arr[i_cat]->scatts_bands[i_scatt * 2]] =
		1;
	    b_needed_bands[cats->cats_arr[i_cat]->
			   scatts_bands[i_scatt * 2 + 1]] = 1;
	}
    }
    return;
}

/*!
   \brief Helper function for clean up.
 */
static void free_compute_scatts_data(int *fd_bands,
				     struct rast_row *bands_rows,
				     int n_a_bands, int *bands_ids,
				     int *fd_cats_rasts,
				     FILE ** f_cats_rasts_conds, int n_a_cats)
{
    int i, band_id;

    for (i = 0; i < n_a_bands; i++) {
	band_id = bands_ids[i];
	if (band_id >= 0) {
	    Rast_close(fd_bands[i]);
	    G_free(bands_rows[band_id].row);
	    G_free(bands_rows[band_id].null_row);
	}
    }

    if (f_cats_rasts_conds)
	for (i = 0; i < n_a_cats; i++)
	    if (f_cats_rasts_conds[i])
		fclose(f_cats_rasts_conds[i]);

    if (fd_cats_rasts)
	for (i = 0; i < n_a_cats; i++)
	    if (fd_cats_rasts[i] >= 0)
		Rast_close(fd_cats_rasts[i]);

}

/*!
   \brief Compute scatter plots data.

   If category has not defined category raster condition file and no scatter plot 
   exists with condition, default/full scatter plot is computed.
   Warning: calls Rast_set_window

   \param region analysis region, beaware that all input data must be prepared for this region 
   	  (bands (their ranges), cats_rasts_conds rasters...)
   \param region function calls Rast_set_window for this region
   \param scatt_conds pointer to scScatts struct of type SC_SCATT_CONDITIONS, 
          where are stored selected areas (conditions) in scatter plots
   \param cats_rasts_conds paths to category raster conditions files representing 
    	  selected areas from mapwindow (conditions) in rasters for every category 
   \param cats_rasts_conds index in array represents corresponding category id
   \param cats_rasts_conds for manupulation with category raster conditions file 
                see also I_id_scatt_to_bands and I_insert_patch_to_cat_rast
   \param bands names of analyzed bands, order of bands is defined by their id
   \param n_bands number of bands
   \param[out] scatts pointer to scScatts struct of type SC_SCATT_DATA, 
   	       where are computed scatter plots stored
   \param[out] cats_rasts array of raster maps names for every category 
               where will be stored all selected pixels

   \return  0 on success
   \return -1 on failure
 */
int I_compute_scatts(struct Cell_head *region, struct scCats *scatt_conds,
		     const char **cats_rasts_conds, const char **bands,
		     int n_bands, struct scCats *scatts,
		     const char **cats_rasts)
{
    const char *mapset;
    char header[1024];

    int fd_cats_rasts[scatt_conds->n_a_cats];
    FILE *f_cats_rasts_conds[scatt_conds->n_a_cats];

    struct rast_row bands_rows[n_bands];

    RASTER_MAP_TYPE data_type;

    int nrows, i_band, n_a_bands, band_id;
    int i_row, head_nchars, i_cat, id_cat;

    int fd_bands[n_bands];
    int bands_ids[n_bands];
    int b_needed_bands[n_bands];

    Rast_set_window(region);

    for (i_band = 0; i_band < n_bands; i_band++)
	fd_bands[i_band] = -1;

    for (i_band = 0; i_band < n_bands; i_band++)
	bands_ids[i_band] = -1;

    if (n_bands != scatts->n_bands || n_bands != scatt_conds->n_bands)
	return -1;

    G_zero(b_needed_bands, (size_t) n_bands * sizeof(int));

    get_needed_bands(scatt_conds, &b_needed_bands[0]);
    get_needed_bands(scatts, &b_needed_bands[0]);

    n_a_bands = 0;

    /* open band rasters, which are needed for computation */
    for (band_id = 0; band_id < n_bands; band_id++) {
	if (b_needed_bands[band_id]) {
	    G_debug(3, "Opening raster no. %d with name: %s", band_id,
		    bands[band_id]);

	    if ((mapset = G_find_raster2(bands[band_id], "")) == NULL) {
		free_compute_scatts_data(fd_bands, bands_rows, n_a_bands,
					 bands_ids, NULL, NULL,
					 scatt_conds->n_a_cats);
		G_warning(_("Unbale to read find raster <%s>"),
			  bands[band_id]);
		return -1;
	    }

	    if ((fd_bands[n_a_bands] =
		 Rast_open_old(bands[band_id], mapset)) < 0) {
		free_compute_scatts_data(fd_bands, bands_rows, n_a_bands,
					 bands_ids, NULL, NULL,
					 scatt_conds->n_a_cats);
		G_warning(_("Unbale to open raster <%s>"), bands[band_id]);
		return -1;
	    }

	    data_type = Rast_get_map_type(fd_bands[n_a_bands]);
	    if (data_type != CELL_TYPE) {
		G_warning(_("Raster <%s> type is not <%s>"), bands[band_id],
			  "CELL");
		return -1;
	    }

	    bands_rows[band_id].row = Rast_allocate_c_buf();
	    bands_rows[band_id].null_row = Rast_allocate_null_buf();

	    if (Rast_read_range
		(bands[band_id], mapset,
		 &bands_rows[band_id].rast_range) != 1) {
		free_compute_scatts_data(fd_bands, bands_rows, n_a_bands,
					 bands_ids, NULL, NULL,
					 scatt_conds->n_a_cats);
		G_warning(_("Unable to read range of raster <%s>"),
			  bands[band_id]);
		return -1;
	    }

	    bands_ids[n_a_bands] = band_id;
	    ++n_a_bands;
	}
    }

    /* open category rasters condition files and category rasters */
    for (i_cat = 0; i_cat < scatts->n_a_cats; i_cat++) {
	id_cat = scatts->cats_ids[i_cat];
	if (cats_rasts[id_cat]) {
	    fd_cats_rasts[i_cat] =
		Rast_open_new(cats_rasts[id_cat], CELL_TYPE);
	}
	else
	    fd_cats_rasts[i_cat] = -1;

	if (cats_rasts_conds[id_cat]) {
	    f_cats_rasts_conds[i_cat] = fopen(cats_rasts_conds[id_cat], "r");
	    if (!f_cats_rasts_conds[i_cat]) {
		free_compute_scatts_data(fd_bands, bands_rows, n_a_bands,
					 bands_ids, fd_cats_rasts,
					 f_cats_rasts_conds,
					 scatt_conds->n_a_cats);
		G_warning(_
			  ("Unable to open category raster condtition file <%s>"),
			  bands[band_id]);
		return -1;
	    }
	}
	else
	    f_cats_rasts_conds[i_cat] = NULL;
    }

    head_nchars = get_cat_rast_header(region, header);
    for (i_cat = 0; i_cat < scatt_conds->n_a_cats; i_cat++)
	if (f_cats_rasts_conds[i_cat])
	    if (fseek(f_cats_rasts_conds[i_cat], head_nchars, SEEK_SET) != 0) {
		G_warning(_
			  ("Corrupted category raster conditions file (fseek failed)"));
		return -1;
	    }

    nrows = Rast_window_rows();

    /* analyze bands by rows */
    for (i_row = 0; i_row < nrows; i_row++) {
	for (i_band = 0; i_band < n_a_bands; i_band++) {
	    band_id = bands_ids[i_band];
	    Rast_get_c_row(fd_bands[i_band], bands_rows[band_id].row, i_row);
	    Rast_get_null_value_row(fd_bands[i_band],
				    bands_rows[band_id].null_row, i_row);
	}
	if (compute_scatts_from_chunk_row
	    (scatt_conds, f_cats_rasts_conds, bands_rows, scatts,
	     fd_cats_rasts) == -1) {
	    free_compute_scatts_data(fd_bands, bands_rows, n_a_bands,
				     			 bands_ids, fd_cats_rasts,
				     			 f_cats_rasts_conds,
				     			 scatt_conds->n_a_cats);
	    return -1;
	}

    }
    free_compute_scatts_data(fd_bands, bands_rows, n_a_bands, bands_ids,
			     			 fd_cats_rasts, f_cats_rasts_conds,
			     			 scatt_conds->n_a_cats);
    return 0;
}

/*!
   \brief Merge arrays according to opacity.
   Every pixel in array must be represented by 4 values (RGBA). 

   Implementd for speeding up of scatter plots rendering.

   \param merged_arr array which will be overlayd with overlay_arr
   \param overlay_arr array to be merged_arr overlayed with
   \param rows number of rows for the both arrays
   \param cols number of columns for the both arrays
   \param alpha transparency (0-1) of the overlay array for merging

   \return  0
 */
int I_merge_arrays(unsigned char *merged_arr, unsigned char *overlay_arr,
		   unsigned rows, unsigned cols, double alpha)
{
    unsigned int i_row, i_col, i_b;
    unsigned int row_idx, col_idx, idx;
    unsigned int c_a_i, c_a;

    for (i_row = 0; i_row < rows; i_row++) {
	row_idx = i_row * cols;
	for (i_col = 0; i_col < cols; i_col++) {
	    col_idx = 4 * (row_idx + i_col);
	    idx = col_idx + 3;

	    c_a = overlay_arr[idx] * alpha;
	    c_a_i = 255 - c_a;

	    merged_arr[idx] =
		(c_a_i * (int)merged_arr[idx] + c_a * 255) / 255;

	    for (i_b = 0; i_b < 3; i_b++) {
		idx = col_idx + i_b;
		merged_arr[idx] =
		    (c_a_i * (int)merged_arr[idx] +
		     c_a * (int)overlay_arr[idx]) / 255;
	    }
	}
    }
    return 0;
}

/*!
   \brief Apply colromap to the raster. 

   Implementd for speeding up of scatter plots rendering.

   \param vals array of values for applying the colormap
   \param vals_mask maks of vals array
   \param nvals number of items of vals_mask and vals array
   \param colmap colour map to be applied
   \param[out] col_vals output raster with applied color map (length is 4 * nvals (RGBA)) 

   \return 0
 */
int I_apply_colormap(unsigned char *vals, unsigned char *vals_mask,
		     unsigned nvals, unsigned char *colmap,
		     unsigned char *col_vals)
{
    unsigned int i_val;
    int v, i, i_cm;

    for (i_val = 0; i_val < nvals; i_val++) {
	i_cm = 4 * i_val;

	v = vals[i_val];

	if (vals_mask && vals_mask[i_val])
	    for (i = 0; i < 4; i++)
		col_vals[i_cm + i] = colmap[258 * 4 + i];
	else if (v > 255)
	    for (i = 0; i < 4; i++)
		col_vals[i_cm + i] = colmap[257 * 4 + i];
	else if (v < 0)
	    for (i = 0; i < 4; i++)
		col_vals[i_cm + i] = colmap[256 * 4 + i];
	else
	    for (i = 0; i < 4; i++) {
		col_vals[i_cm + i] = colmap[v * 4 + i];
	    }
    }
    return 0;
}

/*!
   \brief Wrapper for using of iclass perimeter rasterization by scatter plot. 
   Warning: calls Rast_set_window

   \param polygon array of polygon coordinates [x, y, x, y...]
   \param pol_n_pts number of points in the polygon array
   \param val value to be assigned to cells, which belong to plygon
   \param rast_region region of raster
   \param[out] rast raster to be pologyn rasterized in

   \return 0 on success
   \return 1 on failure
 */

int I_rasterize(double *polygon, int pol_n_pts, unsigned char val,
		struct Cell_head *rast_region, unsigned char *rast)
{
    int i;
    int x0, x1, y;
    int row, row_idx, i_col;

    IClass_perimeter perimeter;

    struct line_pnts *pol;

    pol = Vect_new_line_struct();

    for (i = 0; i < pol_n_pts; i++) {
	Vect_append_point(pol, polygon[i * 2], polygon[i * 2 + 1], 0.0);
    }

    Rast_set_window(rast_region);

    make_perimeter(pol, &perimeter, rast_region);
    for (i = 1; i < perimeter.npoints; i += 2) {
	y = perimeter.points[i].y;
	if (y != perimeter.points[i - 1].y) {
	    G_warning(_
		      ("prepare_signature: scan line %d has odd number of points."),
		      (i + 1) / 2);
	    return 1;
	}

	x0 = perimeter.points[i - 1].x;
	x1 = perimeter.points[i].x;

	if (x0 > x1) {
	    G_warning(_("signature: perimeter points out of order."));
	    return 1;
	}

	row = (rast_region->rows - y);
	if (row < 0 || row >= rast_region->rows) {
	    continue;
	}

	row_idx = rast_region->cols * row;

	for (i_col = x0; i_col <= x1; i_col++) {
	    if (i_col < 0 || i_col >= rast_region->cols) {
		continue;
	    }
	    rast[row_idx + i_col] = val;
	}
    }

    Vect_destroy_line_struct(pol);
    G_free(perimeter.points);
    return 0;
}
