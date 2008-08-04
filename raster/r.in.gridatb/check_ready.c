#include "local_proto.h"
#include <grass/glocale.h>


int check_ready(void)
{
    FILE *fp;
    int retval;

    retval = 0;

    if (!(fp = fopen(file, "r")))
	retval = 1;
    else
	fclose(fp);

    return (retval);
}
