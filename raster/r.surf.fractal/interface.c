/****************************************************************************/
/***      Function to get input from user and check files can be opened   ***/
/***                                                                      ***/
/***                    Jo Wood,  V1.0, 13th September 1994               ***/
/****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "frac.h"
#include <grass/gis.h>
#include <grass/glocale.h>

int 
interface (
    int argc,              /* Number of command line arguments     */
    char *argv[]            /* Contents of command line arguments.  */
)

{
    /*---------------------------------------------------------------------*/
    /*                               INITIALISE				   */
    /*---------------------------------------------------------------------*/ 

	struct GModule *module;
    struct Option       *rast_out;      /* Structure for output raster     */           
    struct Option	*frac_dim;	/* Fractal dimension of surface.   */	
    struct Option	*num_images;	/* Number of images to produce.	   */

    G_gisinit (argv[0]);                /* Link with GRASS interface.	   */

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
		_("Creates a fractal surface of a given fractal dimension.");

    /*---------------------------------------------------------------------*/
    /*                              SET PARSER OPTIONS                     */
    /*---------------------------------------------------------------------*/

    rast_out = G_define_option();
    frac_dim = G_define_option();
    num_images = G_define_option();

    /* Each option needs a 'key' (short description),
         		 a 'description` (a longer one),
         		 a 'type' (eg intiger, or string),
         	     and an indication whether manditory or not */

    rast_out->key         = "out";
    rast_out->description = _("Name of fractal surface raster layer");
    rast_out->type        = TYPE_STRING;
    rast_out->required    = YES;
    
    frac_dim->key         = "d";
    frac_dim->description = _("Fractal dimension of surface (2 < D < 3)");
    frac_dim->type        = TYPE_DOUBLE;
    frac_dim->required    = NO;
    frac_dim->answer	  = "2.05";

    num_images->key         = "n";
    num_images->description = _("Number of intermediate images to produce");
    num_images->type        = TYPE_INTEGER;
    num_images->required    = NO;
    num_images->answer	    = "0";

    if (G_parser(argc,argv))            /* Performs the prompting for      */
        exit(EXIT_FAILURE);            /* keyboard input.                 */

    rast_out_name  = rast_out->answer;
    sscanf(frac_dim->answer,"%lf",&H);
    H = 3.0 - H;
    Steps = atoi(num_images->answer) + 1;

    G_message (_("Steps=%d"), Steps);

    /*--------------------------------------------------------------------*/
    /*                  CHECK OUTPUT RASTER FILE DOES NOT EXIST           */
    /*--------------------------------------------------------------------*/

    mapset_out = G_mapset();            /* Set output to current mapset.  */

    if (G_legal_filename(rast_out_name)=='\0')
    {
        G_fatal_error (_("<%s> is an illegal file name"), rast_out_name);
    }

    /*--------------------------------------------------------------------*/
    /*               CHECK FRACTAL DIMENSION IS WITHIN LIMITS             */
    /*--------------------------------------------------------------------*/

    if ( (H <= 0) || (H >= 1))
    {
	G_fatal_error(_("Fractal dimension of [%.2lf] must be between 2 and 3."), 3.0-H);
    }


    /*--------------------------------------------------------------------*/
    /*        MAKE SURE NUMBER OF IMAGES <= NUMBER OF FOURIER COEFF.      */
    /*--------------------------------------------------------------------*/

    /* TODO */

    return 0;
}
