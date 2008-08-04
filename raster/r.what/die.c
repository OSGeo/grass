#include <grass/gis.h>
#include "local_proto.h"

void die(char *a, char *b)
{
    char *message;

    G_asprintf(&message, "%s: %s %s", G_program_name(), a, b);
    G_fatal_error(message);

    return;
}
