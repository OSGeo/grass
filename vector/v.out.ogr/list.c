#include "local_proto.h"

static int cmp(const void *, const void *);

/* to print available drivers in help text */
char *OGR_list_write_drivers(void)
{
    int i, count;
    size_t len;
    OGRSFDriverH Ogr_driver;
    char buf[2000];
    
    char **list, *ret;

    list = NULL;
    count = len = 0;
    
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

int cmp(const void *a, const void *b) 
{
    return (strcmp(*(char **)a, *(char **)b));
}
