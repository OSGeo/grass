
/****************************************************************************
 *
 * MODULE:       r.external.out
 *               
 * AUTHOR(S):    Glynn Clements, based on r.out.gdal
 *
 * PURPOSE:      Make GRASS write raster maps utilizing the GDAL library.
 *
 * COPYRIGHT:    (C) 2008, 2010 by Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include "gdal.h"

static void list_formats(void)
{
    /* -------------------------------------------------------------------- */
    /*      List supported formats and exit.                                */
    /*         code from GDAL 1.2.5  gcore/gdal_misc.cpp                    */
    /*         Copyright (c) 1999, Frank Warmerdam                          */
    /* -------------------------------------------------------------------- */
    int iDr;

    fprintf(stdout, _("Supported Formats:\n"));
    for (iDr = 0; iDr < GDALGetDriverCount(); iDr++) {
	GDALDriverH hDriver = GDALGetDriver(iDr);
	const char *pszRWFlag;

#ifdef GDAL_DCAP_RASTER
            /* Starting with GDAL 2.0, vector drivers can also be returned */
            /* Only keep raster drivers */
            if (!GDALGetMetadataItem(hDriver, GDAL_DCAP_RASTER, NULL))
                continue;
#endif

	if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATE, NULL))
	    pszRWFlag = "rw+";
	else if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATECOPY, NULL))
	    pszRWFlag = "rw";
	else
	    continue;

	fprintf(stdout, "  %s (%s): %s\n",
		GDALGetDriverShortName(hDriver),
		pszRWFlag, GDALGetDriverLongName(hDriver));
    }
}

static char *format_list(void)
{
    char *buf, *p;
    int len = 0;
    int first = 1;
    int i;

    for (i = 0; i < GDALGetDriverCount(); i++) {
	GDALDriverH driver = GDALGetDriver(i);

#ifdef GDAL_DCAP_RASTER
            /* Starting with GDAL 2.0, vector drivers can also be returned */
            /* Only keep raster drivers */
            if (!GDALGetMetadataItem(driver, GDAL_DCAP_RASTER, NULL))
                continue;
#endif

	if (!GDALGetMetadataItem(driver, GDAL_DCAP_CREATE, NULL) &&
	    !GDALGetMetadataItem(driver, GDAL_DCAP_CREATECOPY, NULL))
	    continue;

	len += strlen(GDALGetDriverShortName(driver)) + 1;
    }

    buf = G_malloc(len);
    p = buf;

    for (i = 0; i < GDALGetDriverCount(); i++) {
	GDALDriverH driver = GDALGetDriver(i);
	const char *name;

#ifdef GDAL_DCAP_RASTER
            /* Starting with GDAL 2.0, vector drivers can also be returned */
            /* Only keep raster drivers */
            if (!GDALGetMetadataItem(driver, GDAL_DCAP_RASTER, NULL))
                continue;
#endif

	if (!GDALGetMetadataItem(driver, GDAL_DCAP_CREATE, NULL) &&
	    !GDALGetMetadataItem(driver, GDAL_DCAP_CREATECOPY, NULL))
	    continue;

	if (first)
	    first = 0;
	else
	    *p++ = ',';

	name = GDALGetDriverShortName(driver);
	strcpy(p, name);
	p += strlen(name);
    }
    *p++ = '\0';

    return buf;
}

static void print_status(void)
{
    FILE *fp;
    struct Key_Value *key_val;
    const char *p;

    if (!G_find_file2("", "GDAL", G_mapset())) {
	fprintf(stdout, "Not using GDAL\n");
	return;
    }

    fp = G_fopen_old("", "GDAL", G_mapset());
    if (!fp)
	G_fatal_error(_("Unable to open GDAL file"));
    key_val = G_fread_key_value(fp);
    fclose(fp);

    p = G_find_key_value("directory", key_val);
    fprintf(stdout, "directory: %s\n", p ? p : "not set (default 'gdal')");

    p = G_find_key_value("extension", key_val);
    fprintf(stdout, "extension: %s\n", p ? p : "<none>");

    p = G_find_key_value("format", key_val);
    fprintf(stdout, "format: %s\n", p ? p : "not set (default GTiff)");

    p = G_find_key_value("options", key_val);
    fprintf(stdout, "options: %s\n", p ? p : "<none>");

    G_free_key_value(key_val);
}

static void check_format(const char *format)
{
    GDALDriverH driver = GDALGetDriverByName(format);

    if (!driver)
	G_fatal_error(_("Format <%s> not supported"), format);

    if (GDALGetMetadataItem(driver, GDAL_DCAP_CREATE, NULL))
	return;

    if (GDALGetMetadataItem(driver, GDAL_DCAP_CREATECOPY, NULL)) {
	G_warning(_("Format <%s> does not support direct write"), format);
	return;
    }

    G_fatal_error(_("Format <%s> does not support writing"), format);
}

static void make_link(const char *dir, const char *ext,
		      const char *format, char **options)
{
    struct Key_Value *key_val = G_create_key_value();
    char *opt_str = NULL;
    FILE *fp;

    if (options && *options) {
	int n_opts = 0, opt_len = 0, i;
	char *p;

	for (i = 0; options[i]; i++) {
	    n_opts++;
	    opt_len += strlen(options[i]) + 1;
	}

	opt_str = G_malloc(opt_len);
	p = opt_str;

	for (i = 0; i < n_opts; i++) {
	    if (i > 0)
		*p++ = ',';
	    strcpy(p, options[i]);
	    p += strlen(options[i]);
	}
	*p++ = '\0';
    }

    if (ext && ext[0] != '.') {
	char *p;
	G_asprintf(&p, ".%s", ext);
	ext = p;
    }

    if (dir)
	G_set_key_value("directory", dir, key_val);
    if (ext)
	G_set_key_value("extension", ext, key_val);
    if (format)
	G_set_key_value("format", format, key_val);
    if (opt_str)
	G_set_key_value("options", opt_str, key_val);

    fp = G_fopen_new("", "GDAL");
    if (!fp)
	G_fatal_error(_("Unable to create GDAL file"));

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing GDAL file"));

    fclose(fp);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct {
	struct Option *dir, *ext, *format, *opts;
    } parm;
    struct Flag *flag_f, *flag_r, *flag_p;

    G_gisinit(argv[0]);

    GDALAllRegister();

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    G_add_keyword(_("external"));
    module->description =
	_("Redirects raster output to file utilizing GDAL library rather than storing in GRASS raster format.");

    parm.dir = G_define_option();
    parm.dir->key = "directory";
    parm.dir->description = _("Name of output directory");
    parm.dir->required = YES;
    parm.dir->type = TYPE_STRING;
    parm.dir->key_desc = "path";
    
    parm.ext = G_define_option();
    parm.ext->key = "extension";
    parm.ext->description = _("Extension for output files");
    parm.ext->required = NO;
    parm.ext->type = TYPE_STRING;

    parm.format = G_define_option();
    parm.format->key = "format";
    parm.format->description = _("Format of output files");
    parm.format->required = YES;
    parm.format->type = TYPE_STRING;
    parm.format->options = format_list();

    parm.opts = G_define_option();
    parm.opts->key = "options";
    parm.opts->description = _("Creation options");
    parm.opts->required = NO;
    parm.opts->multiple = YES;
    parm.opts->type = TYPE_STRING;

    flag_f = G_define_flag();
    flag_f->key = 'f';
    flag_f->description = _("List supported formats and exit");
    flag_f->guisection = _("Print");
    flag_f->suppress_required = YES;
    
    flag_r = G_define_flag();
    flag_r->key = 'r';
    flag_r->description = _("Cease using GDAL and revert to native output");
    flag_r->suppress_required = YES;
    
    flag_p = G_define_flag();
    flag_p->key = 'p';
    flag_p->description = _("Print current status");
    flag_p->guisection = _("Print");
    flag_p->suppress_required = YES;
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (flag_p->answer) {
	print_status();
	exit(EXIT_SUCCESS);
    }

    if (flag_f->answer) {
	list_formats();
	exit(EXIT_SUCCESS);
    }

    if (flag_r->answer) {
	G_remove("", "GDAL");
	exit(EXIT_SUCCESS);
    }

    if (parm.format->answer)
	check_format(parm.format->answer);

    make_link(parm.dir->answer, parm.ext->answer, parm.format->answer,
	      parm.opts->answers);

    exit(EXIT_SUCCESS);
}

