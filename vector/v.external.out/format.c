#include <grass/gis.h>
#include <grass/glocale.h>

#include "ogr_api.h"

void check_format(char *format)
{
    OGRSFDriverH driver;
    
    G_strchg(format, '_', ' ');
    driver = OGRGetDriverByName(format);

    if (!driver)
	G_fatal_error(_("Format <%s> not supported"), format);

    if (OGR_Dr_TestCapability(driver, ODrCCreateDataSource))
	return;

    G_fatal_error(_("Format <%s> does not support writing"), format);
}
