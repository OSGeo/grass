/*****************************************************************************
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
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/config.h>

#include "local_proto.h"

struct Key_Value *projinfo = NULL, *projunits = NULL, *projepsg = NULL;
char *projsrid = NULL, *projwkt = NULL;
struct Cell_head cellhd;

int main(int argc, char *argv[])
{
    /* TODO: replace most of these flags with an option to select the
     * output format */
    struct Flag *printinfo, /* Print contents of PROJ_INFO & PROJ_UNITS */
        *shellinfo,         /* Print in shell script style              */
        *printproj4,        /* Print projection in PROJ.4 format        */
        *datuminfo,         /* Check if datum information is present    */
        *create,            /* Create new projection files              */
#ifdef HAVE_OGR
        *printwkt,  /* Print projection in WKT format           */
        *esristyle, /* Use ESRI-style WKT format                */
#endif
        *dontprettify,    /* Print 'flat' output (no linebreaks)      */
        *forcedatumtrans; /* Force override of datumtrans parameters  */

    struct Option *location, /* Name of new location to create           */
#ifdef HAVE_OGR
        *insrid,  /* spatial reference id (auth name + code   */
        *inepsg,  /* EPSG projection code                     */
        *inwkt,   /* Input file with projection in WKT format */
        *inproj4, /* Projection in PROJ.4 format              */
        *ingeo,   /* Input geo-referenced file readable by
                   * GDAL or OGR                              */
#endif
        *listcodes, /* list codes of given authority */
        *datum,     /* datum to add (or replace existing datum) */
        *dtrans,    /* index to datum transform option          */
        *format;    /* output format */
    struct GModule *module;

    int formats;
    enum OutputFormat outputFormat;
    const char *epsg = NULL;

    /* We don't call G_gisinit() here because it validates the
     * mapset, whereas this module may legitimately be used
     * (to create a new location) when none exists. */
    G_set_program_name(argv[0]);
    G_no_gisinit();

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("projection"));
    G_add_keyword(_("create project"));
#ifdef HAVE_OGR
    module->label = _("Prints or modifies GRASS projection information files "
                      "(in various co-ordinate system descriptions).");
    module->description = _("Can also be used to create new GRASS projects.");
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
        _("[DEPRECATED] Print projection information in shell script style. "
          "This flag is obsolete and will be removed in a future release. Use "
          "format=shell instead.");

    datuminfo = G_define_flag();
    datuminfo->key = 'd';
    datuminfo->guisection = _("Print");
    datuminfo->description =
        _("Verify datum information and print transformation parameters");

    printproj4 = G_define_flag();
    printproj4->key = 'j';
    printproj4->guisection = _("Print");
    printproj4->description =
        _("[DEPRECATED] Print projection information in PROJ.4 format. "
          "This flag is obsolete and will be removed in a future release. Use "
          "format=proj4 instead.");

    dontprettify = G_define_flag();
    dontprettify->key = 'f';
    dontprettify->guisection = _("Print");
#ifdef HAVE_OGR
    dontprettify->description = _("Print 'flat' output with no linebreaks "
                                  "(applies to WKT and PROJ.4 output)");
#else
    dontprettify->description =
        _("Print 'flat' output with no linebreaks (applies to PROJ.4 output)");
#endif

#ifdef HAVE_OGR
    printwkt = G_define_flag();
    printwkt->key = 'w';
    printwkt->guisection = _("Print");
    printwkt->description =
        _("[DEPRECATED] Print projection information in WKT format. "
          "This flag is obsolete and will be removed in a future release. Use "
          "format=wkt instead.");

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

    insrid = G_define_option();
    insrid->key = "srid";
    insrid->type = TYPE_STRING;
    insrid->key_desc = "params";
    insrid->required = NO;
    insrid->guisection = _("Specification");
    insrid->label = _("Spatial reference ID with authority name and code");
    insrid->description = _("E.g. EPSG:4326 or urn:ogc:def:crs:EPSG::4326");

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
    listcodes->description =
        _("List codes for given authority, e.g. EPSG, and exit");

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
    create->description = _("Modify current project's projection files");

    location = G_define_option();
    location->key = "project";
    location->type = TYPE_STRING;
    location->key_desc = "name";
    location->required = NO;
    location->guisection = _("Create");
    location->description = _("Name of new project (location) to create");

    format = G_define_standard_option(G_OPT_F_FORMAT);
    format->options = "plain,shell,json,wkt,proj4";
    format->descriptions = _("plain;Human readable text output;"
                             "shell;shell script style text output;"
                             "json;JSON (JavaScript Object Notation);"
                             "wkt;Well-known text output;"
                             "proj4;PROJ.4 style text output;");
    format->guisection = _("Print");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* Initialisation & Validation */

    if (strcmp(format->answer, "json") == 0) {
        outputFormat = JSON;
    }
    else if (strcmp(format->answer, "shell") == 0) {
        outputFormat = SHELL;
    }
    else if (strcmp(format->answer, "wkt") == 0) {
        outputFormat = WKT;
    }
    else if (strcmp(format->answer, "proj4") == 0) {
        outputFormat = PROJ4;
    }
    else {
        outputFormat = PLAIN;
    }

    if (outputFormat != PLAIN && (!printinfo->answer || shellinfo->answer ||
                                  printproj4->answer || printwkt->answer)) {
        G_fatal_error(_("The format option can only be used with -%c flag"),
                      printinfo->key);
    }

    if (shellinfo->answer) {
        G_warning(_("Flag 'g' is deprecated and will be removed in a future "
                    "release. Please use format=shell instead."));
        outputFormat = SHELL;
    }
    else if (printproj4->answer) {
        G_warning(_("Flag 'j' is deprecated and will be removed in a future "
                    "release. Please use format=proj4 instead."));
        outputFormat = PROJ4;
    }
    else if (printwkt->answer) {
        G_warning(_("Flag 'w' is deprecated and will be removed in a future "
                    "release. Please use format=wkt instead."));
        outputFormat = WKT;
    }

    /* list codes for given authority */
    if (listcodes->answer) {
        list_codes(listcodes->answer);
        exit(EXIT_SUCCESS);
    }

#ifdef HAVE_OGR
    /* -e implies -w */
    if (esristyle->answer && !printwkt->answer)
        printwkt->answer = 1;

    formats = ((ingeo->answer ? 1 : 0) + (inwkt->answer ? 1 : 0) +
               (inproj4->answer ? 1 : 0) + (inepsg->answer ? 1 : 0) +
               (insrid->answer ? 1 : 0));
    if (formats > 1)
        G_fatal_error(_("Only one of '%s', '%s', '%s', '%s' or '%s' options "
                        "may be specified"),
                      ingeo->key, inwkt->key, inproj4->key, inepsg->key,
                      insrid->key);

    /* List supported datums if requested; code originally
     * from G_ask_datum_name() (formerly in libgis) */
    if (datum->answer && strcmp(datum->answer, "list") == 0) {
        const char *dat;
        int i;

        for (i = 0; (dat = G_datum_name(i)); i++) {
            fprintf(stdout, "---\n%d\n%s\n%s\n%s ellipsoid\n", i, dat,
                    G_datum_description(i), G_datum_ellipsoid(i));
        }

        exit(EXIT_SUCCESS);
    }

    epsg = inepsg->answer;
    projinfo = projunits = projepsg = NULL;
    projsrid = projwkt = NULL;

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
    else if (insrid->answer)
        /* Input as spatial reference ID */
        input_srid(insrid->answer);
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

    if ((cellhd.proj != PROJECTION_XY) &&
        (projinfo == NULL || projunits == NULL))
        G_fatal_error(_("Projection files missing"));

    /* Override input datum if requested */
    if (datum->answer)
        set_datum(datum->answer);

    /* Set Datum Parameters if necessary or requested */
    set_datumtrans(atoi(dtrans->answer), forcedatumtrans->answer);

    /* Output */
    /* Only allow one output format at a time, to reduce confusion */
    formats = ((printinfo->answer ? 1 : 0) + (shellinfo->answer ? 1 : 0) +
               (datuminfo->answer ? 1 : 0) + (printproj4->answer ? 1 : 0) +
#ifdef HAVE_OGR
               (printwkt->answer ? 1 : 0) +
#endif
               (create->answer ? 1 : 0));
    if (formats > 1) {
#ifdef HAVE_OGR
        G_fatal_error(_("Only one of -%c, -%c, -%c, -%c, -%c"
                        " or -%c flags may be specified"),
                      printinfo->key, shellinfo->key, datuminfo->key,
                      printproj4->key, printwkt->key, create->key);
#else
        G_fatal_error(_("Only one of -%c, -%c, -%c, -%c"
                        " or -%c flags may be specified"),
                      printinfo->key, shellinfo->key, datuminfo->key,
                      printproj4->key, create->key);
#endif
    }
    if ((printinfo->answer && outputFormat == PLAIN) || outputFormat == SHELL ||
        outputFormat == JSON)
        print_projinfo(outputFormat);
    else if (datuminfo->answer)
        print_datuminfo();
    else if (outputFormat == PROJ4)
        print_proj4(dontprettify->answer);
#ifdef HAVE_OGR
    else if (outputFormat == WKT)
        print_wkt(esristyle->answer, dontprettify->answer);
#endif
    else if (location->answer)
        create_location(location->answer);
    else if (create->answer)
        modify_projinfo();
    else
#ifdef HAVE_OGR
        G_fatal_error(
            _("No output format specified. Define one of the options: "
              "plain, shell, json, wkt, or proj4 using the -%c flag."),
            printinfo->key);
#else
        G_fatal_error(
            _("No output format specified. Define one of the options: "
              "plain, shell, json, or proj4 using the -%c flag."),
            printinfo->key);
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
