#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "ogr_api.h"

int is_ogr(const char *format)
{
    int use_ogr = FALSE;

    if (strcmp(format, "PostgreSQL") == 0) {
        if (getenv("GRASS_VECTOR_OGR"))
            use_ogr = TRUE;

#if defined(HAVE_POSTGRES)
        if (use_ogr) {
            G_warning(_("Environment variable GRASS_VECTOR_OGR is defined,"
                        "using OGR-PostgreSQL driver instead of native "
                        "GRASS-PostGIS data driver."));
        }
#else  /* -> force using OGR */
        G_warning(_("GRASS is not compiled with PostgreSQL support. "
                    "Using OGR-PostgreSQL driver instead of native "
                    "GRASS-PostGIS data driver."));
        use_ogr = TRUE;
#endif /* HAVE_POSTGRES */
    }

    return use_ogr;
}

void check_format(char *format)
{
    if (!is_ogr(format))
        return;

    OGRSFDriverH driver;

    G_strchg(format, '_', ' ');
    driver = OGRGetDriverByName(format);

    if (!driver)
        G_fatal_error(_("Format <%s> not supported"), format);

    if (!OGR_Dr_TestCapability(driver, ODrCCreateDataSource))
        G_fatal_error(_("Format <%s> does not support writing"), format);
}
