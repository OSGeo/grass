#include <stdio.h>

int write_reclass(FILE * fd, long cat1, long cat2, char *label, int usecats)
{
    if (cat1 == 0) {
	cat2 = 0;
	label = "no data";
    }

    fprintf(fd, "%ld = %ld ", cat1, cat1);
    if (*label && usecats)
	fprintf(fd, "%s\n", label);
    else
	fprintf(fd, "%ld\n", cat2);

    return 0;
}
