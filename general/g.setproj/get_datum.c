#include <string.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include "local_proto.h"

int ask_datum(char *datum, char *ellps, char *params)
{
    int answer;

    answer = G_ask_datum_name(datum, ellps);
    if (answer > 0) {
	char *pparams;

	answer = GPJ_ask_datum_params(datum, &pparams);

	if (answer > 0) {
	    strcpy(params, pparams);
	    G_free(pparams);
	    return 1;
	}

	else
	    return -1;
    }
    else
	return -1;


}
