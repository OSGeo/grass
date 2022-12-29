#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include "ogr_api.h"
#endif

int is_ogr(const char *format)
{
    int use_ogr = TRUE;
    
    if(strcmp(format, "PostgreSQL") == 0) {
#if defined HAVE_OGR && defined HAVE_POSTGRES
        if (!getenv("GRASS_VECTOR_OGR"))
            use_ogr  = FALSE;
#else
#ifdef HAVE_POSTGRES
      if (getenv("GRASS_VECTOR_OGR"))
	  G_warning(_("Environment variable GRASS_VECTOR_OGR defined, "
		      "but GRASS is compiled with OGR support. "
		      "Using GRASS-PostGIS data driver instead."));
      use_ogr  = FALSE;
#else /* -> force using OGR */
      G_warning(_("GRASS is not compiled with PostgreSQL support. "
		  "Using OGR-PostgreSQL driver instead of native "
		  "GRASS-PostGIS data driver."));
#endif /* HAVE_POSTRES */
#endif /* HAVE_OGR && HAVE_POSTGRES */
    }
    
    return use_ogr;
}

void check_format(char *format)
{
    if (!is_ogr(format))
        return;
    
#ifdef HAVE_OGR
    OGRSFDriverH driver;
    
    G_strchg(format, '_', ' ');
    driver = OGRGetDriverByName(format);
    
    if (!driver)
        G_fatal_error(_("Format <%s> not supported"), format);
    
    if (!OGR_Dr_TestCapability(driver, ODrCCreateDataSource))
        G_fatal_error(_("Format <%s> does not support writing"), format);
#endif
}
