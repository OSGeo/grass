#include <grass/raster.h>
#include <grass/imagery.h>

#include "bouman.h"

int create_output_labels(struct SigSet *S, struct files *files)
{
    int n;
    struct ClassSig *C;

    Rast_init_cats(S->title, &files->output_labels);
    for (n = 0; n < S->nclasses; n++) {
	C = &S->ClassSig[n];
	Rast_set_c_cat((CELL*) &(C->classnum),
		       (CELL*) &(C->classnum),
		       C->title, &files->output_labels);
    }

    return 0;
}
