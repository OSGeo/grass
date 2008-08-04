#include "list.h"
#include "local_proto.h"

int init(char *pgm)
{
    G_gisinit(pgm);

    read_list(0);

    return 0;
}
