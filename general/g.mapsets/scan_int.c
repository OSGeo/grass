#include "local_proto.h"
#include <stdio.h>
int 
scan_int (char *buf, int *n)
{
    char dummy[2];

    *dummy = 0;
    if (sscanf (buf, "%d%1s", n, dummy) != 1)
        return 0;
    
    return (*dummy == 0 ? 1 : 0) ;
}
