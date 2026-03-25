/***************************************************************************
 *
 * MODULE:       g.version
 * AUTHOR(S):    Michael Shapiro, CERL
 *               Andreas Lange - <andreas.lange rhein-main.de>
 *               Justin Hickey - Thailand - jhickey hpcc.nectec.or.th
 *               Extended info by Martin Landa <landa.martin gmail.com>
 * PURPOSE:      Output GRASS version number, date and copyright message.
 *
 * COPYRIGHT:    (C) 2000-2015 by the GRASS Development Team
 *
 *               This program is free software under the GPL (>=v2)
 *               Read the file COPYING that comes with GRASS for
 *               details.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gjson.h>

#include <gdal_version.h>

#include "local_proto.h"

#include <proj.h>

#ifdef HAVE_GEOS
#include <geos_c.h>
#endif

#ifdef HAVE_SQLITE
#include <sqlite3.h>
#endif

#ifndef GRASS_VERSION_UPDATE_PKG
#define GRASS_VERSION_UPDATE_PKG "0.1"
#endif

/* TODO: remove this style of include */
static const char COPYING[] =
#include <grass/copying.h>
    ;

static const char CITING[] =
#include <grass/citing.h>
    ;

static const char GRASS_CONFIGURE_PARAMS[] =
#include <grass/confparms.h>
    ;

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Flag *copyright, *build, *gish_rev, *cite_flag, *shell, *extended;
    struct Option *fopt;
    enum OutputFormat format;
    G_JSON_Value *root_value = NULL;
    G_JSON_Object *root_object = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("support"));
    G_add_keyword(_("citing"));
    G_add_keyword(_("copyright"));
    G_add_keyword(_("version"));
    G_add_keyword(_("license"));
    module->label = _("Displays GRASS version info.");
    module->description =
        _("Optionally also prints build or copyright information.");

    copyright = G_define_flag();
    copyright->key = 'c';
    copyright->description = _("Print also the copyright message");
    copyright->guisection = _("Additional info");

    cite_flag = G_define_flag();
    cite_flag->key = 'x';
    cite_flag->description = _("Print also the citation options");
    cite_flag->guisection = _("Additional info");

    build = G_define_flag();
    build->key = 'b';
    build->description = _("Print also the build information");
    build->guisection = _("Additional info");

    gish_rev = G_define_flag();
    gish_rev->key = 'r';
    /* this was never the library revision number and date
     * it was the revision number and date of gis.h
     * now it is the git hash and date of all GRASS headers
     * (and anything else in include) */
    gish_rev->description =
        _("Print also the GIS library revision number and date");
    gish_rev->guisection = _("Additional info");

    extended = G_define_flag();
    extended->key = 'e';
    extended->label = _("Print also extended info for additional libraries");
    extended->description = _("GDAL/OGR, PROJ, GEOS");
    extended->guisection = _("Additional info");

    shell = G_define_flag();
    shell->key = 'g';
    shell->label = _("Print info in shell script style (including Git "
                     "reference commit) [deprecated]");
    shell->description = _(
        "This flag is deprecated and will be removed in a future release. Use "
        "format=shell instead.");
    shell->guisection = _("Shell");

    fopt = G_define_standard_option(G_OPT_F_FORMAT);
    fopt->required = NO;
    fopt->options = "plain,shell,json";
    fopt->descriptions = _("plain;Human readable text output;"
                           "shell;shell script style text output;"
                           "json;JSON (JavaScript Object Notation);");
    fopt->guisection = _("Print");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (strcmp(fopt->answer, "json") == 0) {
        format = JSON;
        root_value = G_json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        root_object = G_json_object(root_value);
    }
    else if (strcmp(fopt->answer, "shell") == 0) {
        format = SHELL;
    }
    else {
        format = PLAIN;
    }

    if (shell->answer) {
        G_verbose_message(
            _("Flag 'g' is deprecated and will be removed in a future "
              "release. Please use format=shell instead."));
        if (format == JSON) {
            G_json_value_free(root_value);
            G_fatal_error(_("Cannot use the -g flag with format=json; "
                            "please select only one option."));
        }
        format = SHELL;
    }

    char date_str[30];

    switch (format) {
    case SHELL:
        fprintf(stdout, "version=%s\n", GRASS_VERSION_NUMBER);
        fprintf(stdout, "date=%s\n", GRASS_VERSION_DATE);
        fprintf(stdout, "revision=%s\n", GRASS_VERSION_GIT);
        fprintf(stdout, "build_date=%d-%02d-%02d\n", YEAR, MONTH, DAY);
        fprintf(stdout, "build_platform=%s\n", ARCH);
        fprintf(stdout, "build_off_t_size=%zu\n", sizeof(off_t));
        break;
    case PLAIN:
        fprintf(stdout, "GRASS %s (%s)\n", GRASS_VERSION_NUMBER,
                GRASS_VERSION_DATE);
        break;
    case JSON:
        G_json_object_set_string(root_object, "version", GRASS_VERSION_NUMBER);
        G_json_object_set_string(root_object, "date", GRASS_VERSION_DATE);
        G_json_object_set_string(root_object, "revision", GRASS_VERSION_GIT);

        snprintf(date_str, sizeof(date_str), "%d-%02d-%02d", YEAR, MONTH, DAY);
        G_json_object_set_string(root_object, "build_date", date_str);
        G_json_object_set_string(root_object, "build_platform", ARCH);
        G_json_object_set_number(root_object, "build_off_t_size",
                                 sizeof(off_t));
        break;
    }

    if (copyright->answer) {
        switch (format) {
        case PLAIN:
        case SHELL:
            fprintf(stdout, "\n");
            fputs(COPYING, stdout);
            break;
        case JSON:
            G_json_object_set_string(root_object, "copyright", COPYING);
            break;
        }
    }

    if (cite_flag->answer) {
        switch (format) {
        case PLAIN:
        case SHELL:
            fprintf(stdout, "\n");
            fputs(CITING, stdout);
            break;
        case JSON:
            G_json_object_set_string(root_object, "citation", CITING);
            break;
        }
    }

    if (build->answer) {
        switch (format) {
        case PLAIN:
        case SHELL:
            fprintf(stdout, "\n");
            fputs(GRASS_CONFIGURE_PARAMS, stdout);
            fprintf(stdout, "\n");
            break;
        case JSON:
            G_json_object_set_string(root_object, "build_info",
                                     GRASS_CONFIGURE_PARAMS);
            break;
        }
    }

    if (gish_rev->answer) {
        char *rev_ver = GIS_H_VERSION;
        char *rev_time = GIS_H_DATE;
        int no_libgis = FALSE;

        if (*rev_ver && *rev_time) {
            switch (format) {
            case SHELL:
                fprintf(stdout, "libgis_revision=%s\n", rev_ver);
                fprintf(stdout, "libgis_date=%s\n", rev_time);
                break;
            case PLAIN:
                fprintf(stdout, "libgis revision: %s\n", rev_ver);
                fprintf(stdout, "libgis date: %s\n", rev_time);
                break;
            case JSON:
                G_json_object_set_string(root_object, "libgis_revision",
                                         rev_ver);
                G_json_object_set_string(root_object, "libgis_date", rev_time);
                break;
            }
        }
        else {
            no_libgis = TRUE;
            switch (format) {
            case SHELL:
                fprintf(stdout, "libgis_revision=\n");
                fprintf(stdout, "libgis_date=\n");
                G_warning("GRASS libgis version and date number not available");
                /* this can be alternatively fatal error or it can cause
                   fatal error later */
                break;
            case PLAIN:
                fprintf(
                    stdout,
                    _("Cannot determine GRASS libgis version and date number."
                      " The GRASS build might be broken."
                      " Report this to developers or packagers.\n"));
                break;
            case JSON:
                G_json_object_set_null(root_object, "libgis_revision");
                G_json_object_set_null(root_object, "libgis_date");
                G_warning("GRASS libgis version and date number not available");
                break;
            }
        }
        if (no_libgis) {
            G_debug(1, _("GRASS libgis version and date number don't have "
                         "the expected format."
                         " Trying to print the original strings..."));
            G_debug(1, _("GIS_H_VERSION=\"%s\""), GIS_H_VERSION);
            G_debug(1, _("GIS_H_DATE=\"%s\""), GIS_H_DATE);
        }
    }

    if (extended->answer) {
        char *proj = NULL;

        G_asprintf(&proj, "%d%d%d", PROJ_VERSION_MAJOR, PROJ_VERSION_MINOR,
                   PROJ_VERSION_PATCH);
        if (strlen(proj) == 3) {
            char proj_str[6];
            snprintf(proj_str, sizeof(proj_str), "%c.%c.%c", proj[0], proj[1],
                     proj[2]);

            switch (format) {
            case SHELL:
                fprintf(stdout, "proj=%s\n", proj_str);
                break;
            case PLAIN:
                fprintf(stdout, "PROJ: %s\n", proj_str);
                break;
            case JSON:
                G_json_object_set_string(root_object, "proj", proj_str);
                break;
            }
        }
        else {
            switch (format) {
            case SHELL:
                fprintf(stdout, "proj=%s\n", proj);
                break;
            case PLAIN:
                fprintf(stdout, "PROJ: %s\n", proj);
                break;
            case JSON:
                G_json_object_set_string(root_object, "proj", proj);
                break;
            }
        }
        switch (format) {
        case SHELL:
            fprintf(stdout, "gdal=%s\n", GDAL_RELEASE_NAME);
            break;
        case PLAIN:
            fprintf(stdout, "GDAL/OGR: %s\n", GDAL_RELEASE_NAME);
            break;
        case JSON:
            G_json_object_set_string(root_object, "gdal", GDAL_RELEASE_NAME);
            break;
        }
#ifdef HAVE_GEOS
        switch (format) {
        case SHELL:
            fprintf(stdout, "geos=%s\n", GEOS_VERSION);
            break;
        case PLAIN:
            fprintf(stdout, "GEOS: %s\n", GEOS_VERSION);
            break;
        case JSON:
            G_json_object_set_string(root_object, "geos", GEOS_VERSION);
            break;
        }
#else
        switch (format) {
        case SHELL:
            fprintf(stdout, "geos=\n");
            break;
        case PLAIN:
            fprintf(stdout, "%s\n", _("GRASS not compiled with GEOS support"));
            break;
        case JSON:
            G_json_object_set_null(root_object, "geos");
            break;
        }
#endif
#ifdef HAVE_SQLITE
        switch (format) {
        case SHELL:
            fprintf(stdout, "sqlite=%s\n", SQLITE_VERSION);
            break;
        case PLAIN:
            fprintf(stdout, "SQLite: %s\n", SQLITE_VERSION);
            break;
        case JSON:
            G_json_object_set_string(root_object, "sqlite", SQLITE_VERSION);
            break;
        }
#else
        switch (format) {
        case SHELL:
            fprintf(stdout, "sqlite=\n");
            break;
        case PLAIN:
            fprintf(stdout, "%s\n",
                    _("GRASS not compiled with SQLite support"));
            break;
        case JSON:
            G_json_object_set_null(root_object, "sqlite");
            break;
        }
#endif
    }

    if (format == JSON) {
        char *serialized_string = NULL;
        serialized_string = G_json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        G_json_free_serialized_string(serialized_string);
        G_json_value_free(root_value);
    }

    return (EXIT_SUCCESS);
}
