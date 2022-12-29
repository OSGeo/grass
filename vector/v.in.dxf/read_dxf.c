#include <stdlib.h>
#include <string.h>
#include "global.h"

static char *dxf_fgets(char *, int, struct dxf_file *);

/* on error, returns -1, otherwise returns 0 */
struct dxf_file *dxf_open(char *file)
{
    struct dxf_file *dxf;

    dxf = (struct dxf_file *)G_malloc(sizeof(struct dxf_file));

    dxf->name = G_store(file);
    if (!(dxf->fp = fopen(file, "r")))
	return NULL;

    /* get the file size */
    G_fseek(dxf->fp, 0L, SEEK_END);
    dxf->size = G_ftell(dxf->fp);
    rewind(dxf->fp);

    dxf->pos = 0;

    if (dxf->size < 500000)
	dxf->percent = 10;
    else if (dxf->size < 800000)
	dxf->percent = 5;
    else
	dxf->percent = 2;

    /* initialize G_percent() */
    G_percent(0, 100, dxf->percent);

    return dxf;
}

void dxf_close(struct dxf_file *dxf)
{
    fclose(dxf->fp);
    G_free(dxf->name);
    G_free(dxf);

    return;
}

/* returns a zero if header not found, returns a 1 if found */
int dxf_find_header(struct dxf_file *dxf)
{
    while (dxf_get_code(dxf) != -2) {
	/* some dxf files will not have header information */
	if (strcmp(dxf_buf, "HEADER") == 0 ||
	    strcmp(dxf_buf, "ENTITIES") == 0)
	    return strcmp(dxf_buf, "HEADER") == 0;
    }

    G_fatal_error(_("end of file while looking for HEADER"));

    return -1;
}

/* returns a DXF code
 * -1 if non-numeric
 * -2 on EOF
 */
int dxf_read_code(struct dxf_file *dxf, char *buf, int size)
{
    if (!dxf_fgets(buf, size, dxf))
	return -2;

    if (buf[0] >= '0' && buf[0] <= '9') {
	int code = atoi(buf);

	if (!dxf_fgets(buf, size, dxf))
	    return -2;

	return code;
    }

    return -1;			/* not numeric */
}

static char *dxf_fgets(char *buf, int size, struct dxf_file *dxf)
{
    char *p;
    double perc;

    if ((p = fgets(buf, size, dxf->fp))) {
	dxf->pos += strlen(p);
	perc = (double) dxf->pos / dxf->size;
	G_percent((int) (perc * 100.0), 100, dxf->percent);
	G_squeeze(buf);
    }

    return p;
}
