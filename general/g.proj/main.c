/*  
 ****************************************************************************
 *
 * MODULE:       g.proj 
 * AUTHOR(S):    Paul Kelly - paul-grass@stjohnspoint.co.uk
 *               Shell script style by Martin Landa <landa.martin gmail.com>
 * PURPOSE:      Provides a means of reporting the contents of GRASS
 *               projection information files and creating
 *               new projection information files.
 * COPYRIGHT:    (C) 2003-2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/config.h>

#ifndef HAVE_PROJ_H
#include <proj_api.h>
#endif
#include "local_proto.h"

struct Key_Value *projinfo, *projunits, *projepsg;
struct Cell_head cellhd;

static char *get_authority_names()
{
    char *authnames;

#ifdef USE_PROJDB
    /* PROJ 6 */
    int i, len;
    PROJ_STRING_LIST authlist = proj_get_authorities_from_database(NULL);

    len = 0;
    for (i = 0; authlist[i]; i++) {
	len += strlen(authlist[i]) + 1;
    }
    if (len > 0) {
	authnames = G_malloc((len + 1) * sizeof(char)); /* \0 */
	*authnames = '\0';
	for (i = 0; authlist[i]; i++) {
	    if (i > 0)
		strcat(authnames, ",");
	    strcat(authnames, authlist[i]);
	}
    }
    else {
	authnames = G_store("");
    }
#else
    /* PROJ 4, 5 */
    /* there are various init files in share/proj/:
     * EPSG,GL27,IGNF,ITRF2000,ITRF2008,ITRF2014,nad27,nad83,esri
     * but they have different formats: bothering only with EPSG here */ 
    authnames = G_store("EPSG");
#endif

    return authnames;
}


int main(int argc, char *argv[])
{
    struct Flag *printinfo,	/* Print contents of PROJ_INFO & PROJ_UNITS */
	*shellinfo,             /* Print in shell script style              */
	*printproj4,		/* Print projection in PROJ.4 format        */
	*datuminfo,		/* Check if datum information is present    */
	*create,		/* Create new projection files              */
#ifdef HAVE_OGR
	*printwkt,		/* Print projection in WKT format           */
	*esristyle,		/* Use ESRI-style WKT format                */
#endif
	*dontprettify,		/* Print 'flat' output (no linebreaks)      */
	*forcedatumtrans;	/* Force override of datumtrans parameters  */
    
    struct Option *location,	/* Name of new location to create           */
#ifdef HAVE_OGR
	*inepsg,		/* EPSG projection code                     */
	*inwkt,			/* Input file with projection in WKT format */
	*inproj4,		/* Projection in PROJ.4 format              */
	*ingeo,			/* Input geo-referenced file readable by 
				 * GDAL or OGR                              */
#endif
	*listcodes,		/* list codes of given authority */
	*datum,			/* datum to add (or replace existing datum) */
	*dtrans;		/* index to datum transform option          */
    struct GModule *module;
    
    int formats;
    const char *epsg = NULL;

    G_set_program_name(argv[0]);
    G_no_gisinit();		/* We don't call G_gisinit() here because it validates the
				 * mapset, whereas this module may legitmately be used 
				 * (to create a new location) when none exists */

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("projection"));
    G_add_keyword(_("create location"));
#ifdef HAVE_OGR
    module->label =
	_("Prints or modifies GRASS projection information files "
	  "(in various co-ordinate system descriptions).");
    module->description =
	_("Can also be used to create new GRASS locations.");
#else
    module->description =
	_("Prints and manipulates GRASS projection information files.");
#endif

    printinfo = G_define_flag();
    printinfo->key = 'p';
    printinfo->guisection = _("Print");
    printinfo->description =
	_("Print projection information in conventional GRASS format");

    shellinfo = G_define_flag();
    shellinfo->key = 'g';
    shellinfo->guisection = _("Print");
    shellinfo->description =
	_("Print projection information in shell script style");

    datuminfo = G_define_flag();
    datuminfo->key = 'd';
    datuminfo->guisection = _("Print");
    datuminfo->description =
	_("Verify datum information and print transformation parameters");

    printproj4 = G_define_flag();
    printproj4->key = 'j';
    printproj4->guisection = _("Print");
    printproj4->description =
	_("Print projection information in PROJ.4 format");

    dontprettify = G_define_flag();
    dontprettify->key = 'f';
    dontprettify->guisection = _("Print");
    dontprettify->description =
	_("Print 'flat' output with no linebreaks (applies to "
#ifdef HAVE_OGR
	  "WKT and "
#endif
	  "PROJ.4 output)");

#ifdef HAVE_OGR
    printwkt = G_define_flag();
    printwkt->key = 'w';
    printwkt->guisection = _("Print");
    printwkt->description = _("Print projection information in WKT format");

    esristyle = G_define_flag();
    esristyle->key = 'e';
    esristyle->guisection = _("Print");
    esristyle->description =
	_("Use ESRI-style format (applies to WKT output only)");
    
    ingeo = G_define_option();
    ingeo->key = "georef";
    ingeo->type = TYPE_STRING;
    ingeo->key_desc = "file";
    ingeo->required = NO;
    ingeo->guisection = _("Specification");
    ingeo->description = _("Name of georeferenced data file to read projection "
			   "information from");

    inwkt = G_define_option();
    inwkt->key = "wkt";
    inwkt->type = TYPE_STRING;
    inwkt->key_desc = "file";
    inwkt->required = NO;
    inwkt->guisection = _("Specification");
    inwkt->label = _("Name of ASCII file containing a WKT projection "
		     "description");
    inwkt->description = _("'-' for standard input");

    inproj4 = G_define_option();
    inproj4->key = "proj4";
    inproj4->type = TYPE_STRING;
    inproj4->key_desc = "params";
    inproj4->required = NO;
    inproj4->guisection = _("Specification");
    inproj4->label = _("PROJ.4 projection description");
    inproj4->description = _("'-' for standard input");

    inepsg = G_define_option();
    inepsg->key = "epsg";
    inepsg->type = TYPE_INTEGER;
    inepsg->key_desc = "code";
    inepsg->required = NO;
    inepsg->options = "1-1000000";
    inepsg->guisection = _("Specification");
    inepsg->description = _("EPSG projection code");
#endif

    listcodes = G_define_option();
    listcodes->key = "list_codes";
    listcodes->type = TYPE_STRING;
    listcodes->required = NO;
    listcodes->options = get_authority_names();
    listcodes->guisection = _("Print");
    listcodes->description = _("List codes for given authority, e.g. EPSG, and exit");

    datum = G_define_option();
    datum->key = "datum";
    datum->type = TYPE_STRING;
    datum->key_desc = "name";
    datum->required = NO;
    datum->guisection = _("Datum");
    datum->label =
	_("Datum (overrides any datum specified in input co-ordinate system)");
    datum->description = 
	_("Accepts standard GRASS datum codes, or \"list\" to list and exit");

    dtrans = G_define_option();
    dtrans->key = "datum_trans";
    dtrans->type = TYPE_INTEGER;
    dtrans->key_desc = "index";
    dtrans->required = NO;
    dtrans->options = "-1-100";
    dtrans->answer = "0";
    dtrans->guisection = _("Datum");
    dtrans->label = _("Index number of datum transform parameters");
    dtrans->description = _("\"0\" for unspecified or \"-1\" to list and exit");

    forcedatumtrans = G_define_flag();
    forcedatumtrans->key = 't';
    forcedatumtrans->guisection = _("Datum");
    forcedatumtrans->description =
	_("Force override of datum transformation information in input "
	  "co-ordinate system");

    create = G_define_flag();
    create->key = 'c';
    create->guisection = _("Modify");
    create->description = _("Modify current location projection files");

    location = G_define_option();
    location->key = "location";
    location->type = TYPE_STRING;
    location->key_desc = "name";
    location->required = NO;
    location->guisection = _("Create");
    location->description = _("Name of new location to create");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* Initialisation & Validation */

    /* list codes for given authority */
    if (listcodes->answer) {
#ifdef USE_PROJDB
	/* PROJ 6+ */
	int i, crs_cnt;
	PROJ_CRS_INFO **proj_crs_info;
	
	crs_cnt = 0;
	proj_crs_info = proj_get_crs_info_list_from_database(NULL, listcodes->answer, NULL, &crs_cnt);
	if (crs_cnt < 1)
	    G_fatal_error(_("No codes found for authority %s"),
		          listcodes->answer);
		
	for (i = 0; i < crs_cnt; i++) {
	    const char *proj_definition;
	    PJ *pj;

	    pj = proj_create_from_database(NULL,
	                                   proj_crs_info[i]->auth_name,
	                                   proj_crs_info[i]->code,
					   PJ_CATEGORY_CRS,
					   0, NULL);
	    proj_definition = proj_as_proj_string(NULL, pj, PJ_PROJ_5, NULL);
	    if (!proj_definition) {
		int err = proj_errno(pj);
		
		if (err) {
		    G_warning(_("Unable to fetch proj string: %s"),
			      proj_errno_string(err));
		}
		else {
		    G_warning(_("Unable to fetch proj string: unknown error"));
		}
	    }
	    else {
		fprintf(stdout, "%s|%s|%s\n", proj_crs_info[i]->code,
					      proj_crs_info[i]->name,
					      proj_definition);
	    }
	    proj_destroy(pj);
	}
#else
	char pathname[GPATH_MAX];
	char *authname;
	char code[GNAME_MAX], name[GNAME_MAX], proj_def[GNAME_MAX];
	FILE *fp;
	char buf[4096];
	int line, have_name;

#ifdef HAVE_PROJ_H
	/* PROJ 5 */
	PJ_INIT_INFO init_info;

	authname = listcodes->answer;
	if (G_strcasecmp(authname, "EPSG") == 0)
	    authname = "epsg";

	init_info = proj_init_info(authname);
	sprintf(pathname, init_info.filename);
	
	if (access(pathname, F_OK) != 0)
	    G_fatal_error(_("Unable to find init file %s"), authname);

#else
	/* PROJ 4 */
	/* can't use pj_find_file() from the old proj api
	 * because it does not exist in PROJ 4 */
	char *grass_proj_share;
	
	authname = listcodes->answer;
	if (G_strcasecmp(authname, "EPSG") == 0)
	    authname = "epsg";

	grass_proj_share = getenv("GRASS_PROJSHARE");
	if (!grass_proj_share)
	    G_fatal_error(_("Environment variable GRASS_PROJSHARE is not set"));
	sprintf(pathname, "%s/%s", grass_proj_share, authname);
	G_convert_dirseps_to_host(pathname);
#endif

	/* PROJ 4 / 5 */
	
	/* the init files do not have a common structure, thus restrict to epsg */
	if (strcmp(authname, "epsg") != 0)
	    G_fatal_error(_("Only epsg file is currently supported"));
	
	/* open the init file */
	fp = fopen(pathname, "r");
	if (!fp) {
	    G_fatal_error(_("Unable to open init file <%s>"), authname);
	}
	have_name = 0;
	/* print list of codes, names, definitions */
	for (line = 1; G_getl2(buf, sizeof(buf), fp); line++) {

	    G_strip(buf);
	    if (*buf == '\0')
		continue;

	    if (strncmp(buf, "<metadata>", strlen("<metadata>")) == 0)
		continue;

	    /* name: line starts with '# ' */
	    /* code and definition in next line */

	    if (!have_name) {
		if (*buf != '#')
		    continue;
		sprintf(name, buf + 2);
		have_name = 1;
	    }
	    if (have_name && *buf == '#') {
		sprintf(name, buf + 2);
		continue;
	    }

	    if (have_name && *buf == '<') {
		int i, j, buflen;
		
		buflen = strlen(buf);
		
		i = 0;
		while (i < buflen && buf[i] != '>')
		    i++;
		buf[i] = '\0';
		sprintf(code, buf + 1);
		i++;
		j = i;
		while (i < buflen && buf[i] != '<')
		    i++;
		if (i < buflen && buf[i] == '<')
		    buf[i] = '\0';
		sprintf(proj_def, buf + j);
		G_strip(proj_def);

		fprintf(stdout, "%s|%s|%s\n", code, name, proj_def);
		have_name = 0;
		name[0] = '\0';
	    }
	}
	fclose(fp);
#endif
	exit(EXIT_SUCCESS);
    }

#ifdef HAVE_OGR
    /* -e implies -w */
    if (esristyle->answer && !printwkt->answer)
	printwkt->answer = 1;

    formats = ((ingeo->answer ? 1 : 0) + (inwkt->answer ? 1 : 0) +
	       (inproj4->answer ? 1 : 0) + (inepsg->answer ? 1 : 0));
    if (formats > 1)
	G_fatal_error(_("Only one of '%s', '%s', '%s' or '%s' options may be specified"),
		      ingeo->key, inwkt->key, inproj4->key, inepsg->key);

    /* List supported datums if requested; code originally 
     * from G_ask_datum_name() (formerly in libgis) */
    if (datum->answer && strcmp(datum->answer, "list") == 0) {
	const char *dat;
	int i;

	for (i = 0; (dat = G_datum_name(i)); i++) {	
	    fprintf(stdout, "---\n%d\n%s\n%s\n%s ellipsoid\n",
		    i, dat, G_datum_description(i), G_datum_ellipsoid(i));
	}

	exit(EXIT_SUCCESS);
    }

    epsg = inepsg->answer;
    projinfo = projunits = projepsg = NULL;

    /* Input */
    /* We can only have one input source, hence if..else construct */

    if (formats == 0)
#endif
	/* Input is projection of current location */
	input_currloc();
#ifdef HAVE_OGR
    else if (inwkt->answer)
	/* Input in WKT format */
	input_wkt(inwkt->answer);
    else if (inproj4->answer)
	/* Input in PROJ.4 format */
	input_proj4(inproj4->answer);
    else if (epsg)
	/* Input from EPSG code */
	input_epsg(atoi(epsg));
    else
	/* Input from georeferenced file */
	input_georef(ingeo->answer);
#endif

    /* Consistency Check */

    if ((cellhd.proj != PROJECTION_XY)
	&& (projinfo == NULL || projunits == NULL))
	G_fatal_error(_("Projection files missing"));

    /* Override input datum if requested */
    if(datum->answer)
	set_datum(datum->answer);

    /* Set Datum Parameters if necessary or requested */
    set_datumtrans(atoi(dtrans->answer), forcedatumtrans->answer);


    /* Output */
    /* Only allow one output format at a time, to reduce confusion */
    formats = ((printinfo->answer ? 1 : 0) + (shellinfo->answer ? 1 : 0) +
	       (datuminfo->answer ? 1 : 0) +
	       (printproj4->answer ? 1 : 0) +
#ifdef HAVE_OGR
	       (printwkt->answer ? 1 : 0) +
#endif
	       (create->answer ? 1 : 0));
    if (formats > 1)
	G_fatal_error(_("Only one of -%c, -%c, -%c, -%c"
#ifdef HAVE_OGR
			", -%c"
#endif
			" or -%c flags may be specified"),
		      printinfo->key, shellinfo->key, datuminfo->key, printproj4->key,
#ifdef HAVE_OGR
		      printwkt->key,
#endif
		      create->key);

    if (printinfo->answer || shellinfo->answer)
        print_projinfo(shellinfo->answer);
    else if (datuminfo->answer)
	print_datuminfo();
    else if (printproj4->answer)
	print_proj4(dontprettify->answer);
#ifdef HAVE_OGR
    else if (printwkt->answer)
	print_wkt(esristyle->answer, dontprettify->answer);
#endif
    else if (location->answer)
	create_location(location->answer);
    else if (create->answer)
	modify_projinfo();
    else
#ifdef HAVE_OGR
	G_fatal_error(_("No output format specified, define one "
			"of flags -%c, -%c, -%c, or -%c"),
		      printinfo->key, shellinfo->key, printproj4->key, printwkt->key);
#else
	G_fatal_error(_("No output format specified, define one "
			"of flags -%c, -%c, or -%c"),
		      printinfo->key, shellinfo->key, printproj4->key);
#endif

    /* Tidy Up */
    if (projinfo != NULL)
	G_free_key_value(projinfo);
    if (projunits != NULL)
	G_free_key_value(projunits);
    if (projepsg != NULL)
	G_free_key_value(projepsg);

    exit(EXIT_SUCCESS);

}
