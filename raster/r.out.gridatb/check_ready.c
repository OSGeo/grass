#include <unistd.h>
#include "local_proto.h"
#include <grass/glocale.h>


void check_ready(void)
{
    FILE *fp;

    fp = fopen(file, "r");
    if (!fp)
	return;

    fclose(fp);

    if (overwr)
	unlink(file);
    else
	G_fatal_error("%s - file already exists", file);
}
