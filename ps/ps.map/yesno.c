#include <stdio.h>
#include "local_proto.h"

int yesno(char *key, char *data)
{
    char buf[2];

    if (sscanf(data, "%1s", buf) != 1)
	return 1;
    if (*buf == 'y' || *buf == 'Y')
	return 1;
    if (*buf == 'n' || *buf == 'N')
	return 0;

    error(key, data, "??");
    return 0;
}
