
/*********************************************************************************/

/***                               interface()                                 ***/

/***       Function to get input from user and check files can be opened       ***/

/***  									       ***/

/***         Jo Wood, Department of Geography, V1.2, 7th February 1992         ***/

/*********************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "param.h"

void interface(int argc, char **argv)
{

    /*--------------------------------------------------------------------------*/
    /*                                 INITIALISE                               */

    /*--------------------------------------------------------------------------*/

    struct Option *rast_in,	/* Name of input file from command line. */
     *rast_out,			/* Holds name of output file.           */
     *tol1_val,			/* Tolerance values for feature         */
     *tol2_val,			/* detection (slope and curvature).     */
     *win_size,			/* Size of side of local window.        */
     *parameter,		/* Morphometric parameter to calculate. */
     *expon,			/* Inverse distance exponent for weight. */
     *vert_sc;			/* Vertical scaling factor.             */

    struct Flag *constr;	/* Forces quadratic through the central */

    /* cell of local window if selected.    */
    struct GModule *module;	/* GRASS module description */
    char buf[24];

    G_gisinit(argv[0]);		/* GRASS function which MUST be called  */
    /* first to check for valid database    */
    /* and mapset and prompt user for input. */


    /*--------------------------------------------------------------------------*/
    /*                            SET PARSER OPTIONS                            */

    /*--------------------------------------------------------------------------*/

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("geomorphology"));
    G_add_keyword(_("terrain"));
    G_add_keyword(_("elevation"));
    G_add_keyword(_("landform"));
    module->label = _("Extracts terrain parameters from a DEM.");
    module->description = _("Uses a multi-scale approach"
			    " by taking fitting quadratic parameters to any size window (via least squares).");

    rast_in = G_define_standard_option(G_OPT_R_INPUT);
    rast_out = G_define_standard_option(G_OPT_R_OUTPUT);
    tol1_val = G_define_option();	/* Request memory for each option.      */
    tol2_val = G_define_option();
    win_size = G_define_option();
    parameter = G_define_option();
    expon = G_define_option();
    vert_sc = G_define_option();

    constr = G_define_flag();

    /* Each option has a 'key' (short descriptn), a 'description` (longer one)  */
    /* a 'type' (eg int, or string), and an indication whether manditory or not */

    rast_out->description =
	_("Name for output raster map containing morphometric parameter");

    tol1_val->key = "slope_tolerance";
    tol1_val->description =
	_("Slope tolerance that defines a 'flat' surface (degrees)");
    tol1_val->type = TYPE_DOUBLE;
    tol1_val->required = NO;
    tol1_val->answer = "1.0";

    tol2_val->key = "curvature_tolerance";
    tol2_val->description =
	_("Curvature tolerance that defines 'planar' surface");
    tol2_val->type = TYPE_DOUBLE;
    tol2_val->required = NO;
    tol2_val->answer = "0.0001";

    sprintf(buf, "3-%i", MAX_WSIZE);
    win_size->key = "size";
    win_size->description = _("Size of processing window (odd number only)");
    win_size->type = TYPE_INTEGER;
    win_size->required = NO;
    win_size->options = G_store(buf);
    win_size->answer = "3";

    parameter->key = "method";
    parameter->description =
	_("Morphometric parameter in 'size' window to calculate");
    parameter->type = TYPE_STRING;
    parameter->required = NO;
    parameter->options =
	"elev,slope,aspect,profc,planc,longc,crosc,minic,maxic,feature";
    parameter->answer = "elev";

    expon->key = "exponent";
    expon->description = _("Exponent for distance weighting (0.0-4.0)");
    expon->type = TYPE_DOUBLE;
    expon->required = NO;
    expon->answer = "0.0";

    vert_sc->key = "zscale";
    vert_sc->description = _("Vertical scaling factor");
    vert_sc->type = TYPE_DOUBLE;
    vert_sc->required = NO;
    vert_sc->answer = "1.0";

    constr->key = 'c';
    constr->description = _("Constrain model through central window cell");


    if (G_parser(argc, argv))	/* Actually performs the prompting for      */
	exit(EXIT_FAILURE);	/* keyboard input.                      */


    rast_in_name = rast_in->answer;	/* Now  keyboard input has been parsed, */
    rast_out_name = rast_out->answer;	/* can place the contents into strings  */
    wsize = atoi(win_size->answer);
    constrained = constr->answer;
    sscanf(expon->answer, "%lf", &exponent);
    sscanf(vert_sc->answer, "%lf", &zscale);
    sscanf(tol1_val->answer, "%lf", &slope_tol);
    sscanf(tol2_val->answer, "%lf", &curve_tol);

    if ((exponent < 0.0) || (exponent > 4.0))
	exponent = 0.0;

    if (zscale == 0.0)
	zscale = 1;

    if (!strcmp(parameter->answer, "elev"))
	mparam = ELEV;
    else if (!strcmp(parameter->answer, "slope"))
	mparam = SLOPE;
    else if (!strcmp(parameter->answer, "aspect"))
	mparam = ASPECT;
    else if (!strcmp(parameter->answer, "profc"))
	mparam = PROFC;
    else if (!strcmp(parameter->answer, "planc"))
	mparam = PLANC;
    else if (!strcmp(parameter->answer, "crosc"))
	mparam = CROSC;
    else if (!strcmp(parameter->answer, "longc"))
	mparam = LONGC;
    else if (!strcmp(parameter->answer, "maxic"))
	mparam = MAXIC;
    else if (!strcmp(parameter->answer, "minic"))
	mparam = MINIC;
    else if (!strcmp(parameter->answer, "feature"))
	mparam = FEATURE;
    else {
	G_warning(_("Morphometric parameter not recognised. Assuming 'Elevation'"));
	mparam = ELEV;
    }

    /* make sure input and output names are valid */
    G_check_input_output_name(rast_in_name, rast_out_name, G_FATAL_EXIT);

    if ((wsize / 2 != (wsize - 1) / 2) || (wsize > MAX_WSIZE))
	G_fatal_error(_("Inappropriate window size (too big or even)"));
}
