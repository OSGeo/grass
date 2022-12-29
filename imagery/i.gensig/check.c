#include <grass/imagery.h>
#include <grass/glocale.h>
#include "signature.h"
#include "local_proto.h"


int check_signatures(struct Signature *S)
{
    int i, j;
    struct Signature temp;
    double *lambda;


    lambda = (double *)G_calloc(S->nbands, sizeof(double));
    I_init_signatures(&temp, S->nbands);
    I_new_signature(&temp);
    for (i = 0; i < S->nsigs; i++) {
	copy_covariances(temp.sig[0].var, S->sig[i].var, S->nbands);
	if (!can_invert(temp.sig[0].var, S->nbands)) {
	    S->sig[i].status = -1;
	    G_important_message(_("Signature %d not invertible"), i + 1);
	    continue;
	}
	copy_covariances(temp.sig[0].var, S->sig[i].var, S->nbands);
	if (!eigen(temp.sig[0].var, lambda, S->nbands)) {
	    S->sig[i].status = -1;
	    G_important_message(_("Signature %d unable to get eigen values"), i + 1);
	    continue;
	}
	for (j = 0; j < S->nbands; j++) {
	    if (lambda[j] <= 0.0) {
		S->sig[i].status = -1;
		G_important_message(_("Signature %d not positive definite"), i + 1);
		break;
	    }
	}
    }
    G_free(lambda);
    I_free_signatures(&temp);

    return 0;
}
