
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:     	gradient management functions 
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/N_pde.h>

/*!
 * \brief Allocate a N_gradient_2d structure
 *
 * \return N_gradient_2d *
 *
 * */
N_gradient_2d *N_alloc_gradient_2d(void)
{
    N_gradient_2d *grad;

    grad = (N_gradient_2d *) G_calloc(1, sizeof(N_gradient_2d));

    return grad;
}

/*!
 * \brief Free's a N_gradient_2d structure
 *
 * \return void
 *
 * */
void N_free_gradient_2d(N_gradient_2d * grad)
{
    G_free(grad);
    grad = NULL;

    return;
}

/*!
 * \brief allocate and initialize a N_gradient_2d structure
 *
 * \param NC double - the gradient between northern and center cell
 * \param SC double - the gradient between southern and center cell
 * \param WC double - the gradient between western and center cell
 * \param EC double - the gradient between eastern and center cell
 * \return N_gradient_2d *
 *
 * */
N_gradient_2d *N_create_gradient_2d(double NC, double SC, double WC,
				    double EC)
{
    N_gradient_2d *grad;

    G_debug(5, "N_create_gradient_2d: create N_gradient_2d");

    grad = N_alloc_gradient_2d();

    grad->NC = NC;
    grad->SC = SC;
    grad->WC = WC;
    grad->EC = EC;

    return grad;
}

/*!
 * \brief copy a N_gradient_2d structure
 *
 * \param source - the source N_gradient_2d struct
 * \param target - the target N_gradient_2d struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int N_copy_gradient_2d(N_gradient_2d * source, N_gradient_2d * target)
{
    G_debug(5, "N_copy_gradient_2d: copy N_gradient_2d");

    if (!source || !target)
	return 0;

    target->NC = source->NC;
    target->SC = source->SC;
    target->WC = source->WC;
    target->EC = source->EC;

    return 1;
}

/*!
 * \brief Return a N_gradient_2d structure calculated from the input gradient field
 * at position [row][col]
 *
 *  This function returns the gradient of a cell at position [row][col] from the input gradient field.
 *  Returend is a new structure of type N_gradient_2d.
 *
 *  \param field N_gradient_field_2d * - A two dimensional gradient field
 *  \param gradient N_gradient_2d * - the gradient structure which should be filled with data, if a NULL pointer is given, a new structure will be created
 *  \param col int
 *  \param row int
 *  \return N_gradient_2d * - the new or filled gradient structure
 *  
 *
 * */
N_gradient_2d *N_get_gradient_2d(N_gradient_field_2d * field,
				 N_gradient_2d * gradient, int col, int row)
{
    double NC = 0, SC = 0, WC = 0, EC = 0;
    N_gradient_2d *grad = gradient;


    NC = N_get_array_2d_d_value(field->y_array, col, row);
    SC = N_get_array_2d_d_value(field->y_array, col, row + 1);
    WC = N_get_array_2d_d_value(field->x_array, col, row);
    EC = N_get_array_2d_d_value(field->x_array, col + 1, row);

    G_debug(5,
	    "N_get_gradient_2d: calculate N_gradient_2d NC %g SC %g WC %g EC %g",
	    NC, SC, WC, EC);

    /*if gradient is a NULL pointer, create a new one */
    if (!grad) {
	grad = N_create_gradient_2d(NC, SC, WC, EC);
    }
    else {
	grad->NC = NC;
	grad->SC = SC;
	grad->WC = WC;
	grad->EC = EC;
    }

    return grad;
}

/*!
 * \brief Allocate a N_gradient_3d structure
 *
 * \return N_gradient_3d *
 *
 * */
N_gradient_3d *N_alloc_gradient_3d(void)
{
    N_gradient_3d *grad;

    grad = (N_gradient_3d *) G_calloc(1, sizeof(N_gradient_3d));

    return grad;
}

/*!
 * \brief Free's a N_gradient_3d structure
 *
 * \return void
 *
 * */
void N_free_gradient_3d(N_gradient_3d * grad)
{
    G_free(grad);
    grad = NULL;

    return;
}


/*!
 * \brief allocate and initialize a N_gradient_3d structure
 *
 * \param NC double - the gradient between northern and center cell
 * \param SC double - the gradient between southern and center cell
 * \param WC double - the gradient between western and center cell
 * \param EC double - the gradient between eastern and center cell
 * \param TC double - the gradient between top and center cell
 * \param BC double - the gradient between bottom and center cell
 * \return N_gradient_3d *
 *
 * */
N_gradient_3d *N_create_gradient_3d(double NC, double SC, double WC,
				    double EC, double TC, double BC)
{
    N_gradient_3d *grad;

    G_debug(5, "N_create_gradient_3d: create N_gradient_3d");

    grad = N_alloc_gradient_3d();

    grad->NC = NC;
    grad->SC = SC;
    grad->WC = WC;
    grad->EC = EC;
    grad->TC = TC;
    grad->BC = BC;

    return grad;
}

/*!
 * \brief copy a N_gradient_3d structure
 *
 * \param source - the source N_gradient_3d struct
 * \param target - the target N_gradient_3d struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int N_copy_gradient_3d(N_gradient_3d * source, N_gradient_3d * target)
{
    G_debug(5, "N_copy_gradient_3d: copy N_gradient_3d");

    if (!source || !target)
	return 0;

    target->NC = source->NC;
    target->SC = source->SC;
    target->WC = source->WC;
    target->EC = source->EC;
    target->TC = source->TC;
    target->BC = source->BC;

    return 1;
}


/*!
 * \brief Return a N_gradient_3d structure calculated from the input gradient field
 * at position [depth][row][col]
 *
 *  This function returns the gradient of a 3d cell at position [depth][row][col] from the input gradient field.
 *  Returned is a new structure of type N_gradient_3d.
 *
 *  \param field N_gradient_field_3d * - A three dimensional gradient field
 *  \param gradient N_gradient_3d * - an existing gradient structure or a NULL pointer, if a NULL pointer is providet a new structure will be returned
 *  \param col int
 *  \param row int
 *  \param depth int
 *  \return N_gradient_3d *
 *  
 *
 * */
N_gradient_3d *N_get_gradient_3d(N_gradient_field_3d * field,
				 N_gradient_3d * gradient, int col, int row,
				 int depth)
{
    double NC, SC, WC, EC, TC, BC;
    N_gradient_3d *grad = gradient;

    NC = N_get_array_3d_d_value(field->y_array, col, row, depth);
    SC = N_get_array_3d_d_value(field->y_array, col, row + 1, depth);
    WC = N_get_array_3d_d_value(field->x_array, col, row, depth);
    EC = N_get_array_3d_d_value(field->x_array, col + 1, row, depth);
    BC = N_get_array_3d_d_value(field->z_array, col, row, depth);
    TC = N_get_array_3d_d_value(field->z_array, col, row, depth + 1);

    G_debug(6,
	    "N_get_gradient_3d: calculate N_gradient_3d NC %g SC %g WC %g EC %g TC %g BC %g",
	    NC, SC, WC, EC, TC, BC);

    /*if gradient is a NULL pointer, create a new one */
    if (!grad) {
	grad = N_create_gradient_3d(NC, SC, WC, EC, TC, BC);
    }
    else {
	grad->NC = NC;
	grad->SC = SC;
	grad->WC = WC;
	grad->EC = EC;
	grad->BC = BC;
	grad->TC = TC;
    }

    return grad;
}

/*!
 * \brief Allocate a N_gradient_neighbours_x structure
 *
 * This structure contains all neighbour gradients in x direction of one cell  
 *
 * \return N_gradient_neighbours_x  *
 *
 * */
N_gradient_neighbours_x *N_alloc_gradient_neighbours_x(void)
{
    N_gradient_neighbours_x *grad;

    grad =
	(N_gradient_neighbours_x *) G_calloc(1,
					     sizeof(N_gradient_neighbours_x));

    return grad;
}

/*!
 * \brief Free's a N_gradient_neighbours_x structure
 *
 * \return void
 *
 * */
void N_free_gradient_neighbours_x(N_gradient_neighbours_x * grad)
{
    G_free(grad);
    grad = NULL;

    return;
}


/*!
 * \brief Allocate and initialize a N_gradient_neighbours_x structure
 *
 * \param NWN double - the gradient between north-west and northern cell
 * \param NEN double - the gradient between north-east and northern cell
 * \param WC double - the gradient between western and center cell
 * \param EC double - the gradient between eastern and center cell
 * \param SWS double - the gradient between south-west and southern cell
 * \param SES double - the gradient between south-east and southern cell
 * \return N_gradient_neighbours_x *

 *
 * */
N_gradient_neighbours_x *N_create_gradient_neighbours_x(double NWN,
							double NEN, double WC,
							double EC, double SWS,
							double SES)
{
    N_gradient_neighbours_x *grad;

    G_debug(6,
	    "N_create_gradient_neighbours_x: create N_gradient_neighbours_x");

    grad = N_alloc_gradient_neighbours_x();

    grad->NWN = NWN;
    grad->NEN = NEN;
    grad->WC = WC;
    grad->EC = EC;
    grad->SWS = SWS;
    grad->SES = SES;

    return grad;
}

/*!
 * \brief copy a N_gradient_neighbours_x structure
 *
 * \param source - the source N_gradient_neighbours_x struct
 * \param target - the target N_gradient_neighbours_x struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int
N_copy_gradient_neighbours_x(N_gradient_neighbours_x * source,
			     N_gradient_neighbours_x * target)
{
    G_debug(6, "N_copy_gradient_neighbours_x: copy N_gradient_neighbours_x");

    if (!source || !target)
	return 0;

    target->NWN = source->NWN;
    target->NEN = source->NEN;
    target->WC = source->WC;
    target->EC = source->EC;
    target->SWS = source->SWS;
    target->SES = source->SES;

    return 1;
}

/*!
 * \brief Allocate a N_gradient_neighbours_y structure
 *
 * This structure contains all neighbour gradients in y direction of one cell  
 *
 * \return N_gradient_neighbours_y  *
 *
 * */
N_gradient_neighbours_y *N_alloc_gradient_neighbours_y(void)
{
    N_gradient_neighbours_y *grad;

    grad =
	(N_gradient_neighbours_y *) G_calloc(1,
					     sizeof(N_gradient_neighbours_y));

    return grad;
}

/*!
 * \brief Free's a N_gradient_neighbours_y structure
 *
 * \return void
 *
 * */
void N_free_gradient_neighbours_y(N_gradient_neighbours_y * grad)
{
    G_free(grad);
    grad = NULL;

    return;
}

/*!
 * \brief Allocate and initialize a N_gradient_neighbours_y structure
 *
 * \param NWW double - the gradient between north-west and western cell
 * \param NEE double - the gradient between north-east and eastern cell
 * \param NC double - the gradient between northern and center cell
 * \param SC double - the gradient between southern and center cell
 * \param SWW double - the gradient between south-west and western cell
 * \param SEE double - the gradient between south-east and eastern cell
 * \return N_gradient_neighbours_y *

 *
 * */
N_gradient_neighbours_y *N_create_gradient_neighbours_y(double NWW,
							double NEE, double NC,
							double SC, double SWW,
							double SEE)
{
    N_gradient_neighbours_y *grad;

    G_debug(6,
	    "N_create_gradient_neighbours_y: create N_gradient_neighbours_y");

    grad = N_alloc_gradient_neighbours_y();

    grad->NWW = NWW;
    grad->NEE = NEE;
    grad->NC = NC;
    grad->SC = SC;
    grad->SWW = SWW;
    grad->SEE = SEE;

    return grad;
}

/*!
 * \brief copy a N_gradient_neighbours_y structure
 *
 * \param source - the source N_gradient_neighbours_y struct
 * \param target - the target N_gradient_neighbours_y struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int
N_copy_gradient_neighbours_y(N_gradient_neighbours_y * source,
			     N_gradient_neighbours_y * target)
{
    G_debug(6, "N_copy_gradient_neighbours_y: copy N_gradient_neighbours_y");

    if (!source || !target)
	return 0;

    target->NWW = source->NWW;
    target->NEE = source->NEE;
    target->NC = source->NC;
    target->SC = source->SC;
    target->SWW = source->SWW;
    target->SEE = source->SEE;

    return 1;
}

/*!
 * \brief Allocate a N_gradient_neighbours_z structure
 *
 * This structure contains all neighbour gradients in z direction of one cell  
 *
 * \return N_gradient_neighbours_z  *
 *
 * */
N_gradient_neighbours_z *N_alloc_gradient_neighbours_z(void)
{
    N_gradient_neighbours_z *grad;

    grad =
	(N_gradient_neighbours_z *) G_calloc(1,
					     sizeof(N_gradient_neighbours_z));

    return grad;
}

/*!
 * \brief Free's a N_gradient_neighbours_z structure
 *
 * \return void
 *
 * */
void N_free_gradient_neighbours_z(N_gradient_neighbours_z * grad)
{
    G_free(grad);
    grad = NULL;

    return;
}

/*!
 * \brief Allocate and initialize a N_gradient_neighbours_z structure
 *
 * \param NWZ double - the gradient between upper and lower north-western cells
 * \param NZ double - the gradient between upper and lower northern cells
 * \param NEZ double - the gradient between upper and lower north-eastern cells
 * \param WZ double - the gradient between upper and lower western cells
 * \param CZ double - the gradient between upper and lower center cells
 * \param EZ double - the gradient between upper and lower eastern cells
 * \param SWZ double - the gradient between upper and lower south-western cells
 * \param SZ double - the gradient between upper and lower southern cells
 * \param SEZ double - the gradient between upper and lower south-eastern cells
 * \return N_gradient_neighbours_z *

 *
 * */
N_gradient_neighbours_z *N_create_gradient_neighbours_z(double NWZ, double NZ,
							double NEZ, double WZ,
							double CZ, double EZ,
							double SWZ, double SZ,
							double SEZ)
{
    N_gradient_neighbours_z *grad;

    G_debug(6,
	    "N_create_gradient_neighbours_z: create N_gradient_neighbours_z");

    grad = N_alloc_gradient_neighbours_z();

    grad->NWZ = NWZ;
    grad->NZ = NZ;
    grad->NEZ = NEZ;
    grad->WZ = WZ;
    grad->CZ = CZ;
    grad->EZ = EZ;
    grad->SWZ = SWZ;
    grad->SZ = SZ;
    grad->SEZ = SEZ;

    return grad;
}

/*!
 * \brief copy a N_gradient_neighbours_z structure
 *
 * \param source - the source N_gradient_neighbours_z struct
 * \param target - the target N_gradient_neighbours_z struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int
N_copy_gradient_neighbours_z(N_gradient_neighbours_z * source,
			     N_gradient_neighbours_z * target)
{
    G_debug(6, "N_copy_gradient_neighbours_z: copy N_gradient_neighbours_z");

    if (!source || !target)
	return 0;

    target->NWZ = source->NWZ;
    target->NZ = source->NZ;
    target->NEZ = source->NEZ;
    target->WZ = source->WZ;
    target->CZ = source->CZ;
    target->EZ = source->EZ;
    target->SWZ = source->SWZ;
    target->SZ = source->SZ;
    target->SEZ = source->SEZ;

    return 1;
}

/*!
 * \brief Allocate a N_gradient_neighbours_2d structure
 *
 * This structure contains all neighbour gradients in all directions of one cell 
 * in a 2d raster layer
 *
 * \return N_gradient_neighbours_2d *
 *
 * */
N_gradient_neighbours_2d *N_alloc_gradient_neighbours_2d(void)
{
    N_gradient_neighbours_2d *grad;

    grad =
	(N_gradient_neighbours_2d *) G_calloc(1,
					      sizeof
					      (N_gradient_neighbours_2d));

    grad->x = N_alloc_gradient_neighbours_x();
    grad->y = N_alloc_gradient_neighbours_y();

    return grad;
}

/*!
 * \brief Free's a N_gradient_neighbours_2d structure
 *
 * \return void
 *
 * */
void N_free_gradient_neighbours_2d(N_gradient_neighbours_2d * grad)
{

    N_free_gradient_neighbours_x(grad->x);
    N_free_gradient_neighbours_y(grad->y);

    G_free(grad);
    grad = NULL;

    return;
}

/*!
 * \brief Allocate and initialize a N_gradient_neighbours_2d structure
 *
 * The parameter N_gradient_neighbours x and y are copied into the new allocated structure 
 * and can be deleted after the initializing
 *
 * \return N_gradient_neighbours_2d * -- if failure NULL is returned
 *
 * */
N_gradient_neighbours_2d
    * N_create_gradient_neighbours_2d(N_gradient_neighbours_x * x,
				      N_gradient_neighbours_y * y)
{
    N_gradient_neighbours_2d *grad;
    int fail = 0;

    G_debug(5,
	    "N_create_gradient_neighbours_2d: create N_gradient_neighbours_2d");

    grad = N_alloc_gradient_neighbours_2d();

    if (!N_copy_gradient_neighbours_x(x, grad->x))
	fail++;
    if (!N_copy_gradient_neighbours_y(y, grad->y))
	fail++;

    if (fail > 0) {
	N_free_gradient_neighbours_2d(grad);
	grad = NULL;
    }

    return grad;
}

/*!
 * \brief copy a N_gradient_neighbours_2d structure
 *
 * \param source - the source N_gradient_neighbours_2d struct
 * \param target - the target N_gradient_neighbours_2d struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int
N_copy_gradient_neighbours_2d(N_gradient_neighbours_2d * source,
			      N_gradient_neighbours_2d * target)
{
    int fail = 0;

    G_debug(5,
	    "N_copy_gradient_neighbours_2d: copy N_gradient_neighbours_2d");

    if (!source || !target)
	return 0;

    if (!(N_copy_gradient_neighbours_x(source->x, target->x)))
	fail++;
    if (!(N_copy_gradient_neighbours_y(source->y, target->y)))
	fail++;

    if (fail > 0) {
	return 0;
    }

    return 1;
}

/*!
 * \brief Return a N_gradient_neighbours_2d structure calculated from the input gradient field
 * at position [row][col]
 *
 *  This function returns the gradient neighbours in x and y dierection 
 *  of a cell at position [row][col] from the input gradient field.
 *  Returend is a pointer to a structure of type N_gradient_neighbours_2d.
 *
 *  \param field N_gradient_field_2d * - A two dimensional gradient field
 *  \param gradient N_gradient_neighbours_2d * - the gradient structure which should be filled with data, if a NULL pointer is given, a new structure will be created
 *  \param col int
 *  \param row int
 *  \return N_gradient_neighbours_2d * - the new or filled gradient structure
 *  
 *
 * */
N_gradient_neighbours_2d *N_get_gradient_neighbours_2d(N_gradient_field_2d *
						       field,
						       N_gradient_neighbours_2d
						       * gradient, int col,
						       int row)
{
    double NWN, NEN, WC, EC, SWS, SES;
    double NWW, NEE, NC, SC, SWW, SEE;
    N_gradient_neighbours_2d *grad = NULL;
    N_gradient_neighbours_x *grad_x = NULL;
    N_gradient_neighbours_y *grad_y = NULL;


    NWN = N_get_array_2d_d_value(field->x_array, col, row - 1);
    NEN = N_get_array_2d_d_value(field->x_array, col + 1, row - 1);
    WC = N_get_array_2d_d_value(field->x_array, col, row);
    EC = N_get_array_2d_d_value(field->x_array, col + 1, row);
    SWS = N_get_array_2d_d_value(field->x_array, col, row + 1);
    SES = N_get_array_2d_d_value(field->x_array, col + 1, row + 1);

    NWW = N_get_array_2d_d_value(field->y_array, col - 1, row);
    NEE = N_get_array_2d_d_value(field->y_array, col + 1, row);
    NC = N_get_array_2d_d_value(field->y_array, col, row);
    SC = N_get_array_2d_d_value(field->y_array, col, row + 1);
    SWW = N_get_array_2d_d_value(field->y_array, col - 1, row + 1);
    SEE = N_get_array_2d_d_value(field->y_array, col + 1, row + 1);


    grad_x = N_create_gradient_neighbours_x(NWN, NEN, WC, EC, SWS, SES);
    grad_y = N_create_gradient_neighbours_y(NWW, NEE, NC, SC, SWW, SEE);

    G_debug(5,
	    "N_get_gradient_neighbours_2d: calculate N_gradient_neighbours_x NWN %g NEN %g WC %g EC %g SWS %g SES %g",
	    NWN, NEN, WC, EC, SWS, SES);

    G_debug(5,
	    "N_get_gradient_neighbours_2d: calculate N_gradient_neighbours_y NWW %g NEE %g NC %g SC %g SWW %g SEE %g",
	    NWW, NEE, NC, SC, SWW, SEE);


    /*if gradient is a NULL pointer, create a new one */
    if (!gradient) {
	grad = N_create_gradient_neighbours_2d(grad_x, grad_y);
	gradient = grad;
    }
    else {
	grad = N_create_gradient_neighbours_2d(grad_x, grad_y);
	N_copy_gradient_neighbours_2d(grad, gradient);
	N_free_gradient_neighbours_2d(grad);
    }

    N_free_gradient_neighbours_x(grad_x);
    N_free_gradient_neighbours_y(grad_y);

    return gradient;
}


/*!
 * \brief Allocate a N_gradient_neighbours_3d structure
 *
 * This structure contains all neighbour gradients in all directions of one cell 
 * in a 3d raster layer
 *
 * \return N_gradient_neighbours_3d *
 *
 * */
N_gradient_neighbours_3d *N_alloc_gradient_neighbours_3d(void)
{
    N_gradient_neighbours_3d *grad;

    grad =
	(N_gradient_neighbours_3d *) G_calloc(1,
					      sizeof
					      (N_gradient_neighbours_3d));

    grad->xt = N_alloc_gradient_neighbours_x();
    grad->xc = N_alloc_gradient_neighbours_x();
    grad->xb = N_alloc_gradient_neighbours_x();
    grad->yt = N_alloc_gradient_neighbours_y();
    grad->yc = N_alloc_gradient_neighbours_y();
    grad->yb = N_alloc_gradient_neighbours_y();
    grad->zt = N_alloc_gradient_neighbours_z();
    grad->zb = N_alloc_gradient_neighbours_z();

    return grad;
}

/*!
 * \brief Free's a N_gradient_neighbours_3d structure
 *
 * \return void
 *
 * */
void N_free_gradient_neighbours_3d(N_gradient_neighbours_3d * grad)
{

    N_free_gradient_neighbours_x(grad->xt);
    N_free_gradient_neighbours_x(grad->xc);
    N_free_gradient_neighbours_x(grad->xb);
    N_free_gradient_neighbours_y(grad->yt);
    N_free_gradient_neighbours_y(grad->yc);
    N_free_gradient_neighbours_y(grad->yb);
    N_free_gradient_neighbours_z(grad->zt);
    N_free_gradient_neighbours_z(grad->zb);

    G_free(grad);
    grad = NULL;

    return;
}

/*!
 * \brief Allocate and initialize a N_gradient_neighbours_3d structure
 *
 * The parameter N_gradient_neighbours x(tcb) and y(tcb) and z(tb) are copied into the new allocated structure 
 * and can be deleted after the initializing
 *
 * \return N_gradient_neighbours_3d * -- if failure NULL is returned

 *
 * */
N_gradient_neighbours_3d
    * N_create_gradient_neighbours_3d(N_gradient_neighbours_x * xt,
				      N_gradient_neighbours_x * xc,
				      N_gradient_neighbours_x * xb,
				      N_gradient_neighbours_y * yt,
				      N_gradient_neighbours_y * yc,
				      N_gradient_neighbours_y * yb,
				      N_gradient_neighbours_z * zt,
				      N_gradient_neighbours_z * zb)
{
    N_gradient_neighbours_3d *grad;
    int fail = 0;

    G_debug(5,
	    "N_create_gradient_neighbours_3d: create N_gradient_neighbours_3d");

    grad = N_alloc_gradient_neighbours_3d();

    if (!(N_copy_gradient_neighbours_x(xt, grad->xt)))
	fail++;
    if (!(N_copy_gradient_neighbours_x(xc, grad->xc)))
	fail++;
    if (!(N_copy_gradient_neighbours_x(xb, grad->xb)))
	fail++;
    if (!(N_copy_gradient_neighbours_y(yt, grad->yt)))
	fail++;
    if (!(N_copy_gradient_neighbours_y(yc, grad->yc)))
	fail++;
    if (!(N_copy_gradient_neighbours_y(yb, grad->yb)))
	fail++;
    if (!(N_copy_gradient_neighbours_z(zt, grad->zt)))
	fail++;
    if (!(N_copy_gradient_neighbours_z(zb, grad->zb)))
	fail++;

    if (fail > 0) {
	return NULL;
    }

    return grad;
}

/*!
 * \brief copy a N_gradient_neighbours_3d structure
 *
 * \param source - the source N_gradient_neighbours_3d struct
 * \param target - the target N_gradient_neighbours_3d struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int
N_copy_gradient_neighbours_3d(N_gradient_neighbours_3d * source,
			      N_gradient_neighbours_3d * target)
{
    int fail = 0;

    G_debug(5,
	    "N_copy_gradient_neighbours_3d: copy N_gradient_neighbours_3d");

    if (!source || !target)
	return 0;

    if (!(N_copy_gradient_neighbours_x(source->xt, target->xt)))
	fail++;
    if (!(N_copy_gradient_neighbours_x(source->xc, target->xc)))
	fail++;
    if (!(N_copy_gradient_neighbours_x(source->xb, target->xb)))
	fail++;
    if (!(N_copy_gradient_neighbours_y(source->yt, target->yt)))
	fail++;
    if (!(N_copy_gradient_neighbours_y(source->yc, target->yc)))
	fail++;
    if (!(N_copy_gradient_neighbours_y(source->yb, target->yb)))
	fail++;
    if (!(N_copy_gradient_neighbours_z(source->zt, target->zt)))
	fail++;
    if (!(N_copy_gradient_neighbours_z(source->zb, target->zb)))
	fail++;

    if (fail > 0) {
	return 0;
    }

    return 1;
}

/*!
 * \brief Allocate a N_gradient_field_2d
 *
 * The field arrays are of type DCELL. 
 *
 * \param rows - number of rows of the 2d array from which the gradient should be calculated
 * \param cols - number of cols of the 2d array from which the gradient should be calculated
 * \return N_gradient_field_2d *
 *
 * */
N_gradient_field_2d *N_alloc_gradient_field_2d(int cols, int rows)
{
    N_gradient_field_2d *field;

    G_debug(5,
	    "N_alloc_gradient_field_2d: allocate a N_gradient_field_2d struct");

    field = (N_gradient_field_2d *) G_calloc(1, sizeof(N_gradient_field_2d));

    field->x_array = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    field->y_array = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);

    field->cols = cols;
    field->rows = rows;

    return field;
}

/*!
 * \brief Free's a N_gradient_neighbours_2d structure
 *
 * \return void
 *
 * */
void N_free_gradient_field_2d(N_gradient_field_2d * field)
{

    N_free_array_2d(field->x_array);
    N_free_array_2d(field->y_array);

    G_free(field);

    field = NULL;

    return;
}

/*!
 * \brief Copy N_gradient_field_2d structure from source to target
 *
 * \param source - the source N_gradient_field_2d struct
 * \param target - the target N_gradient_field_2d struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int
N_copy_gradient_field_2d(N_gradient_field_2d * source,
			 N_gradient_field_2d * target)
{
    G_debug(3, "N_copy_gradient_field_2d: copy N_gradient_field_2d");

    if (!source || !target)
	return 0;

    N_copy_array_2d(source->x_array, target->x_array);
    N_copy_array_2d(source->y_array, target->y_array);

    return 1;
}

/*! \brief Print gradient field information to stdout
 *
 * \param field N_gradient_2d_field *
 * \return void
 *
 * */
void N_print_gradient_field_2d_info(N_gradient_field_2d * field)
{
    fprintf(stdout, "N_gradient_field_2d \n");
    fprintf(stdout, "Cols %i\n", field->cols);
    fprintf(stdout, "Rows: %i\n", field->rows);
    fprintf(stdout, "X array pointer: %p\n", field->x_array);
    fprintf(stdout, "Y array pointer: %p\n", field->y_array);
    fprintf(stdout, "Min %g\n", field->min);
    fprintf(stdout, "Max %g\n", field->max);
    fprintf(stdout, "Sum %g\n", field->sum);
    fprintf(stdout, "Mean %g\n", field->mean);
    fprintf(stdout, "Nonull %i\n", field->nonull);
    fprintf(stdout, "X array info \n");
    N_print_array_2d_info(field->x_array);
    fprintf(stdout, "Y array info \n");
    N_print_array_2d_info(field->y_array);

    return;
}


/*!
 * \brief Allocate a N_gradient_field_3d
 *
 * The field arrays are always of type DCELL_TYPE. 
 *
 * \param cols - number of cols of the 3d array from which the gradient should be calculated
 * \param rows - number of rows of the 3d array from which the gradient should be calculated
 * \param depths - number of depths of the 3d array from which the gradient should be calculated
 * \return N_gradient_field_3d *
 *
 * */
N_gradient_field_3d *N_alloc_gradient_field_3d(int cols, int rows, int depths)
{
    N_gradient_field_3d *field;

    G_debug(5,
	    "N_alloc_gradient_field_3d: allocate a N_gradient_field_3d struct");

    field = (N_gradient_field_3d *) G_calloc(1, sizeof(N_gradient_field_3d));

    field->x_array = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    field->y_array = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    field->z_array = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);

    field->cols = cols;
    field->rows = rows;
    field->depths = depths;

    return field;
}


/*!
 * \brief Free's a N_gradient_neighbours_3d structure
 *
 * \return void
 *
 * */
void N_free_gradient_field_3d(N_gradient_field_3d * field)
{

    N_free_array_3d(field->x_array);
    N_free_array_3d(field->y_array);
    N_free_array_3d(field->z_array);

    G_free(field);

    field = NULL;

    return;
}


/*!
 * \brief Copy N_gradient_field_3d structure from source to target
 *
 * \param source - the source N_gradient_field_3d struct
 * \param target - the target N_gradient_field_3d struct
 * \return int - 1 success, 0 failure while copying
 *
 * */
int
N_copy_gradient_field_3d(N_gradient_field_3d * source,
			 N_gradient_field_3d * target)
{
    G_debug(3, "N_copy_gradient_field_3d: copy N_gradient_field_3d");

    if (!source || !target)
	return 0;

    N_copy_array_3d(source->x_array, target->x_array);
    N_copy_array_3d(source->y_array, target->y_array);
    N_copy_array_3d(source->z_array, target->z_array);

    return 1;
}

/*! \brief Print gradient field information to stdout
 *
 * \param field N_gradient_3d_field *
 * \return void
 *
 * */
void N_print_gradient_field_3d_info(N_gradient_field_3d * field)
{

    fprintf(stdout, "N_gradient_field_3d \n");
    fprintf(stdout, "Cols %i\n", field->cols);
    fprintf(stdout, "Rows: %i\n", field->rows);
    fprintf(stdout, "Depths %i\n", field->depths);
    fprintf(stdout, "X array pointer: %p\n", field->x_array);
    fprintf(stdout, "Y array pointer: %p\n", field->y_array);
    fprintf(stdout, "Z array pointer: %p\n", field->z_array);
    fprintf(stdout, "Min %g\n", field->min);
    fprintf(stdout, "Max %g\n", field->max);
    fprintf(stdout, "Sum %g\n", field->sum);
    fprintf(stdout, "Mean %g\n", field->mean);
    fprintf(stdout, "Nonull %i\n", field->nonull);
    fprintf(stdout, "X array info \n");
    N_print_array_3d_info(field->x_array);
    fprintf(stdout, "Y array info \n");
    N_print_array_3d_info(field->y_array);
    fprintf(stdout, "Z array info \n");
    N_print_array_3d_info(field->z_array);

    return;
}
