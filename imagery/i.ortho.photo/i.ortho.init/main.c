
/****************************************************************************
 *
 * MODULE:       i.photo.init
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman
 *
 * PURPOSE:      Creates or modifies entries in a camera initial exposure
 *               station file for imagery group referenced by a sub-block
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "orthophoto.h"


int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *group_opt,
		  *xc_opt, *yc_opt, *zc_opt,
		  *xcsd_opt, *ycsd_opt, *zcsd_opt,
		  *omega_opt, *phi_opt, *kappa_opt,
		  *omegasd_opt, *phisd_opt, *kappasd_opt;
    struct Flag *use_flag, *print_flag;

    struct Ortho_Image_Group group;
    struct Ortho_Camera_Exp_Init *init_info;
    double deg2rad, rad2deg;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("orthorectify"));
    module->description =
	_("Interactively creates or modifies entries in a camera "
	  "initial exposure station file for imagery group referenced "
	  "by a sub-block.");

    group_opt = G_define_standard_option(G_OPT_I_GROUP);
    group_opt->required = YES;
    group_opt->description =
	_("Name of imagery group for ortho-rectification");

    xc_opt = G_define_option();
    xc_opt->key = "xc";
    xc_opt->type = TYPE_DOUBLE;
    xc_opt->description = _("Initial Camera Exposure X-coordinate");

    yc_opt = G_define_option();
    yc_opt->key = "yc";
    yc_opt->type = TYPE_DOUBLE;
    yc_opt->description = _("Initial Camera Exposure Y-coordinate");

    zc_opt = G_define_option();
    zc_opt->key = "zc";
    zc_opt->type = TYPE_DOUBLE;
    zc_opt->description = _("Initial Camera Exposure Z-coordinate");

    xcsd_opt = G_define_option();
    xcsd_opt->key = "xc_sd";
    xcsd_opt->type = TYPE_DOUBLE;
    xcsd_opt->description = _("X-coordinate standard deviation");

    ycsd_opt = G_define_option();
    ycsd_opt->key = "yc_sd";
    ycsd_opt->type = TYPE_DOUBLE;
    ycsd_opt->description = _("Y-coordinate standard deviation");

    zcsd_opt = G_define_option();
    zcsd_opt->key = "zc_sd";
    zcsd_opt->type = TYPE_DOUBLE;
    zcsd_opt->description = _("Z-coordinate standard deviation");
    
    omega_opt = G_define_option();
    omega_opt->key = "omega";
    omega_opt->type = TYPE_DOUBLE;
    omega_opt->description = _("Initial Camera Omega (roll) degrees");

    phi_opt = G_define_option();
    phi_opt->key = "omega";
    phi_opt->type = TYPE_DOUBLE;
    phi_opt->description = _("Initial Camera Phi (pitch) degrees");

    kappa_opt = G_define_option();
    kappa_opt->key = "omega";
    kappa_opt->type = TYPE_DOUBLE;
    kappa_opt->description = _("Initial Camera Kappa (yaw) degrees");

    omegasd_opt = G_define_option();
    omegasd_opt->key = "omega";
    omegasd_opt->type = TYPE_DOUBLE;
    omegasd_opt->description = _("Omega (roll) standard deviation");

    phisd_opt = G_define_option();
    phisd_opt->key = "omega";
    phisd_opt->type = TYPE_DOUBLE;
    phisd_opt->description = _("Phi (pitch) standard deviation");

    kappasd_opt = G_define_option();
    kappasd_opt->key = "omega";
    kappasd_opt->type = TYPE_DOUBLE;
    kappasd_opt->description = _("Kappa (yaw) standard deviation");

    use_flag = G_define_flag();
    use_flag->key = 'r';
    use_flag->description = _("Use initial values at run time");

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->description = _("Print initial values");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    deg2rad = M_PI / 180.;
    rad2deg = 180. / M_PI;

    /* get group ref */
    strcpy(group.name, group_opt->answer);

    if (!I_find_group(group.name)) {
	G_fatal_error(_("Group [%s] not found"), group.name);
    }
    G_debug(1, "Found group %s", group.name);

    /* get initial camera exposure info */
    if (I_find_initial(group.name)) {
	I_get_init_info(group.name, &group.camera_exp);
    }
    else {
	/* create new initial camera exposure info */
	
	/* all values must be given */
	if (!xc_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    xc_opt->key);
	}
	if (!yc_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    yc_opt->key);
	}
	if (!zc_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    zc_opt->key);
	}
	if (!xcsd_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    xcsd_opt->key);
	}
	if (!ycsd_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    ycsd_opt->key);
	}
	if (!zcsd_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    zcsd_opt->key);
	}
	if (!omega_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    omega_opt->key);
	}
	if (!phi_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    phi_opt->key);
	}
	if (!kappa_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    kappa_opt->key);
	}
	if (!omegasd_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    omegasd_opt->key);
	}
	if (!phisd_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    phisd_opt->key);
	}
	if (!kappasd_opt->answer) {
	    G_fatal_error(_("Option '%s' is required for new exposure info"),
			    kappasd_opt->key);
	}
    }

    /* modify info */
    init_info = &group.camera_exp;

    if (xc_opt->answer) {
	init_info->XC_init = atof(xc_opt->answer);
    }
    if (yc_opt->answer) {
	init_info->YC_init = atof(yc_opt->answer);
    }
    if (zc_opt->answer) {
	init_info->ZC_init = atof(zc_opt->answer);
    }
    if (xcsd_opt->answer) {
	init_info->XC_var = atof(xcsd_opt->answer);
    }
    if (ycsd_opt->answer) {
	init_info->YC_var = atof(ycsd_opt->answer);
    }
    if (zcsd_opt->answer) {
	init_info->ZC_var = atof(zcsd_opt->answer);
    }
    if (omega_opt->answer) {
	init_info->omega_init = atof(omega_opt->answer) * deg2rad;
    }
    if (phi_opt->answer) {
	init_info->phi_init = atof(phi_opt->answer) * deg2rad;
    }
    if (kappa_opt->answer) {
	init_info->kappa_init = atof(kappa_opt->answer) * deg2rad;
    }
    if (omegasd_opt->answer) {
	init_info->omega_var = atof(omegasd_opt->answer) * deg2rad;
    }
    if (phisd_opt->answer) {
	init_info->phi_var = atof(phisd_opt->answer) * deg2rad;
    }
    if (kappasd_opt->answer) {
	init_info->kappa_var = atof(kappasd_opt->answer) * deg2rad;
    }
    init_info->status = use_flag->answer != 0;
    
    if (print_flag->answer) {
	/* do not translate, scripts might want to parse the output */
	fprintf(stdout, "xc=%.17g\n", init_info->XC_init);
	fprintf(stdout, "yc=%.17g\n", init_info->YC_init);
	fprintf(stdout, "zc=%.17g\n", init_info->ZC_init);
	fprintf(stdout, "xc_sd=%.17g\n", init_info->XC_var);
	fprintf(stdout, "yc_sd=%.17g\n", init_info->YC_var);
	fprintf(stdout, "zc_sd=%.17g\n", init_info->ZC_var);

	fprintf(stdout, "omega=%.17g\n", init_info->omega_init * rad2deg);
	fprintf(stdout, "phi=%.17g\n", init_info->phi_init * rad2deg);
	fprintf(stdout, "kappa=%.17g\n", init_info->kappa_init * rad2deg);
	fprintf(stdout, "omega_sd=%.17g\n", init_info->omega_var * rad2deg);
	fprintf(stdout, "phi_sd=%.17g\n", init_info->phi_var * rad2deg);
	fprintf(stdout, "kappa_sd=%.17g\n", init_info->kappa_var * rad2deg);
	
	fprintf(stdout, "use=%d", init_info->status);
    }

    /* save info */
    I_put_init_info(group.name, &group.camera_exp);

    exit(EXIT_SUCCESS);
}
