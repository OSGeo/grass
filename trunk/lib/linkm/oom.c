#include <grass/linkm.h>
#include <stdlib.h>

int link_out_of_memory(void)
{
    fprintf(stderr, "LinkM: Out of memory\n");
    exit(EXIT_FAILURE);
}
