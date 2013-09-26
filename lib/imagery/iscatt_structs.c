/*!
   \file lib/imagery/iscatt_structs.c

   \brief Imagery library - functions for manipulation with structures used
          by wx.iscatt (wx Interactive Scatter Plot Tool)

   Copyright (C) 2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Stepan Turek <stepan.turek@seznam.cz> (GSoC 2013, Mentor: Martin Landa)
 */
#include <math.h>

#include <grass/imagery.h>
#include <grass/gis.h>

/*!
   \brief Compute band ids from scatter plot id.

   Scatter plot id describes which bands defines the scatter plot.

   Let say we have 3 bands, their ids are 0, 1 and 2.
   Scatter plot with id 0 consists of band 1 (b_1_id) 0 and  band 2 (b_2_id) 1.
   All scatter plots:
   scatt_id b_1_id b_2_id
   0        0      1
   1        0      2
   2        1      2

   \param scatt_id scatter plot id
   \param n_bands number of bands
   \param[out] b_1_id id of band1
   \param[out] b_2_id id of band2

   \return 0
 */
int I_id_scatt_to_bands(const int scatt_id, const int n_bands, int *b_1_id,
			int *b_2_id)
{
    int n_b1 = n_bands - 1;

    *b_1_id =
	(int)((2 * n_b1 + 1 -
	       sqrt((double)((2 * n_b1 + 1) * (2 * n_b1 + 1) - 8 * scatt_id)))
	      / 2);

    *b_2_id =
	scatt_id - ((*b_1_id) * (2 * n_b1 + 1) - (*b_1_id) * (*b_1_id)) / 2 +
	(*b_1_id) + 1;

    return 0;
}


/*!
   \brief Compute scatter plot id from band ids.

   See also I_id_scatt_to_bands

   \param b_1_id id of band1
   \param b_1_id id of band2
   \param n_bands number of bands
   \param[out] scatt_id scatter plot id

   \return 0
 */
int I_bands_to_id_scatt(const int b_1_id, const int b_2_id, const int n_bands,
			int *scatt_id)
{
    int n_b1 = n_bands - 1;

    *scatt_id =
	(b_1_id * (2 * n_b1 + 1) - b_1_id * b_1_id) / 2 + b_2_id - b_1_id - 1;

    return 0;
}

/*!
   \brief Initialize structure for storing scatter plots data.

   \param cats pointer to scCats struct 
   \param n_bands number of bands
   \param type SC_SCATT_DATA - stores scatter plots 
   \param type SC_SCATT_CONDITIONS - stores selected areas in scatter plots
 */
void I_sc_init_cats(struct scCats *cats, int n_bands, int type)
{
    int i_cat;

    cats->type = type;

    cats->n_cats = 100;
    cats->n_a_cats = 0;

    cats->n_bands = n_bands;
    cats->n_scatts = (n_bands - 1) * n_bands / 2;

    cats->cats_arr =
	(struct scScatts **)G_malloc(cats->n_cats *
				     sizeof(struct scScatts *));
    G_zero(cats->cats_arr, cats->n_cats * sizeof(struct scScatts *));

    cats->cats_ids = (int *)G_malloc(cats->n_cats * sizeof(int));
    cats->cats_idxs = (int *)G_malloc(cats->n_cats * sizeof(int));

    for (i_cat = 0; i_cat < cats->n_cats; i_cat++)
	cats->cats_idxs[i_cat] = -1;

    return;
}

/*!
   \brief Free data of struct scCats, the structure itself remains alocated.

   \param cats pointer to existing scCats struct
 */
void I_sc_free_cats(struct scCats *cats)
{
    int i_cat;

    for (i_cat = 0; i_cat < cats->n_a_cats; i_cat++) {
	if (cats->cats_arr[i_cat]) {
	    G_free(cats->cats_arr[i_cat]->scatt_idxs);
	    G_free(cats->cats_arr[i_cat]->scatts_bands);
	    G_free(cats->cats_arr[i_cat]->scatts_arr);
	    G_free(cats->cats_arr[i_cat]);
	}
    }

    G_free(cats->cats_ids);
    G_free(cats->cats_idxs);
    G_free(cats->cats_arr);

    cats->n_cats = 0;
    cats->n_a_cats = 0;
    cats->n_bands = 0;
    cats->n_scatts = 0;
    cats->type = -1;

    return;
}

/*!
   \brief Add category.

   Category represents group of scatter plots.

   \param cats pointer to scCats struct

   \return assigned category id (starts with 0)
   \return -1 if maximum nuber of categories was reached
 */
int I_sc_add_cat(struct scCats *cats)
{
    int i_scatt, i_cat_id, cat_id;
    int n_a_cats = cats->n_a_cats;

    if (cats->n_a_cats >= cats->n_cats)
	return -1;

    for (i_cat_id = 0; i_cat_id < cats->n_cats; i_cat_id++)
	if (cats->cats_idxs[i_cat_id] < 0) {
	    cat_id = i_cat_id;
	    break;
	}

    cats->cats_ids[n_a_cats] = cat_id;
    cats->cats_idxs[cat_id] = n_a_cats;

    cats->cats_arr[n_a_cats] =
	(struct scScatts *)G_malloc(sizeof(struct scScatts));

    cats->cats_arr[n_a_cats]->scatts_arr =
	(struct scdScattData **)G_malloc(cats->n_scatts *
					 sizeof(struct scdScattData *));
    G_zero((cats->cats_arr[n_a_cats]->scatts_arr),
	   cats->n_scatts * sizeof(struct scdScattData *));

    cats->cats_arr[n_a_cats]->n_a_scatts = 0;

    cats->cats_arr[n_a_cats]->scatts_bands =
	(int *)G_malloc(cats->n_scatts * 2 * sizeof(int));

    cats->cats_arr[n_a_cats]->scatt_idxs =
	(int *)G_malloc(cats->n_scatts * sizeof(int));
    for (i_scatt = 0; i_scatt < cats->n_scatts; i_scatt++)
	cats->cats_arr[n_a_cats]->scatt_idxs[i_scatt] = -1;

    ++cats->n_a_cats;

    return cat_id;
}

/*!
   \brief Insert scatter plot data .
   Inserted scatt_data struct must have same type as
   cats struct (SC_SCATT_DATA or SC_SCATT_CONDITIONS).

   \param cats pointer to scCats struct
   \param scarr_data pointer to scdScattData struct
   \param cat_id id number of category
   \param scatt_id id number of scatter plot

   \return  0 on success
   \return -1 on failure
 */
int I_sc_insert_scatt_data(struct scCats *cats,
			   struct scdScattData *scatt_data, int cat_id,
			   int scatt_id)
{
    int band_1, band_2, cat_idx, n_a_scatts;
    struct scScatts *scatts;

    if (cat_id < 0 || cat_id >= cats->n_cats)
	return -1;

    cat_idx = cats->cats_idxs[cat_id];
    if (cat_idx < 0)
	return -1;

    if (scatt_id < 0 && scatt_id >= cats->n_scatts)
	return -1;

    scatts = cats->cats_arr[cat_idx];
    if (scatts->scatt_idxs[scatt_id] >= 0)
	return -1;

    if (!scatt_data->b_conds_arr && cats->type == SC_SCATT_CONDITIONS)
	return -1;

    if (!scatt_data->scatt_vals_arr && cats->type == SC_SCATT_DATA)
	return -1;

    n_a_scatts = scatts->n_a_scatts;

    scatts->scatt_idxs[scatt_id] = n_a_scatts;

    I_id_scatt_to_bands(scatt_id, cats->n_bands, &band_1, &band_2);

    scatts->scatts_bands[n_a_scatts * 2] = band_1;
    scatts->scatts_bands[n_a_scatts * 2 + 1] = band_2;

    scatts->scatts_arr[n_a_scatts] = scatt_data;
    ++scatts->n_a_scatts;

    return 0;
}

/*!
   \brief Insert scatter plot data.

   \param scatt_data pointer to existing struct scdScattData
   \param type SC_SCATT_DATA for scatter plots or
               SC_SCATT_CONDITIONS for selected areas in scatter plot
   \param n_vals number of data values
   \param data array of values (unsigned char for SC_SCATT_CONDITIONS,
          unsigned int for SC_SCATT_DATA)
 */
void I_scd_init_scatt_data(struct scdScattData *scatt_data, int type,
			   int n_vals, void *data)
{
    scatt_data->n_vals = n_vals;

    if (type == SC_SCATT_DATA) {
	if (data)
	    scatt_data->scatt_vals_arr = (unsigned int *)data;
	else {
	    scatt_data->scatt_vals_arr =
		(unsigned int *)G_malloc(n_vals * sizeof(unsigned int));
        G_zero(scatt_data->scatt_vals_arr,
		   n_vals * sizeof(unsigned int));
	}
	scatt_data->b_conds_arr = NULL;
    }
    else if (type == SC_SCATT_CONDITIONS) {
	if (data)
	    scatt_data->b_conds_arr = (unsigned char *)data;
	else {
	    scatt_data->b_conds_arr =
		(unsigned char *)G_malloc(n_vals * sizeof(unsigned char));
        G_zero(scatt_data->b_conds_arr,
		   n_vals * sizeof(unsigned char));
	}
	scatt_data->scatt_vals_arr = NULL;
    }

    return;
}
