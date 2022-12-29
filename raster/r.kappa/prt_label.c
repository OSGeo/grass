#include <grass/raster.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"


void prt_label(void)
{
    int i, j;
    long *cats;
    char *cl;
    FILE *fd;

    if (output == NULL)
	fd = stdout;
    else if ((fd = fopen(output, "a")) == NULL) {
	G_fatal_error(_("Can't open file <%s> to write label"), output);
	return;
    }

    /* print labels */
    for (i = 0; i < nlayers; i++) {
	fprintf(fd, "\n");
	fprintf(fd, "MAP%-d Category Description\n", i + 1);
	for (j = 0; j < ncat; j++) {
	    cats = rlst;
	    cl = Rast_get_c_cat((CELL *) &(cats[j]), &(layers[i].labels));
	    if (cl)
		G_strip(cl);
	    if (cl == NULL || *cl == 0)
		cl = "(no description)";
	    fprintf(fd, "%ld:  %s\n", rlst[j], cl);
	}
    }
    if (output != NULL)
	fclose(fd);
}
