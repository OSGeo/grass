#include <grass/config.h>

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int hitreturn(void)
{
    char buf[100];

    G_message(_("\nhit RETURN to continue -->"));
    G_gets(buf);

    return EXIT_SUCCESS;
}
