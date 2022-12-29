
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      standard parser option for the numerical pde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include <grass/N_pde.h>

/*!
 * \brief Create standardised Option structure related to the gpde library.
 *
 * This function will create a standardised Option structure
 * defined by parameter opt. A list of valid parameters can be found in N_pde.h.
 * It allocates memory for the Option structure and returns a pointer to
 * this memory (of <i>type struct Option *</i>).<br>
 *
 * If an invalid parameter was specified an empty Option structure will
 * be returned (not NULL).
 *
 * This function is related to the gpde library, general standard options can be 
 * found in lib/gis/parser.c. These options are set with G_define_standard_option ();
 *
 * \param[in] opt Type of Option struct to create
 * \return Option * Pointer to an Option struct
 *
 * */
struct Option *N_define_standard_option(int opt)
{
    struct Option *Opt;

    Opt = G_define_option();

    switch (opt) {
	/*solver for symmetric, positive definite linear equation systems */
    case N_OPT_SOLVER_SYMM:
	Opt->key = "solver";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->key_desc = "name";
	Opt->answer = "cg";
	Opt->options = "gauss,lu,cholesky,jacobi,sor,cg,bicgstab,pcg";
        Opt->guisection = "Solver";
	Opt->description =
	    ("The type of solver which should solve the symmetric linear equation system");
	break;
	/*solver for unsymmetric linear equation systems */
    case N_OPT_SOLVER_UNSYMM:
	Opt->key = "solver";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->key_desc = "name";
	Opt->answer = "bicgstab";
	Opt->options = "gauss,lu,jacobi,sor,bicgstab";
        Opt->guisection = "Solver";
	Opt->description =
	    ("The type of solver which should solve the linear equation system");
	break;
    case N_OPT_MAX_ITERATIONS:
	Opt->key = "maxit";
	Opt->type = TYPE_INTEGER;
	Opt->required = NO;
	Opt->answer = "10000";
        Opt->guisection = "Solver";
	Opt->description =
	    ("Maximum number of iteration used to solve the linear equation system");
	break;
    case N_OPT_ITERATION_ERROR:
	Opt->key = "error";
	Opt->type = TYPE_DOUBLE;
	Opt->required = NO;
	Opt->answer = "0.000001";
        Opt->guisection = "Solver";
	Opt->description =
	    ("Error break criteria for iterative solver");
	break;
    case N_OPT_SOR_VALUE:
	Opt->key = "relax";
	Opt->type = TYPE_DOUBLE;
	Opt->required = NO;
	Opt->answer = "1";
        Opt->guisection = "Solver";
	Opt->description =
	    ("The relaxation parameter used by the jacobi and sor solver for speedup or stabilizing");
	break;
    case N_OPT_CALC_TIME:
	Opt->key = "dtime";
	Opt->type = TYPE_DOUBLE;
	Opt->required = YES;
	Opt->answer = "86400";
        Opt->guisection = "Solver";
	Opt->description = _("The calculation time in seconds");
	break;
    }

    return Opt;
}
