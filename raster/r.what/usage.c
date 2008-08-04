#include <stdlib.h>
#include <grass/gis.h>
#include "local_proto.h"

void usage(void)
{
    char *message;

    G_asprintf(&message, "%s [-fci] [null=string] layer1 [layer2] ...",
	       G_program_name());
    G_fatal_error(message);

    return;
}
