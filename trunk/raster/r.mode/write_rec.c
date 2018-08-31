#include <stdio.h>

int write_reclass(FILE * fd, long cat1, long cat2, char *label)
{
    if (cat1 == 0) {
	cat2 = 0;
	label = "no data";
    }
    fprintf(fd, "%ld = %ld ", cat1, cat2);
    if (label && *label)
	fprintf(fd, "%s\n", label);
    else
	fprintf(fd, "(Category %ld)\n", cat2);

    return 0;
}
