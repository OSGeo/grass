#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include "ogr_api.h"
#endif

void check_format(char *format)
{
    if(strcmp(format, "PostgreSQL") == 0)
	return;
    
#ifdef HAVE_OGR
    OGRSFDriverH driver;
    
    G_strchg(format, '_', ' ');
    driver = OGRGetDriverByName(format);

    if (!driver)
	G_fatal_error(_("Format <%s> not supported"), format);

    if (OGR_Dr_TestCapability(driver, ODrCCreateDataSource))
	return;

    G_fatal_error(_("Format <%s> does not support writing"), format);
#endif
}
