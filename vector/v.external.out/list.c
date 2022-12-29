#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include "ogr_api.h"
#endif /* HAVE_OGR */

static int cmp(const void *, const void *);
static char **format_list(int *, size_t *);

int cmp(const void *a, const void *b)
{
    return strcmp(*(char **)a, *(char **)b);
}

char **format_list(int *count, size_t *len)
{
    int i;
    char **list;

    list = NULL;
    *count = 0;
    if (len)
	*len = 0;

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
	
	list = G_realloc(list, ((*count) + 1) * sizeof(char *));

	/* chg white space to underscore in OGR driver names */
	sprintf(buf, "%s", OGR_Dr_GetName(Ogr_driver));
	G_strchg(buf, ' ', '_');
	list[(*count)++] = G_store(buf);
	if (len)
	    *len += strlen(buf) + 1; /* + ',' */
    }

    /* order formats by name */
    qsort(list, *count, sizeof(char *), cmp);
#endif
#if defined HAVE_POSTGRES && !defined HAVE_OGR
    list = G_realloc(list, ((*count) + 1) * sizeof(char *));
    list[(*count)++] = G_store("PostgreSQL");
    if (len)
	*len += strlen("PostgreSQL") + 1;
#endif 

    return list;
}

char *format_options()
{
    int  i, count;
    char **list, *ret;
    size_t len;
    
    ret  = NULL;
    list = format_list(&count, &len);
    
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
    int i, count;
    char **list;
    
    G_message(_("Supported formats:"));

    list = format_list(&count, NULL);
    
    for (i = 0; i < count; i++)
	fprintf(stdout, "%s\n", list[i]);
    fflush(stdout);

    G_free(list);
}
