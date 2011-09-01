#include <grass/gis.h>
#include <grass/glocale.h>

#include "ogr_api.h"

char *format_list(void)
{
    char *buf, *p;
    int len = 0;
    char first = TRUE;
    int i;

    OGRSFDriverH driver;
    
    for (i = 0; i < OGRGetDriverCount(); i++) {
	driver = OGRGetDriver(i);

	if (!OGR_Dr_TestCapability(driver, ODrCCreateDataSource))
	    continue;

	len += strlen(OGR_Dr_GetName(driver)) + 1;
    }

    buf = G_malloc(len);
    p = buf;

    for (i = 0; i < OGRGetDriverCount(); i++) {
	const char *name;

	driver = OGRGetDriver(i);
	if (!OGR_Dr_TestCapability(driver, ODrCCreateDataSource))
	    continue;

	if (first)
	    first = FALSE;
	else
	    *p++ = ',';

	name = OGR_Dr_GetName(driver);
	G_strchg(buf, ' ', '_');
	strcpy(p, name);
	p += strlen(name);
    }
    *p++ = '\0';

    return buf;
}

void list_formats(void)
{
    /* -------------------------------------------------------------------- */
    /*      List supported formats and exit.                                */
    /*         code from GDAL 1.2.5  gcore/gdal_misc.cpp                    */
    /*         Copyright (c) 1999, Frank Warmerdam                          */
    /* -------------------------------------------------------------------- */
    int iDr;
    OGRSFDriverH driver;

    G_message(_("Supported Formats:"));
    for (iDr = 0; iDr < OGRGetDriverCount(); iDr++) {
	driver = OGRGetDriver(iDr);

	if (!OGR_Dr_TestCapability(driver, ODrCCreateDataSource))
	    continue;

	fprintf(stdout, " %s\n", OGR_Dr_GetName(driver));
    }
    fflush(stdout);
}
