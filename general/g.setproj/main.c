/*
 ****************************************************************************
 *
 * MODULE:       g.setproj 
 * AUTHOR(S):    M. L. Holko - Soil Conservation Service, USDA
 *               Morten Hulden - morten@ngb.se
 *               Andreas Lange - andreas.lange@rhein-main.de
 * PURPOSE:      Provides a means of creating a new projection information
 *               file (productivity tool).
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* old log retained for information */
/* main.c    
 *    1.1   05/16/91  GRASS4.0
 *    Created by : M.L.Holko , Soil Conservation Service, USDA
 *    Purpose: Productivity tool
 *          Provides a means of creating a new projection
 *             information file
 *
 *  ------Rev 4.+ arguements --------------------------------------
 *    Input arguements:
 *             m.setproj set=mapset for output project info file
 *                    proj=projection of the output project info file
 *
 *   1.2 Changed by Morten Hulden 10/10/99 to add support for more projections        
 *                  morten@ngb.se
 *
 *   1.3 Changed by Andreas Lange 07/25/00 to add datum support
 *                  Andreas.Lange@Rhein-Main.de
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

/* some global variables */
int ier, zone;
double radius;

int main(int argc, char *argv[])
{
    struct GModule *module;
    int Out_proj;
    int out_stat;
    int old_zone, old_proj;
    int i;
    int stat;
    char cmnd2[500];
    char proj_out[20], proj_name[50], set_name[20];
    char path[1024], buffa[1024], buffb[1024], answer[200], answer1[200];
    char answer2[200], buff[1024];
    char tmp_buff[20], *buf;

    struct Key_Value *old_proj_keys, *out_proj_keys, *in_unit_keys;
    double aa, e2;
    double f;
    FILE *FPROJ;
    int exist = 0;
    char spheroid[100];
    int j, k, sph_check;
    struct Cell_head cellhd;
    char datum[100], dat_ellps[100], dat_params[100];
    struct proj_parm *proj_parms;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("general, projection");
    module->description =
	_("Interactively reset the location's projection settings.");

    if( argc>1 && G_parser(argc, argv) )
	exit(EXIT_FAILURE);


    if (strcmp(G_mapset(), "PERMANENT") != 0)
	G_fatal_error(
	  _("You must be in the PERMANENT mapset to run g.setproj"));

	/***
         * no longer necessary, table is a static struct 
	 * init_unit_table();
        ***/
    sprintf(set_name, "PERMANENT");
    G__file_name(path, "", PROJECTION_FILE, set_name);

    /* get the output projection parameters, if existing */
    /* Check for ownership here */
    stat = G__mapset_permissions(set_name);
    if (stat == 0) {
	G_fatal_error(_("PERMANENT: permission denied"));
    }
    G_get_default_window(&cellhd);
    if (-1 == G_set_window(&cellhd))
	G_fatal_error(_("Current region cannot be set"));

    if (G_get_set_window(&cellhd) == -1)
	G_fatal_error(_("Retrieving and setting region failed"));

    Out_proj = cellhd.proj;
    old_zone = cellhd.zone;
    old_proj = cellhd.proj;

    if (access(path, 0) == 0) {
	exist = 1;
	FPROJ = fopen(path, "r");
	old_proj_keys = G_fread_key_value(FPROJ);
	fclose(FPROJ);
	buf = G_find_key_value("name", old_proj_keys);
	fprintf(stderr,
	    "\nWARNING: A projection file already exists for this location\n(Filename '%s')\n",
		path);
	fprintf(stderr,
	    "\nThis file contains all the parameters for the\nlocation's projection: %s\n", buf);
	fprintf(stderr,
	    "\n    Overriding this information implies that the old projection parameters\n"
	    "	 were incorrect.  If you change the parameters, all existing data will be\n"
	    "	 interpreted differently by the projection software.\n%c%c%c", 7, 7, 7);
	fprintf(stderr,
	    "    GRASS will not re-project your data automatically\n");

	if (!G_yes(_("Would you still like to change some of the parameters?"), 0)) {
	    G_message(_("The projection information will not be updated"));
	    leave(SP_NOCHANGE);
	}
    }
    out_proj_keys = G_create_key_value();

    if (exist) {
	buf = G_find_key_value("zone", old_proj_keys);
	if (buf != NULL)
	    sscanf(buf, "%d", &zone);
	if (zone != old_zone) {
	    G_warning(_("Zone in default geographic region definition: %d\n"
			" is different from zone in PROJ_INFO file: %d"),
		      old_zone, zone);
	    old_zone = zone;
	}
    }
    switch (Out_proj) {
    case 0:			/* No projection/units */
	if (!exist) {
	    /* leap frog over code, and just make sure we remove the file */
	    G_warning (_("XY-location cannot be projected"));
	    goto write_file;
	    break;
	}
    case PROJECTION_UTM:
	if (!exist) {
	    sprintf(proj_name, "%s", G__projection_name(PROJECTION_UTM));
	    sprintf(proj_out, "utm");
	    break;
	}
    case PROJECTION_SP:
	if (!exist) {
	    sprintf(proj_name, "%s", G__projection_name(PROJECTION_SP));
	    sprintf(proj_out, "stp");
	    break;
	}
    case PROJECTION_LL:
	if (!exist) {
	    sprintf(proj_name, "%s", G__projection_name(PROJECTION_LL));
	    sprintf(proj_out, "ll");
	    break;
	}
    case PROJECTION_OTHER:
	if (G_ask_proj_name(proj_out, proj_name) < 0)
	    leave(SP_NOCHANGE);

	if (G_strcasecmp(proj_out, "LL") == 0)
	    Out_proj = PROJECTION_LL;
	else if (G_strcasecmp(proj_out, "UTM") == 0)
	    Out_proj = PROJECTION_UTM;
	else if (G_strcasecmp(proj_out, "STP") == 0)
	    Out_proj = PROJECTION_SP;
	break;
    default:
	G_fatal_error(_("Unknown projection"));
    }
    cellhd.proj = Out_proj;

    proj_parms = get_proj_parms(proj_out);
    if (!proj_parms)
	G_fatal_error(
	  _("Projection %s is not specified in the file 'proj-parms.table'"), proj_out);

    G_set_key_value("name", proj_name, out_proj_keys);

    sph_check = 0;
    if (G_yes(_("Do you wish to specify a geodetic datum for this location?"), 1)) {
	char lbuf[100], lbufa[100];
	if (exist &&
	    (G_get_datumparams_from_projinfo(old_proj_keys, lbuf, lbufa) ==
	     2)) {
	    G_strip(lbuf);
	    if ((i = G_get_datum_by_name(lbuf)) > 0) {
		G_message(_("The current datum is %s (%s)"),
			G_datum_name(i), G_datum_description(i));
		if (G_yes(
		  _("Do you wish to change the datum (or datum transformation parameters)?"), 0))
		    sph_check = ask_datum(datum, dat_ellps, dat_params);
		else {
		    sprintf(datum, lbuf);
		    sprintf(dat_params, lbufa);
		    sprintf(dat_ellps, G_datum_ellipsoid(i));
		    sph_check = 1;
		    G_message(_("The datum information has not been changed"));
		}
	    }
	    else
		sph_check = ask_datum(datum, dat_ellps, dat_params);

	}
	else
	    sph_check = ask_datum(datum, dat_ellps, dat_params);
    }

    if (sph_check > 0) {
	char *paramkey, *paramvalue;
	/* write out key/value pairs to out_proj_keys */
	if (G_strcasecmp(datum, "custom") != 0)
	    G_set_key_value("datum", datum, out_proj_keys);
	/*        G_set_key_value("datumparams", dat_params, out_proj_keys); */
	paramkey = strtok(dat_params, "=");
	paramvalue = dat_params + strlen(paramkey) + 1;
	G_set_key_value(paramkey, paramvalue, out_proj_keys);
	sprintf(spheroid, "%s", dat_ellps);
    }
    else {

/*****************   GET spheroid  **************************/

	if (Out_proj != PROJECTION_SP) {	/* some projections have 
						 * fixed spheroids */
	    if (G_strcasecmp(proj_out, "ALSK") == 0 ||
		G_strcasecmp(proj_out, "GS48") == 0 ||
		G_strcasecmp(proj_out, "GS50") == 0) {
		sprintf(spheroid, "%s", "clark66");
		G_set_key_value("ellps", spheroid, out_proj_keys);
		sph_check = 1;
	    }
	    else if (G_strcasecmp(proj_out, "LABRD") == 0 ||
		     G_strcasecmp(proj_out, "NZMG") == 0) {
		sprintf(spheroid, "%s", "international");
		G_set_key_value("ellps", spheroid, out_proj_keys);
		sph_check = 1;
	    }
	    else if (G_strcasecmp(proj_out, "SOMERC") == 0) {
		sprintf(spheroid, "%s", "bessel");
		G_set_key_value("ellps", spheroid, out_proj_keys);
		sph_check = 1;
	    }
	    else if (G_strcasecmp(proj_out, "OB_TRAN") == 0) {
		/* Hard coded to use "Equidistant Cylincrical"
		 * until g.setproj has been changed to run
		 * recurively, to allow input of options for
		 * a second projection, MHu991010 */
		G_set_key_value("o_proj", "eqc", out_proj_keys);
		sph_check = 2;
	    }
	    else {
		if (exist &&
		    (buf = G_find_key_value("ellps", old_proj_keys)) != NULL) {
		    strcpy(spheroid, buf);
		    G_strip(spheroid);
		    if (G_get_spheroid_by_name(spheroid, &aa, &e2, &f)) {
			/* if legal ellips. exist, ask wether or not to change it */
			G_message(_("The current ellipsoid is %s"), spheroid);
			if (G_yes(_("Do you want to change ellipsoid parameter?"), 0))
			    sph_check = G_ask_ellipse_name(spheroid);
			else {
			    G_message(_("The ellipse information has not been changed"));
			    sph_check = 1;
			}
		    }		/* the val is legal */
		    else
			sph_check = G_ask_ellipse_name(spheroid);
		}
		else
		    sph_check = G_ask_ellipse_name(spheroid);
	    }
	}

	if (sph_check > 0) {
	    if (sph_check == 2) {	/* ask radius */
		if (exist) {
		    buf = G_find_key_value("a", old_proj_keys);
		    if ((buf != NULL) && (sscanf(buf, "%lf", &radius) == 1)) {
			G_message(_("The radius is currently %f"), radius);
			if (G_yes(_("Do you want to change the radius?"), 0))
			    radius = prompt_num_double(
				_("Enter radius for the sphere in meters"), RADIUS_DEF, 1);
		    }
		}
		else
		    radius = prompt_num_double(
			_("Enter radius for the sphere in meters"), RADIUS_DEF, 1);
	    }			/* end ask radius */
	}
    }

/*** END get spheroid  ***/


    /* create the PROJ_INFO & PROJ_UNITS files, if required */
    if (G_strcasecmp(proj_out, "LL") == 0) ;
    else if (G_strcasecmp(proj_out, "STP") == 0)
	get_stp_proj(buffb);
    else if (sph_check != 2) {
	G_strip(spheroid);
	if (G_get_spheroid_by_name(spheroid, &aa, &e2, &f) == 0)
	    G_fatal_error(_("Invalid input ellipsoid"));
    }

  write_file:
    /*
     **  NOTE   the program will (hopefully) never exit abnormally
     **  after this point.  Thus we know the file will be completely
     **  written out once it is opened for write 
     */
    if (exist) {
	sprintf(buff, "%s~", path);
	G_rename_file(path, buff);
    }
    if (Out_proj == 0)
	goto write_units;

    /*
     **   Include MISC parameters for PROJ_INFO
     */
    if (G_strcasecmp(proj_out, "STP") == 0) {
	for (i = 0; i < strlen(buffb); i++)
	    if (buffb[i] == ' ')
		buffb[i] = '\t';
	sprintf(cmnd2, "%s\t\n", buffb);
	for (i = 0; i < strlen(cmnd2); i++) {
	    j = k = 0;
	    if (cmnd2[i] == '+') {
		while (cmnd2[++i] != '=')
		    buffa[j++] = cmnd2[i];
		buffa[j] = 0;
		while (cmnd2[++i] != '\t' && cmnd2[i] != '\n' && cmnd2[i] != 0)
		    buffb[k++] = cmnd2[i];
		buffb[k] = 0;
		G_set_key_value(buffa, buffb, out_proj_keys);
	    }
	}
    }
    else if (G_strcasecmp(proj_out, "LL") == 0) {
	G_set_key_value("proj", "ll", out_proj_keys);
	G_set_key_value("ellps", spheroid, out_proj_keys);
    }
    else {
	if (sph_check != 2) {
	    G_set_key_value("proj", proj_out, out_proj_keys);
	    G_set_key_value("ellps", spheroid, out_proj_keys);
	    sprintf(tmp_buff, "%.10f", aa);
	    G_set_key_value("a", tmp_buff, out_proj_keys);
	    sprintf(tmp_buff, "%.10f", e2);
	    G_set_key_value("es", tmp_buff, out_proj_keys);
	    sprintf(tmp_buff, "%.10f", f);
	    G_set_key_value("f", tmp_buff, out_proj_keys);
	}
	else {
	    G_set_key_value("proj", proj_out, out_proj_keys);
	    /* G_set_key_value ("ellps", "sphere", out_proj_keys); */
	    sprintf(tmp_buff, "%.10f", radius);
	    G_set_key_value("a", tmp_buff, out_proj_keys);
	    G_set_key_value("es", "0.0", out_proj_keys);
	    G_set_key_value("f", "0.0", out_proj_keys);
	}

	for (i = 0;; i++) {
	    struct proj_parm *parm = &proj_parms[i];
	    struct proj_desc *desc;

	    if (!parm->name)
		break;

	    desc = get_proj_desc(parm->name);
	    if (!desc)
		break;

	    if (parm->ask) {
		if (G_strcasecmp(desc->type, "bool") == 0) {
		    if (G_yes((char *)desc->desc, 0)) {
			G_set_key_value(desc->key, "defined", out_proj_keys);
			if (G_strcasecmp(parm->name, "SOUTH") == 0)
			    cellhd.zone = -abs(cellhd.zone);
		    }
		}
		else if (G_strcasecmp(desc->type, "lat") == 0) {
		    double val;
		    while (!get_LL_stuff(parm, desc, 1, &val)) ;
		    sprintf(tmp_buff, "%.10f", val);
		    G_set_key_value(desc->key, tmp_buff, out_proj_keys);
		}
		else if (G_strcasecmp(desc->type, "lon") == 0) {
		    double val;
		    while (!get_LL_stuff(parm, desc, 0, &val)) ;
		    sprintf(tmp_buff, "%.10f", val);
		    G_set_key_value(desc->key, tmp_buff, out_proj_keys);
		}
		else if (G_strcasecmp(desc->type, "float") == 0) {
		    double val;
		    while (!get_double(parm, desc, &val)) ;
		    sprintf(tmp_buff, "%.10f", val);
		    G_set_key_value(desc->key, tmp_buff, out_proj_keys);
		}
		else if (G_strcasecmp(desc->type, "int") == 0) {
		    int val;
		    while (!get_int(parm, desc, &val)) ;
		    sprintf(tmp_buff, "%d", val);
		    G_set_key_value(desc->key, tmp_buff, out_proj_keys);
		}
		else if (G_strcasecmp(desc->type, "zone") == 0) {
		    if ((Out_proj == PROJECTION_UTM) && (old_zone != 0)) {
			G_message(_("The UTM zone is now set to %d"), old_zone);
			if (!G_yes(_("Do you want to change the UTM zone?"), 0)) {
			    G_message(_("UTM zone information has not been updated"));
			    zone = old_zone;
			    break;
			}
			else {
			    G_message(_("But if you change zone, all the existing "
					"data will be interpreted by projection software. "
					"GRASS will not automatically re-project or even "
					"change the headers for existing maps."));
			    if (!G_yes(_("Would you still like to change the UTM zone?"),
				 0)) {
				zone = old_zone;
				break;
			    }
			}
		    }		/* UTM */

		    while (!get_zone()) ;

		    sprintf(tmp_buff, "%d", zone);
		    G_set_key_value("zone", tmp_buff, out_proj_keys);
		    cellhd.zone = zone;
		}
	    }
	    else if (parm->def_exists) {
		/* don't ask, use the default */

		if (G_strcasecmp(desc->type, "float") == 0 ||
		    G_strcasecmp(desc->type, "lat") == 0 ||
		    G_strcasecmp(desc->type, "lon") == 0) {
		    sprintf(tmp_buff, "%.10f", parm->deflt);
		    G_set_key_value(desc->key, tmp_buff, out_proj_keys);
		}
		else if (G_strcasecmp(desc->type, "int") == 0) {
		    sprintf(tmp_buff, "%d", (int)parm->deflt);
		    G_set_key_value(desc->key, tmp_buff, out_proj_keys);
		}
	    }
	}			/* for OPTIONS */
    }

    /* create the PROJ_INFO & PROJ_UNITS files, if required */

    G_write_key_value_file(path, out_proj_keys, &out_stat);
    if (out_stat != 0) {
	G_fatal_error(_("Error writing PROJ_INFO file <%s>"), path);
    }

    G_free_key_value(out_proj_keys);
    if (exist)
	G_free_key_value(old_proj_keys);

  write_units:
    G__file_name(path, "", UNIT_FILE, set_name);

    /* if we got this far, the user
     ** already affirmed to write over old info
     ** so if units file is here, remove it.
     */
    if (access(path, 0) == 0) {
	sprintf(buff, "%s~", path);
	G_rename_file(path, buff);
    }
    if (Out_proj == 0)
	leave(0);

    {
	in_unit_keys = G_create_key_value();

	switch (Out_proj) {
	case PROJECTION_UTM:
	    G_set_key_value("unit", "meter", in_unit_keys);
	    G_set_key_value("units", "meters", in_unit_keys);
	    G_set_key_value("meters", "1.0", in_unit_keys);
	    break;
	case PROJECTION_SP:
	    for (;;) {

		do {
		    fprintf(stderr, "\nSpecify the correct units to use:\n");
		    fprintf(stderr, "Enter the corresponding number\n");
		    fprintf(stderr,
			    "1.\tUS Survey Foot (Default for State Plane 1927)\n");
		    fprintf(stderr, "2.\tInternational Foot\n");
		    fprintf(stderr, "3.\tMeter\n");
		    fprintf(stderr, ">");
		} while (!G_gets(answer));

		G_strip(answer);
		if (strcmp(answer, "1") == 0) {
		    G_set_key_value("unit", "USfoot", in_unit_keys);
		    G_set_key_value("units", "USfeet", in_unit_keys);
		    G_set_key_value("meters", "0.30480060960121920243",
				    in_unit_keys);
		    break;
		}
		else if (strcmp(answer, "2") == 0) {
		    G_set_key_value("unit", "foot", in_unit_keys);
		    G_set_key_value("units", "feet", in_unit_keys);
		    G_set_key_value("meters", "0.3048", in_unit_keys);
		    break;
		}
		else if (strcmp(answer, "3") == 0) {
		    G_set_key_value("unit", "meter", in_unit_keys);
		    G_set_key_value("units", "meters", in_unit_keys);
		    G_set_key_value("meters", "1.0", in_unit_keys);
		    break;
		}
		else
		    fprintf(stderr, "\nInvalid Entry (number 1 - 3)\n");
	    }
	    break;
	case PROJECTION_LL:
	    G_set_key_value("unit", "degree", in_unit_keys);
	    G_set_key_value("units", "degrees", in_unit_keys);
	    G_set_key_value("meters", "1.0", in_unit_keys);
	    break;
	default:
	    if (G_strcasecmp(proj_out, "LL") != 0) {
		fprintf(stderr, _("Enter plural form of units [meters]: "));
		G_gets(answer);
		if (strlen(answer) == 0) {
		    G_set_key_value("unit", "meter", in_unit_keys);
		    G_set_key_value("units", "meters", in_unit_keys);
		    G_set_key_value("meters", "1.0", in_unit_keys);
		}
		else {
		    const struct proj_unit *unit;
		    G_strip(answer);
		    unit = get_proj_unit(answer);
		    if (unit) {
#ifdef FOO
			if (G_strcasecmp(proj_out, "STP") == 0 &&
			    !strcmp(answer, "feet")) {
			    fprintf(stderr,
				    "%cPROJECTION 99 State Plane cannot be in FEET.\n",
				    7);
			    remove(path);	/* remove file */
			    leave(SP_FATAL);
			}
#endif
			G_set_key_value("unit", unit->unit, in_unit_keys);
			G_set_key_value("units", unit->units, in_unit_keys);
			sprintf(buffb, "%.10f", unit->fact);
			G_set_key_value("meters", buffb, in_unit_keys);
		    }
		    else {
			double unit_fact;
			while (1) {
			    fprintf(stderr, _("Enter singular for unit: "));
			    G_gets(answer1);
			    G_strip(answer1);
			    if (strlen(answer1) > 0)
				break;
			}
			while (1) {
			    fprintf(stderr,
				    _("Enter conversion factor from %s to meters: "),
				    answer);
			    G_gets(answer2);
			    G_strip(answer2);
			    if (!
				(strlen(answer2) == 0 ||
				 (1 != sscanf(answer2, "%lf", &unit_fact))))
				break;
			}
			G_set_key_value("unit", answer1, in_unit_keys);
			G_set_key_value("units", answer, in_unit_keys);
			sprintf(buffb, "%.10f", unit_fact);
			G_set_key_value("meters", buffb, in_unit_keys);
		    }
		}
	    }
	    else {
		G_set_key_value("unit", "degree", in_unit_keys);
		G_set_key_value("units", "degrees", in_unit_keys);
		G_set_key_value("meters", "1.0", in_unit_keys);
	    }
	}			/* switch */

	G_write_key_value_file(path, in_unit_keys, &out_stat);
	if (out_stat != 0)
	    G_fatal_error(_("Error writing into UNITS output file <%s>"), path);

	G_free_key_value(in_unit_keys);
    }				/* if */

    if (G__put_window(&cellhd, "", "DEFAULT_WIND") < 0)
	G_fatal_error(_("Unable to write to DEFAULT_WIND region file"));
    fprintf(stderr,
	    _("\nProjection information has been recorded for this location\n\n"));
    if ((old_zone != zone) | (old_proj != cellhd.proj)) {
	G_message(_("The geographic region information in WIND is now obsolete"));
	G_message(_("Run g.region -d to update it"));
    }
    leave(0);
}

int min1(int a, int b)
{
    if (a > b)
	return b;
    else
	return a;
}

int leave(int n)
{
    exit(n);
}
