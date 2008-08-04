#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>

int input(FILE * fp, int tty, char *buf)
{
    char temp1[10], temp2[2];

    if (tty)
	fprintf(stderr, "> ");
    if (!G_getl2(buf, 1024, fp))
	return 0;
    if (sscanf(buf, "%5s%1s", temp1, temp2) == 1 && strcmp(temp1, "end") == 0)
	return 0;
    return 1;
}
