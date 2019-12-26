
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      functions to assemble a linear equation system
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

/* local protos */
static int make_les_entry_2d(int i, int j, int offset_i, int offset_j,
			     int count, int pos, N_les * les,
			     G_math_spvector * spvect,
			     N_array_2d * cell_count, N_array_2d * status,
			     N_array_2d * start_val, double entry,
			     int cell_type);

static int make_les_entry_3d(int i, int j, int k, int offset_i, int offset_j,
			     int offset_k, int count, int pos, N_les * les,
			     G_math_spvector * spvect,
			     N_array_3d * cell_count, N_array_3d * status,
			     N_array_3d * start_val, double entry,
			     int cell_type);

/* *************************************************************** * 
 * ********************** N_alloc_5star ************************** * 
 * *************************************************************** */
/*!
 * \brief allocate a 5 point star data structure
 *
 * \return N_data_star *
 * */
N_data_star *N_alloc_5star(void)
{
    N_data_star *star = (N_data_star *) G_calloc(1, sizeof(N_data_star));

    star->type = N_5_POINT_STAR;
    star->count = 5;
    return star;
}

/* *************************************************************** * 
 * ********************* N_alloc_7star *************************** * 
 * *************************************************************** */
/*!
 * \brief allocate a 7 point star data structure
 *
 * \return N_data_star *
 * */
N_data_star *N_alloc_7star(void)
{
    N_data_star *star = (N_data_star *) G_calloc(1, sizeof(N_data_star));

    star->type = N_7_POINT_STAR;
    star->count = 7;
    return star;
}

/* *************************************************************** * 
 * ********************* N_alloc_9star *************************** * 
 * *************************************************************** */
/*!
 * \brief allocate a 9 point star data structure
 *
 * \return N_data_star *
 *
 * \attention The 9 point start is not yet implemented in the matrix assembling function
 *
 * */
N_data_star *N_alloc_9star(void)
{
    N_data_star *star = (N_data_star *) G_calloc(1, sizeof(N_data_star));

    star->type = N_9_POINT_STAR;
    star->count = 9;
    return star;
}

/* *************************************************************** * 
 * ********************* N_alloc_27star ************************** * 
 * *************************************************************** */
/*!
 * \brief allocate a 27 point star data structure
 *
 * \return N_data_star *
 *
 * \attention The 27 point start is not yet implemented in the matrix assembling function
 *
 * */
N_data_star *N_alloc_27star(void)
{
    N_data_star *star = (N_data_star *) G_calloc(1, sizeof(N_data_star));

    star->type = N_27_POINT_STAR;
    star->count = 27;
    return star;
}

/* *************************************************************** * 
 * ********************** N_create_5star ************************* * 
 * *************************************************************** */
/*!
 * \brief allocate and initialize a 5 point star data structure
 *
 * \param C double
 * \param W double
 * \param E double
 * \param N double
 * \param S double
 * \param V double
 * \return N_data_star *
 * */
N_data_star *N_create_5star(double C, double W, double E, double N,
			    double S, double V)
{
    N_data_star *star = N_alloc_5star();

    star->C = C;
    star->W = W;
    star->E = E;
    star->N = N;
    star->S = S;

    star->V = V;

    G_debug(5, "N_create_5star:  w %g e %g n %g s %g c %g v %g\n", star->W,
	    star->E, star->N, star->S, star->C, star->V);

    return star;
}

/* *************************************************************** * 
 * ************************* N_create_7star ********************** * 
 * *************************************************************** */
/*!
 * \brief allocate and initialize a 7 point star data structure
 *
 * \param C double
 * \param W double
 * \param E double
 * \param N double
 * \param S double
 * \param T double
 * \param B double
 * \param V double
 * \return N_data_star *
 * */
N_data_star *N_create_7star(double C, double W, double E, double N,
			    double S, double T, double B, double V)
{
    N_data_star *star = N_alloc_7star();

    star->C = C;
    star->W = W;
    star->E = E;
    star->N = N;
    star->S = S;

    star->T = T;
    star->B = B;

    star->V = V;

    G_debug(5, "N_create_7star:  w %g e %g n %g s %g t %g b %g c %g v %g\n",
	    star->W, star->E, star->N, star->S, star->T, star->B, star->C,
	    star->V);

    return star;
}

/* *************************************************************** * 
 * ************************ N_create_9star *********************** * 
 * *************************************************************** */
/*!
 * \brief allocate and initialize a 9 point star data structure
 *
 * \param C  double
 * \param W  double
 * \param E  double
 * \param N  double
 * \param S  double
 * \param NW double
 * \param SW double
 * \param NE double
 * \param SE double
 * \param V  double
 * \return N_data_star *
 * */
N_data_star *N_create_9star(double C, double W, double E, double N,
			    double S, double NW, double SW, double NE,
			    double SE, double V)
{
    N_data_star *star = N_alloc_9star();

    star->C = C;
    star->W = W;
    star->E = E;
    star->N = N;
    star->S = S;

    star->NW = NW;
    star->SW = SW;
    star->NE = NE;
    star->SE = SE;

    star->V = V;

    G_debug(5,
	    "N_create_9star:  w %g e %g n %g s %g nw %g sw %g ne %g se %g c %g v %g\n",
	    star->W, star->E, star->N, star->S, star->NW, star->SW, star->NE,
	    star->SE, star->C, star->V);

    return star;
}

/* *************************************************************** * 
 * ************************ N_create_27star *********************** * 
 * *************************************************************** */
/*!
 * \brief allocate and initialize a 27 point star data structure
 *
 * \param C  double
 * \param W  double
 * \param E  double
 * \param N  double
 * \param S  double
 * \param NW double
 * \param SW double
 * \param NE double
 * \param SE double
 * \param T  double
 * \param W_T  double
 * \param E_T  double
 * \param N_T  double
 * \param S_T  double
 * \param NW_T double
 * \param SW_T double
 * \param NE_T double
 * \param SE_T double
 * \param B  double
 * \param W_B  double
 * \param E_B  double
 * \param N_B  double
 * \param S_B  double
 * \param NW_B double
 * \param SW_B double
 * \param NE_B double
 * \param SE_B double
 * \param V  double
 * \return N_data_star *
 * */
N_data_star *N_create_27star(double C, double W, double E, double N, double S,
			     double NW, double SW, double NE, double SE,
			     double T, double W_T, double E_T, double N_T,
			     double S_T, double NW_T, double SW_T,
			     double NE_T, double SE_T, double B, double W_B,
			     double E_B, double N_B, double S_B, double NW_B,
			     double SW_B, double NE_B, double SE_B, double V)
{
    N_data_star *star = N_alloc_27star();

    star->C = C;
    star->W = W;
    star->E = E;
    star->N = N;
    star->S = S;

    star->NW = NW;
    star->SW = SW;
    star->NE = NE;
    star->SE = SE;

    star->T = T;
    star->W_T = W_T;
    star->E_T = E_T;
    star->N_T = N_T;
    star->S_T = S_T;

    star->NW_T = NW_T;
    star->SW_T = SW_T;
    star->NE_T = NE_T;
    star->SE_T = SE_T;

    star->B = B;
    star->W_B = W_B;
    star->E_B = E_B;
    star->N_B = N_B;
    star->S_B = S_B;

    star->NW_B = NW_B;
    star->SW_B = SW_B;
    star->NE_B = NE_B;
    star->SE_B = SE_B;

    star->V = V;

    G_debug(5,
	    "N_create_27star:  w %g e %g n %g s %g nw %g sw %g ne %g se %g c %g v %g\n",
	    star->W, star->E, star->N, star->S, star->NW, star->SW, star->NE,
	    star->SE, star->C, star->V);

    G_debug(5,
	    "N_create_27star:  w_t %g e_t %g n_t %g s_t %g nw_t %g sw_t %g ne_t %g se_t %g t %g \n",
	    star->W_T, star->E_T, star->N_T, star->S_T, star->NW_T,
	    star->SW_T, star->NE_T, star->SE_T, star->T);

    G_debug(5,
	    "N_create_27star:  w_b %g e_b %g n_b %g s_b %g nw_b %g sw_b %g ne_b %g se_B %g b %g\n",
	    star->W_B, star->E_B, star->N_B, star->S_B, star->NW_B,
	    star->SW_B, star->NE_B, star->SE_B, star->B);



    return star;
}


/* *************************************************************** * 
 * ****************** N_set_les_callback_3d_func ***************** * 
 * *************************************************************** */
/*!
 * \brief Set the callback function which is called while assembling the les in 3d
 *
 * \param data N_les_callback_3d *
 * \param callback_func_3d N_data_star *
 * \return void
 * */
void
N_set_les_callback_3d_func(N_les_callback_3d * data,
			   N_data_star * (*callback_func_3d) ())
{
    data->callback = callback_func_3d;
}

/* *************************************************************** * 
 * *************** N_set_les_callback_2d_func ******************** * 
 * *************************************************************** */
/*!
 * \brief Set the callback function which is called while assembling the les in 2d
 *
 * \param data N_les_callback_2d *
 * \param callback_func_2d N_data_star * 
 * \return void
 * */
void
N_set_les_callback_2d_func(N_les_callback_2d * data,
			   N_data_star * (*callback_func_2d) ())
{
    data->callback = callback_func_2d;
}

/* *************************************************************** * 
 * ************** N_alloc_les_callback_3d ************************ * 
 * *************************************************************** */
/*!
 * \brief Allocate the structure holding the callback function
 *
 * A template callback is set. Use N_set_les_callback_3d_func
 * to set up a specific function.
 *
 * \return N_les_callback_3d *
 * */
N_les_callback_3d *N_alloc_les_callback_3d(void)
{
    N_les_callback_3d *call;

    call = (N_les_callback_3d *) G_calloc(1, sizeof(N_les_callback_3d *));
    call->callback = N_callback_template_3d;

    return call;
}

/* *************************************************************** * 
 * *************** N_alloc_les_callback_2d *********************** * 
 * *************************************************************** */
/*!
 * \brief Allocate the structure holding the callback function
 *
 * A template callback is set. Use N_set_les_callback_2d_func
 * to set up a specific function.
 *
 * \return N_les_callback_2d *
 * */
N_les_callback_2d *N_alloc_les_callback_2d(void)
{
    N_les_callback_2d *call;

    call = (N_les_callback_2d *) G_calloc(1, sizeof(N_les_callback_2d *));
    call->callback = N_callback_template_2d;

    return call;
}

/* *************************************************************** * 
 * ******************** N_callback_template_3d ******************* * 
 * *************************************************************** */
/*!
 * \brief A callback template creates a 7 point star structure
 *
 * This is a template callback for mass balance calculation with 7 point stars
 * based on 3d data (g3d).
 *
 * \param data void *
 * \param geom N_geom_data *
 * \param depth int
 * \param row   int
 * \param col   int
 * \return N_data_star *
 *
 * */
N_data_star *N_callback_template_3d(void *data, N_geom_data * geom, int col,
				    int row, int depth)
{
    N_data_star *star = N_alloc_7star();

    star->E = 1 / geom->dx;
    star->W = 1 / geom->dx;
    star->N = 1 / geom->dy;
    star->S = 1 / geom->dy;
    star->T = 1 / geom->dz;
    star->B = 1 / geom->dz;
    star->C = -1 * (2 / geom->dx + 2 / geom->dy + 2 / geom->dz);
    star->V = -1;

    G_debug(5,
	    "N_callback_template_3d:  w %g e %g n %g s %g t %g b %g c %g v %g\n",
	    star->W, star->E, star->N, star->S, star->T, star->B, star->C,
	    star->V);


    return star;
}

/* *************************************************************** * 
 * ********************* N_callback_template_2d ****************** * 
 * *************************************************************** */
/*!
 * \brief A callback template creates a 9 point star structure
 *
 * This is a template callback for mass balance calculation with 9 point stars
 * based on 2d data (raster).
 *
 * \param data void *
 * \param geom N_geom_data *
 * \param row int
 * \param col int
 * \return N_data_star *
 *
 * */
N_data_star *N_callback_template_2d(void *data, N_geom_data * geom, int col,
				    int row)
{
    N_data_star *star = N_alloc_9star();

    star->E = 1 / geom->dx;
    star->NE = 1 / sqrt(geom->dx * geom->dx + geom->dy * geom->dy);
    star->SE = 1 / sqrt(geom->dx * geom->dx + geom->dy * geom->dy);
    star->W = 1 / geom->dx;
    star->NW = 1 / sqrt(geom->dx * geom->dx + geom->dy * geom->dy);
    star->SW = 1 / sqrt(geom->dx * geom->dx + geom->dy * geom->dy);
    star->N = 1 / geom->dy;
    star->S = 1 / geom->dy;
    star->C =
	-1 * (star->E + star->NE + star->SE + star->W + star->NW + star->SW +
	      star->N + star->S);
    star->V = 0;

    return star;
}

/* *************************************************************** * 
 * ******************** N_assemble_les_2d ************************ * 
 * *************************************************************** */
/*!
 * \brief Assemble a linear equation system (les) based on 2d location data (raster) and active cells
 *
 * This function calls #N_assemble_les_2d_param
 *
 */
N_les *N_assemble_les_2d(int les_type, N_geom_data * geom,
			 N_array_2d * status, N_array_2d * start_val,
			 void *data, N_les_callback_2d * call)
{
    return N_assemble_les_2d_param(les_type, geom, status, start_val, data,
				   call, N_CELL_ACTIVE);
}

/*!
 * \brief Assemble a linear equation system (les) based on 2d location data (raster) and active cells
 *
 * This function calls #N_assemble_les_2d_param
 *
 */
N_les *N_assemble_les_2d_active(int les_type, N_geom_data * geom,
				N_array_2d * status, N_array_2d * start_val,
				void *data, N_les_callback_2d * call)
{
    return N_assemble_les_2d_param(les_type, geom, status, start_val, data,
				   call, N_CELL_ACTIVE);
}

/*!
 * \brief Assemble a linear equation system (les) based on 2d location data (raster) and active and dirichlet cells
 *
 * This function calls #N_assemble_les_2d_param
 *
 */
N_les *N_assemble_les_2d_dirichlet(int les_type, N_geom_data * geom,
				   N_array_2d * status,
				   N_array_2d * start_val, void *data,
				   N_les_callback_2d * call)
{
    return N_assemble_les_2d_param(les_type, geom, status, start_val, data,
				   call, N_CELL_DIRICHLET);
}

/*!
 * \brief Assemble a linear equation system (les) based on 2d location data  (raster)
 *
 * 
 * The linear equation system type can be set to N_NORMAL_LES to create a regular
 * matrix, or to N_SPARSE_LES to create a sparse matrix. This function returns
 * a new created linear equation system which can be solved with 
 * linear equation solvers. An 2d array with start values and an 2d status array
 * must be provided as well as the location geometry and a void pointer to data 
 * passed to the callback which creates the les row entries. This callback
 * must be defined in the N_les_callback_2d strcuture.
 *
 * The creation of the les is parallelized with OpenMP. 
 * If you implement new callbacks, please make sure that the 
 * function calls are thread safe.
 *
 *
 * the les can be created in two ways, with dirichlet and similar cells and without them,
 * to spare some memory. If the les is created with dirichlet cell, the dirichlet boundary condition
 * must be added.
 *
 * \param les_type int
 * \param geom      N_geom_data*
 * \param status    N_array_2d *
 * \param start_val N_array_2d *
 * \param data void *
 * \param cell_type int  -- les assemble based on N_CELL_ACTIVE or N_CELL_DIRICHLET
 * \param call N_les_callback_2d *
 * \return N_les *
 * */
N_les *N_assemble_les_2d_param(int les_type, N_geom_data * geom,
			       N_array_2d * status, N_array_2d * start_val,
			       void *data, N_les_callback_2d * call,
			       int cell_type)
{
    int i, j, count = 0, pos = 0;
    int cell_type_count = 0;
    int **index_ij;
    N_array_2d *cell_count;
    N_les *les = NULL;

    G_debug(2,
	    "N_assemble_les_2d: starting to assemble the linear equation system");

    /* At first count the number of valid cells and save 
     * each number in a new 2d array. Those numbers are used 
     * to create the linear equation system.
     * */

    cell_count = N_alloc_array_2d(geom->cols, geom->rows, 1, CELL_TYPE);

    /* include dirichlet cells in the les */
    if (cell_type == N_CELL_DIRICHLET) {
	for (j = 0; j < geom->rows; j++) {
	    for (i = 0; i < geom->cols; i++) {
		/*use all non-inactive cells for les creation */
		if (N_CELL_INACTIVE < N_get_array_2d_c_value(status, i, j) &&
		    N_get_array_2d_c_value(status, i, j) < N_MAX_CELL_STATE)
		    cell_type_count++;
	    }
	}
    }
    /*use only active cell in the les */
    if (cell_type == N_CELL_ACTIVE) {
	for (j = 0; j < geom->rows; j++) {
	    for (i = 0; i < geom->cols; i++) {
		/*count only active cells */
		if (N_CELL_ACTIVE == N_get_array_2d_d_value(status, i, j))
		    cell_type_count++;
	    }
	}
    }

    G_debug(2, "N_assemble_les_2d: number of used cells %i\n",
	    cell_type_count);

    if (cell_type_count == 0)
	G_fatal_error
	    ("Not enough cells [%i] to create the linear equation system. Check the cell status. Only active cells (value = 1) are used to create the equation system.",
	     cell_type_count);

    /* Then allocate the memory for the linear equation system (les). 
     * Only valid cells are used to create the les. */
    index_ij = (int **)G_calloc(cell_type_count, sizeof(int *));
    for (i = 0; i < cell_type_count; i++)
	index_ij[i] = (int *)G_calloc(2, sizeof(int));

    les = N_alloc_les_Ax_b(cell_type_count, les_type);

    count = 0;

    /*count the number of cells which should be used to create the linear equation system */
    /*save the i and j indices and create a ordered numbering */
    for (j = 0; j < geom->rows; j++) {
	for (i = 0; i < geom->cols; i++) {
	    /*count every non-inactive cell */
	    if (cell_type == N_CELL_DIRICHLET) {
		if (N_CELL_INACTIVE < N_get_array_2d_c_value(status, i, j) &&
		    N_get_array_2d_c_value(status, i, j) < N_MAX_CELL_STATE) {
		    N_put_array_2d_c_value(cell_count, i, j, count);
		    index_ij[count][0] = i;
		    index_ij[count][1] = j;
		    count++;
		    G_debug(5,
			    "N_assemble_les_2d: non-inactive cells count %i at pos x[%i] y[%i]\n",
			    count, i, j);
		}
		/*count every active cell */
	    }
	    else if (N_CELL_ACTIVE == N_get_array_2d_c_value(status, i, j)) {
		N_put_array_2d_c_value(cell_count, i, j, count);
		index_ij[count][0] = i;
		index_ij[count][1] = j;
		count++;
		G_debug(5,
			"N_assemble_les_2d: active cells count %i at pos x[%i] y[%i]\n",
			count, i, j);
	    }
	}
    }

    G_debug(2, "N_assemble_les_2d: starting the parallel assemble loop");

    /* Assemble the matrix in parallel */
#pragma omp parallel for private(i, j, pos, count) schedule(static)
    for (count = 0; count < cell_type_count; count++) {
	i = index_ij[count][0];
	j = index_ij[count][1];

	/*create the entries for the */
	N_data_star *items = call->callback(data, geom, i, j);

	/* we need a sparse vector pointer anytime */
	G_math_spvector *spvect = NULL;

	/*allocate a sprase vector */
	if (les_type == N_SPARSE_LES) {
	    spvect = G_math_alloc_spvector(items->count);
	}
	/* initial conditions */
	les->x[count] = N_get_array_2d_d_value(start_val, i, j);

	/* the entry in the vector b */
	les->b[count] = items->V;

	/* pos describes the position in the sparse vector.
	 * the first entry is always the diagonal entry of the matrix*/
	pos = 0;

	if (les_type == N_SPARSE_LES) {
	    spvect->index[pos] = count;
	    spvect->values[pos] = items->C;
	}
	else {
	    les->A[count][count] = items->C;
	}
	/* western neighbour, entry is col - 1 */
	if (i > 0) {
	    pos = make_les_entry_2d(i, j, -1, 0, count, pos, les, spvect,
				    cell_count, status, start_val, items->W,
				    cell_type);
	}
	/* eastern neighbour, entry col + 1 */
	if (i < geom->cols - 1) {
	    pos = make_les_entry_2d(i, j, 1, 0, count, pos, les, spvect,
				    cell_count, status, start_val, items->E,
				    cell_type);
	}
	/* northern neighbour, entry row - 1 */
	if (j > 0) {
	    pos =
		make_les_entry_2d(i, j, 0, -1, count, pos, les, spvect,
				  cell_count, status, start_val, items->N,
				  cell_type);
	}
	/* southern neighbour, entry row + 1 */
	if (j < geom->rows - 1) {
	    pos = make_les_entry_2d(i, j, 0, 1, count, pos, les, spvect,
				    cell_count, status, start_val, items->S,
				    cell_type);
	}
	/*in case of a nine point star, we have additional entries */
	if (items->type == N_9_POINT_STAR) {
	    /* north-western neighbour, entry is col - 1 row - 1 */
	    if (i > 0 && j > 0) {
		pos = make_les_entry_2d(i, j, -1, -1, count, pos, les, spvect,
					cell_count, status, start_val,
					items->NW, cell_type);
	    }
	    /* north-eastern neighbour, entry col + 1 row - 1 */
	    if (i < geom->cols - 1 && j > 0) {
		pos = make_les_entry_2d(i, j, 1, -1, count, pos, les, spvect,
					cell_count, status, start_val,
					items->NE, cell_type);
	    }
	    /* south-western neighbour, entry is col - 1 row + 1 */
	    if (i > 0 && j < geom->rows - 1) {
		pos = make_les_entry_2d(i, j, -1, 1, count, pos, les, spvect,
					cell_count, status, start_val,
					items->SW, cell_type);
	    }
	    /* south-eastern neighbour, entry col + 1 row + 1 */
	    if (i < geom->cols - 1 && j < geom->rows - 1) {
		pos = make_les_entry_2d(i, j, 1, 1, count, pos, les, spvect,
					cell_count, status, start_val,
					items->SE, cell_type);
	    }
	}

	/*How many entries in the les */
	if (les->type == N_SPARSE_LES) {
	    spvect->cols = pos + 1;
	    G_math_add_spvector(les->Asp, spvect, count);
	}

	if (items)
	    G_free(items);
    }

    /*release memory */
    N_free_array_2d(cell_count);

    for (i = 0; i < cell_type_count; i++)
	G_free(index_ij[i]);

    G_free(index_ij);

    return les;
}

/*!
 * \brief Integrate Dirichlet or Transmission boundary conditions into the les (2s)
 *
 * Dirichlet and Transmission boundary conditions will be integrated into
 * the provided linear equation system. This is meaningful if
 * the les was created with #N_assemble_les_2d_dirichlet, because in
 * this case Dirichlet boundary conditions are not automatically included.
 *
 * The provided les will be modified:
 *
 * Ax = b will be split into Ax_u + Ax_d = b
 *
 * x_u - the unknowns
 * x_d - the Dirichlet cells
 *
 * Ax_u = b -Ax_d will be computed. Then the matrix A will be modified to
 *
 * | A_u  0 | x_u
 * |  0   I | x_d
 *
 * \param les N_les* -- the linear equation system
 * \param geom N_geom_data* -- geometrical data information
 * \param status N_array_2d* -- the status array containing the cell types
 * \param start_val N_array_2d* -- an array with start values
 * \return int -- 1 = success, 0 = failure
 * */
int N_les_integrate_dirichlet_2d(N_les * les, N_geom_data * geom,
				 N_array_2d * status, N_array_2d * start_val)
{
    int rows, cols;
    int count = 0;
    int i, j, x, y, stat;
    double *dvect1;
    double *dvect2;

    G_debug(2,
	    "N_les_integrate_dirichlet_2d: integrating the dirichlet boundary condition");

    rows = geom->rows;
    cols = geom->cols;

    /*we nned to additional vectors */
    dvect1 = (double *)G_calloc(les->cols, sizeof(double));
    dvect2 = (double *)G_calloc(les->cols, sizeof(double));

    /*fill the first one with the x vector data of Dirichlet cells */
    count = 0;
    for (y = 0; y < rows; y++) {
	for (x = 0; x < cols; x++) {
	    stat = N_get_array_2d_c_value(status, x, y);
	    if (stat > N_CELL_ACTIVE && stat < N_MAX_CELL_STATE) {
		dvect1[count] = N_get_array_2d_d_value(start_val, x, y);
		count++;
	    }
	    else if (stat == N_CELL_ACTIVE) {
		dvect1[count] = 0.0;
		count++;
	    }
	}
    }

#pragma omp parallel default(shared)
    {
	/*perform the matrix vector product and */
	if (les->type == N_SPARSE_LES)
	    G_math_Ax_sparse(les->Asp, dvect1, dvect2, les->rows);
	else
	    G_math_d_Ax(les->A, dvect1, dvect2, les->rows, les->cols);
#pragma omp for schedule (static) private(i)
	for (i = 0; i < les->cols; i++)
	    les->b[i] = les->b[i] - dvect2[i];
    }

    /*now set the Dirichlet cell rows and cols to zero and the 
     * diagonal entry to 1*/
    count = 0;
    for (y = 0; y < rows; y++) {
	for (x = 0; x < cols; x++) {
	    stat = N_get_array_2d_c_value(status, x, y);
	    if (stat > N_CELL_ACTIVE && stat < N_MAX_CELL_STATE) {
		if (les->type == N_SPARSE_LES) {
		    /*set the rows to zero */
		    for (i = 0; i < les->Asp[count]->cols; i++)
			les->Asp[count]->values[i] = 0.0;
		    /*set the cols to zero */
		    for (i = 0; i < les->rows; i++) {
			for (j = 0; j < les->Asp[i]->cols; j++) {
			    if (les->Asp[i]->index[j] == count)
				les->Asp[i]->values[j] = 0.0;
			}
		    }

		    /*entry on the diagonal */
		    les->Asp[count]->values[0] = 1.0;

		}
		else {
		    /*set the rows to zero */
		    for (i = 0; i < les->cols; i++)
			les->A[count][i] = 0.0;
		    /*set the cols to zero */
		    for (i = 0; i < les->rows; i++)
			les->A[i][count] = 0.0;

		    /*entry on the diagonal */
		    les->A[count][count] = 1.0;
		}
	    }
	    if (stat >= N_CELL_ACTIVE)
		count++;
	}
    }

    return 0;

}

/* **************************************************************** */
/* **** make an entry in the les (2d) ***************************** */
/* **************************************************************** */
int make_les_entry_2d(int i, int j, int offset_i, int offset_j, int count,
		      int pos, N_les * les, G_math_spvector * spvect,
		      N_array_2d * cell_count, N_array_2d * status,
		      N_array_2d * start_val, double entry, int cell_type)
{
    int K;
    int di = offset_i;
    int dj = offset_j;

    K = N_get_array_2d_c_value(cell_count, i + di, j + dj) -
	N_get_array_2d_c_value(cell_count, i, j);

    /* active cells build the linear equation system */
    if (cell_type == N_CELL_ACTIVE) {
	/* dirichlet or transmission cells must be handled like this */
	if (N_get_array_2d_c_value(status, i + di, j + dj) > N_CELL_ACTIVE &&
	    N_get_array_2d_c_value(status, i + di, j + dj) < N_MAX_CELL_STATE)
	    les->b[count] -=
		N_get_array_2d_d_value(start_val, i + di, j + dj) * entry;
	else if (N_get_array_2d_c_value(status, i + di, j + dj) ==
		 N_CELL_ACTIVE) {
	    if ((count + K) >= 0 && (count + K) < les->cols) {
		G_debug(5,
			" make_les_entry_2d: (N_CELL_ACTIVE) create matrix entry at row[%i] col[%i] value %g\n",
			count, count + K, entry);
		pos++;
		if (les->type == N_SPARSE_LES) {
		    spvect->index[pos] = count + K;
		    spvect->values[pos] = entry;
		}
		else {
		    les->A[count][count + K] = entry;
		}
	    }
	}
    }				/* if dirichlet cells should be used then check for all valid cell neighbours */
    else if (cell_type == N_CELL_DIRICHLET) {
	/* all valid cells */
	if (N_get_array_2d_c_value(status, i + di, j + dj) > N_CELL_INACTIVE
	    && N_get_array_2d_c_value(status, i + di,
				      j + dj) < N_MAX_CELL_STATE) {
	    if ((count + K) >= 0 && (count + K) < les->cols) {
		G_debug(5,
			" make_les_entry_2d: (N_CELL_DIRICHLET) create matrix entry at row[%i] col[%i] value %g\n",
			count, count + K, entry);
		pos++;
		if (les->type == N_SPARSE_LES) {
		    spvect->index[pos] = count + K;
		    spvect->values[pos] = entry;
		}
		else {
		    les->A[count][count + K] = entry;
		}
	    }
	}
    }

    return pos;
}


/* *************************************************************** * 
 * ******************** N_assemble_les_3d ************************ * 
 * *************************************************************** */
/*!
 * \brief Assemble a linear equation system (les) based on 3d location data (g3d) active cells
 *
 * This function calls #N_assemble_les_3d_param
 * */
N_les *N_assemble_les_3d(int les_type, N_geom_data * geom,
			 N_array_3d * status, N_array_3d * start_val,
			 void *data, N_les_callback_3d * call)
{
    return N_assemble_les_3d_param(les_type, geom, status, start_val, data,
				   call, N_CELL_ACTIVE);
}

/*!
 * \brief Assemble a linear equation system (les) based on 3d location data (g3d) active cells
 *
 * This function calls #N_assemble_les_3d_param
 * */
N_les *N_assemble_les_3d_active(int les_type, N_geom_data * geom,
				N_array_3d * status, N_array_3d * start_val,
				void *data, N_les_callback_3d * call)
{
    return N_assemble_les_3d_param(les_type, geom, status, start_val, data,
				   call, N_CELL_ACTIVE);
}

/*!
 * \brief Assemble a linear equation system (les) based on 3d location data (g3d) active and dirichlet cells
 *
 * This function calls #N_assemble_les_3d_param
 * */
N_les *N_assemble_les_3d_dirichlet(int les_type, N_geom_data * geom,
				   N_array_3d * status,
				   N_array_3d * start_val, void *data,
				   N_les_callback_3d * call)
{
    return N_assemble_les_3d_param(les_type, geom, status, start_val, data,
				   call, N_CELL_DIRICHLET);
}

/*!
 * \brief Assemble a linear equation system (les) based on 3d location data (g3d)
 *
 * The linear equation system type can be set to N_NORMAL_LES to create a regular
 * matrix, or to N_SPARSE_LES to create a sparse matrix. This function returns
 * a new created linear equation system which can be solved with 
 * linear equation solvers. An 3d array with start values and an 3d status array
 * must be provided as well as the location geometry and a void pointer to data 
 * passed to the callback which creates the les row entries. This callback
 * must be defined in the N_les_callback_3d structure.
 * 
 * The creation of the les is parallelized with OpenMP. 
 * If you implement new callbacks, please make sure that the 
 * function calls are thread safe.
 *
 * the les can be created in two ways, with dirichlet and similar cells and without them,
 * to spare some memory. If the les is created with dirichlet cell, the dirichlet boundary condition
 * must be added.
 *
 * \param les_type int
 * \param geom      N_geom_data*
 * \param status    N_array_3d *
 * \param start_val N_array_3d *
 * \param data void *
 * \param call N_les_callback_3d *
 * \param cell_type int  -- les assemble based on N_CELL_ACTIVE or N_CELL_DIRICHLET
 * \return N_les *
 * */
N_les *N_assemble_les_3d_param(int les_type, N_geom_data * geom,
			       N_array_3d * status, N_array_3d * start_val,
			       void *data, N_les_callback_3d * call,
			       int cell_type)
{
    int i, j, k, count = 0, pos = 0;
    int cell_type_count = 0;
    N_array_3d *cell_count;
    N_les *les = NULL;
    int **index_ij;

    G_debug(2,
	    "N_assemble_les_3d: starting to assemble the linear equation system");

    cell_count =
	N_alloc_array_3d(geom->cols, geom->rows, geom->depths, 1, DCELL_TYPE);

    /* First count the number of valid cells and save  
     * each number in a new 3d array. Those numbers are used 
     * to create the linear equation system.*/

    if (cell_type == N_CELL_DIRICHLET) {
	/* include dirichlet cells in the les */
	for (k = 0; k < geom->depths; k++) {
	    for (j = 0; j < geom->rows; j++) {
		for (i = 0; i < geom->cols; i++) {
		    /*use all non-inactive cells for les creation */
		    if (N_CELL_INACTIVE <
			(int)N_get_array_3d_d_value(status, i, j, k) &&
			(int)N_get_array_3d_d_value(status, i, j,
						    k) < N_MAX_CELL_STATE)
			cell_type_count++;
		}
	    }
	}
    }
    else {
	/*use only active cell in the les */
	for (k = 0; k < geom->depths; k++) {
	    for (j = 0; j < geom->rows; j++) {
		for (i = 0; i < geom->cols; i++) {
		    /*count only active cells */
		    if (N_CELL_ACTIVE
			== (int)N_get_array_3d_d_value(status, i, j, k))
			cell_type_count++;

		}
	    }
	}
    }

    G_debug(2,
	    "N_assemble_les_3d: number of  used cells %i\n", cell_type_count);

    if (cell_type_count == 0.0)
	G_fatal_error
	    ("Not enough active cells [%i] to create the linear equation system. Check the cell status. Only active cells (value = 1) are used to create the equation system.",
	     cell_type_count);

    /* allocate the memory for the linear equation system (les). 
     * Only valid cells are used to create the les. */
    les = N_alloc_les_Ax_b(cell_type_count, les_type);

    index_ij = (int **)G_calloc(cell_type_count, sizeof(int *));
    for (i = 0; i < cell_type_count; i++)
	index_ij[i] = (int *)G_calloc(3, sizeof(int));

    count = 0;
    /*count the number of cells which should be used to create the linear equation system */
    /*save the k, i and j indices and create a ordered numbering */
    for (k = 0; k < geom->depths; k++) {
	for (j = 0; j < geom->rows; j++) {
	    for (i = 0; i < geom->cols; i++) {
		if (cell_type == N_CELL_DIRICHLET) {
		    if (N_CELL_INACTIVE <
			(int)N_get_array_3d_d_value(status, i, j, k) &&
			(int)N_get_array_3d_d_value(status, i, j,
						    k) < N_MAX_CELL_STATE) {
			N_put_array_3d_d_value(cell_count, i, j, k, count);
			index_ij[count][0] = i;
			index_ij[count][1] = j;
			index_ij[count][2] = k;
			count++;
			G_debug(5,
				"N_assemble_les_3d: non-inactive cells count %i at pos x[%i] y[%i] z[%i]\n",
				count, i, j, k);
		    }
		}
		else if (N_CELL_ACTIVE ==
			 (int)N_get_array_3d_d_value(status, i, j, k)) {
		    N_put_array_3d_d_value(cell_count, i, j, k, count);
		    index_ij[count][0] = i;
		    index_ij[count][1] = j;
		    index_ij[count][2] = k;
		    count++;
		    G_debug(5,
			    "N_assemble_les_3d: active cells count %i at pos x[%i] y[%i] z[%i]\n",
			    count, i, j, k);
		}
	    }
	}
    }

    G_debug(2, "N_assemble_les_3d: starting the parallel assemble loop");

#pragma omp parallel for private(i, j, k, pos, count) schedule(static)
    for (count = 0; count < cell_type_count; count++) {
	i = index_ij[count][0];
	j = index_ij[count][1];
	k = index_ij[count][2];

	/*create the entries for the */
	N_data_star *items = call->callback(data, geom, i, j, k);

	G_math_spvector *spvect = NULL;

	/*allocate a sprase vector */
	if (les_type == N_SPARSE_LES)
	    spvect = G_math_alloc_spvector(items->count);
	/* initial conditions */

	les->x[count] = N_get_array_3d_d_value(start_val, i, j, k);

	/* the entry in the vector b */
	les->b[count] = items->V;

	/* pos describes the position in the sparse vector.
	 * the first entry is always the diagonal entry of the matrix*/
	pos = 0;

	if (les_type == N_SPARSE_LES) {
	    spvect->index[pos] = count;
	    spvect->values[pos] = items->C;
	}
	else {
	    les->A[count][count] = items->C;
	}
	/* western neighbour, entry is col - 1 */
	if (i > 0) {
	    pos =
		make_les_entry_3d(i, j, k, -1, 0, 0, count, pos, les, spvect,
				  cell_count, status, start_val, items->W,
				  cell_type);
	}
	/* eastern neighbour, entry col + 1 */
	if (i < geom->cols - 1) {
	    pos = make_les_entry_3d(i, j, k, 1, 0, 0, count, pos, les, spvect,
				    cell_count, status, start_val, items->E,
				    cell_type);
	}
	/* northern neighbour, entry row -1 */
	if (j > 0) {
	    pos =
		make_les_entry_3d(i, j, k, 0, -1, 0, count, pos, les, spvect,
				  cell_count, status, start_val, items->N,
				  cell_type);
	}
	/* southern neighbour, entry row +1 */
	if (j < geom->rows - 1) {
	    pos = make_les_entry_3d(i, j, k, 0, 1, 0, count, pos, les, spvect,
				    cell_count, status, start_val, items->S,
				    cell_type);
	}
	/*only for a 7 star entry needed */
	if (items->type == N_7_POINT_STAR || items->type == N_27_POINT_STAR) {
	    /* the upper cell (top), entry depth + 1 */
	    if (k < geom->depths - 1) {
		pos =
		    make_les_entry_3d(i, j, k, 0, 0, 1, count, pos, les,
				      spvect, cell_count, status, start_val,
				      items->T, cell_type);
	    }
	    /* the lower cell (bottom), entry depth - 1 */
	    if (k > 0) {
		pos =
		    make_les_entry_3d(i, j, k, 0, 0, -1, count, pos, les,
				      spvect, cell_count, status, start_val,
				      items->B, cell_type);
	    }
	}

	/*How many entries in the les */
	if (les->type == N_SPARSE_LES) {
	    spvect->cols = pos + 1;
	    G_math_add_spvector(les->Asp, spvect, count);
	}

	if (items)
	    G_free(items);
    }

    N_free_array_3d(cell_count);

    for (i = 0; i < cell_type_count; i++)
	G_free(index_ij[i]);

    G_free(index_ij);

    return les;
}

/*!
 * \brief Integrate Dirichlet or Transmission boundary conditions into the les (3d)
 *
 * Dirichlet and Transmission boundary conditions will be integrated into
 * the provided linear equation system. This is meaningful if
 * the les was created with #N_assemble_les_2d_dirichlet, because in
 * this case Dirichlet boundary conditions are not automatically included.
 *
 * The provided les will be modified:
 *
 * Ax = b will be split into Ax_u + Ax_d = b
 *
 * x_u - the unknowns
 * x_d - the Dirichlet cells
 *
 * Ax_u = b -Ax_d will be computed. Then the matrix A will be modified to
 *
 * | A_u  0 | x_u
 * |  0   I | x_d
 *
 * \param les N_les* -- the linear equation system
 * \param geom N_geom_data* -- geometrical data information
 * \param status N_array_2d* -- the status array containing the cell types
 * \param start_val N_array_2d* -- an array with start values
 * \return int -- 1 = success, 0 = failure
 * */
int N_les_integrate_dirichlet_3d(N_les * les, N_geom_data * geom,
				 N_array_3d * status, N_array_3d * start_val)
{
    int rows, cols, depths;
    int count = 0;
    int i, j, x, y, z, stat;
    double *dvect1;
    double *dvect2;

    G_debug(2,
	    "N_les_integrate_dirichlet_3d: integrating the dirichlet boundary condition");

    rows = geom->rows;
    cols = geom->cols;
    depths = geom->depths;

    /*we nned to additional vectors */
    dvect1 = (double *)G_calloc(les->cols, sizeof(double));
    dvect2 = (double *)G_calloc(les->cols, sizeof(double));

    /*fill the first one with the x vector data of Dirichlet cells */
    count = 0;
    for (z = 0; z < depths; z++) {
	for (y = 0; y < rows; y++) {
	    for (x = 0; x < cols; x++) {
		stat = (int)N_get_array_3d_d_value(status, x, y, z);
		if (stat > N_CELL_ACTIVE && stat < N_MAX_CELL_STATE) {
		    dvect1[count] =
			N_get_array_3d_d_value(start_val, x, y, z);
		    count++;
		}
		else if (stat == N_CELL_ACTIVE) {
		    dvect1[count] = 0.0;
		    count++;
		}
	    }
	}
    }

#pragma omp parallel default(shared)
    {
	/*perform the matrix vector product and */
	if (les->type == N_SPARSE_LES)
	    G_math_Ax_sparse(les->Asp, dvect1, dvect2, les->rows);
	else
	    G_math_d_Ax(les->A, dvect1, dvect2, les->rows, les->cols);
#pragma omp for schedule (static) private(i)
	for (i = 0; i < les->cols; i++)
	    les->b[i] = les->b[i] - dvect2[i];
    }

    /*now set the Dirichlet cell rows and cols to zero and the 
     * diagonal entry to 1*/
    count = 0;
    for (z = 0; z < depths; z++) {
	for (y = 0; y < rows; y++) {
	    for (x = 0; x < cols; x++) {
		stat = (int)N_get_array_3d_d_value(status, x, y, z);
		if (stat > N_CELL_ACTIVE && stat < N_MAX_CELL_STATE) {
		    if (les->type == N_SPARSE_LES) {
			/*set the rows to zero */
			for (i = 0; i < les->Asp[count]->cols; i++)
			    les->Asp[count]->values[i] = 0.0;
			/*set the cols to zero */
			for (i = 0; i < les->rows; i++) {
			    for (j = 0; j < les->Asp[i]->cols; j++) {
				if (les->Asp[i]->index[j] == count)
				    les->Asp[i]->values[j] = 0.0;
			    }
			}

			/*entry on the diagonal */
			les->Asp[count]->values[0] = 1.0;

		    }
		    else {
			/*set the rows to zero */
			for (i = 0; i < les->cols; i++)
			    les->A[count][i] = 0.0;
			/*set the cols to zero */
			for (i = 0; i < les->rows; i++)
			    les->A[i][count] = 0.0;

			/*entry on the diagonal */
			les->A[count][count] = 1.0;
		    }
		}
		count++;
	    }
	}
    }

    return 0;

}

/* **************************************************************** */
/* **** make an entry in the les (3d) ***************************** */
/* **************************************************************** */
int make_les_entry_3d(int i, int j, int k, int offset_i, int offset_j,
		      int offset_k, int count, int pos, N_les * les,
		      G_math_spvector * spvect, N_array_3d * cell_count,
		      N_array_3d * status, N_array_3d * start_val,
		      double entry, int cell_type)
{
    int K;
    int di = offset_i;
    int dj = offset_j;
    int dk = offset_k;

    K = (int)N_get_array_3d_d_value(cell_count, i + di, j + dj, k + dk) -
	(int)N_get_array_3d_d_value(cell_count, i, j, k);

    if (cell_type == N_CELL_ACTIVE) {
	if ((int)N_get_array_3d_d_value(status, i + di, j + dj, k + dk) >
	    N_CELL_ACTIVE &&
	    (int)N_get_array_3d_d_value(status, i + di, j + dj,
					k + dk) < N_MAX_CELL_STATE)
	    les->b[count] -=
		N_get_array_3d_d_value(start_val, i + di, j + dj,
				       k + dk) * entry;
	else if ((int)N_get_array_3d_d_value(status, i + di, j + dj, k + dk)
		 == N_CELL_ACTIVE) {
	    if ((count + K) >= 0 && (count + K) < les->cols) {
		G_debug(5,
			" make_les_entry_3d: (N_CELL_ACTIVE) create matrix entry at row[%i] col[%i] value %g\n",
			count, count + K, entry);
		pos++;
		if (les->type == N_SPARSE_LES) {
		    spvect->index[pos] = count + K;
		    spvect->values[pos] = entry;
		}
		else {
		    les->A[count][count + K] = entry;
		}
	    }
	}
    }
    else if (cell_type == N_CELL_DIRICHLET) {
	if ((int)N_get_array_3d_d_value(status, i + di, j + dj, k + dk)
	    != N_CELL_INACTIVE) {
	    if ((count + K) >= 0 && (count + K) < les->cols) {
		G_debug(5,
			" make_les_entry_3d: (N_CELL_DIRICHLET) create matrix entry at row[%i] col[%i] value %g\n",
			count, count + K, entry);
		pos++;
		if (les->type == N_SPARSE_LES) {
		    spvect->index[pos] = count + K;
		    spvect->values[pos] = entry;
		}
		else {
		    les->A[count][count + K] = entry;
		}
	    }
	}
    }

    return pos;
}
