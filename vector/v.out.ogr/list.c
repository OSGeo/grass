#include "local_proto.h"

/* to print available drivers in help text */
char *OGR_list_write_drivers(void)
{
    int drn, i;
    OGRSFDriverH Ogr_driver;
    char buf[2000];

    char OGRdrivers[2000];

    OGRdrivers[0] = '\0';

    /* Open OGR DSN */
    OGRRegisterAll();
    G_debug(2, "driver count = %d", OGRGetDriverCount());
    drn = -1;
    for (i = 0; i < OGRGetDriverCount(); i++) {
	/* only fetch read/write drivers */
	if (OGR_Dr_TestCapability(OGRGetDriver(i), ODrCCreateDataSource)) {
	    Ogr_driver = OGRGetDriver(i);
	    G_debug(2, "driver %d/%d : %s", i, OGRGetDriverCount(),
		    OGR_Dr_GetName(Ogr_driver));
	    /* chg white space to underscore in OGR driver names */
	    sprintf(buf, "%s", OGR_Dr_GetName(Ogr_driver));
	    G_strchg(buf, ' ', '_');
	    strcat(OGRdrivers, buf);
	    if (i < OGRGetDriverCount() - 1)
		strcat(OGRdrivers, ",");
	}
    }
    G_debug(2, "all drivers: %s", OGRdrivers);

    return G_store(OGRdrivers);
}
