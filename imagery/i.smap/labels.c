#include <grass/Rast.h>
#include <grass/imagery.h>

#include "bouman.h"

int create_output_labels(struct SigSet *S, struct files *files)
{
    int n;
    struct ClassSig *C;

    Rast_init_cats((CELL) 0, S->title, &files->output_labels);
    for (n = 0; n < S->nclasses; n++) {
	C = &S->ClassSig[n];
	Rast_set_cat((CELL) C->classnum, C->title, &files->output_labels);
    }

    return 0;
}
