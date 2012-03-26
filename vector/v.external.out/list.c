#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include "ogr_api.h"

int cmp(const void *a, const void *b) 
{
    return (strcmp(*(char **)a, *(char **)b));
}
#endif /* HAVE_OGR */

char *format_list(void)
{
    int i, count;
    size_t len;
    
    char **list, *ret;

    list = NULL;
    count = len = 0;
    ret = NULL;

#ifdef HAVE_OGR
    char buf[2000];
    
    OGRSFDriverH Ogr_driver;
    
    /* Open OGR DSN */
    OGRRegisterAll();
    G_debug(2, "driver count = %d", OGRGetDriverCount());
    for (i = 0; i < OGRGetDriverCount(); i++) {
	/* only fetch read/write drivers */
	if (!OGR_Dr_TestCapability(OGRGetDriver(i), ODrCCreateDataSource))
	    continue;
	
	Ogr_driver = OGRGetDriver(i);
	G_debug(2, "driver %d/%d : %s", i, OGRGetDriverCount(),
		OGR_Dr_GetName(Ogr_driver));
	
	list = G_realloc(list, (count + 1) * sizeof(char *));

	/* chg white space to underscore in OGR driver names */
	sprintf(buf, "%s", OGR_Dr_GetName(Ogr_driver));
	G_strchg(buf, ' ', '_');
	list[count++] = G_store(buf);
	len += strlen(buf) + 1; /* + ',' */
    }

    qsort(list, count, sizeof(char *), cmp);
#endif
#if defined HAVE_POSTGRES && !defined HAVE_OGR
    list = G_realloc(list, (count + 1) * sizeof(char *));
    list[count++] = G_store("PostgreSQL");
    len += strlen("PostgreSQL") + 1;
#endif 
    if (len > 0) {
	ret = G_malloc((len + 1) * sizeof(char)); /* \0 */
	*ret = '\0';
	for (i = 0; i < count; i++) {
	    if (i > 0)
		strcat(ret, ",");
	    strcat(ret, list[i]);
	    G_free(list[i]);
	}
	G_free(list);
    }
    else {
	ret = G_store("");
    }
    
    G_debug(2, "all drivers: %s", ret);
    
    return ret;
}

void list_formats(void)
{
    G_message(_("List of supported formats:"));

#ifdef HAVE_OGR
    /* -------------------------------------------------------------------- */
    /*      List supported formats and exit.                                */
    /*         code from GDAL 1.2.5  gcore/gdal_misc.cpp                    */
    /*         Copyright (c) 1999, Frank Warmerdam                          */
    /* -------------------------------------------------------------------- */
    int iDr;
    OGRSFDriverH driver;


    for (iDr = 0; iDr < OGRGetDriverCount(); iDr++) {
	driver = OGRGetDriver(iDr);

	if (!OGR_Dr_TestCapability(driver, ODrCCreateDataSource))
	    continue;

	fprintf(stdout, "%s\n", OGR_Dr_GetName(driver));
    }
#endif
#ifdef HAVE_POSTGRES
    fprintf(stdout, "PostGIS\n");
#endif
    fflush(stdout);
}
