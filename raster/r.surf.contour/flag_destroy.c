#include <stdlib.h>
#include <grass/gis.h>
#include "flag.h"

int flag_destroy(FLAG * flags)
{
    G_free(flags->array[0]);
    G_free(flags->array);
    G_free(flags);

    return 0;
}
